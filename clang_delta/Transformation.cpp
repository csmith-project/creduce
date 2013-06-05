//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "Transformation.h"

#include <sstream>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/SmallString.h"

using namespace clang;

class TransNameQueryVisitor : public 
        RecursiveASTVisitor<TransNameQueryVisitor> {

public:
  TransNameQueryVisitor(TransNameQueryWrap *Instance, 
                        const std::string &Prefix)
    : WrapInstance(Instance),
      NamePrefix(Prefix)
  { }

  bool VisitVarDecl(VarDecl *VD);

private:
  TransNameQueryWrap *WrapInstance;

  const std::string NamePrefix;

};

bool TransNameQueryVisitor::VisitVarDecl(VarDecl *VD)
{
  std::string Name = VD->getNameAsString();
  size_t Sz = NamePrefix.size();

  if (Name.compare(0, Sz, NamePrefix))
    return true;

  std::string PostfixStr = Name.substr(Sz);
  TransAssert((PostfixStr.size() > 0) && "Bad trans tmp name!");

  std::stringstream TmpSS(PostfixStr);
  unsigned int PostfixV;
  if (!(TmpSS >> PostfixV))
    TransAssert(0 && "Non-integer trans tmp name!");

  if (PostfixV > WrapInstance->MaxPostfix)
    WrapInstance->MaxPostfix = PostfixV;

  return true;
}

TransNameQueryWrap::TransNameQueryWrap(const std::string &Prefix)
  : NamePrefix(Prefix),
    MaxPostfix(0)
{
  NameQueryVisitor = new TransNameQueryVisitor(this, Prefix);
}

TransNameQueryWrap::~TransNameQueryWrap(void)
{
  delete NameQueryVisitor;
}

bool TransNameQueryWrap::TraverseDecl(Decl *D)
{
  return NameQueryVisitor->TraverseDecl(D);
}

void Transformation::Initialize(ASTContext &context) 
{
  Context = &context;
  SrcManager = &Context->getSourceManager();
  TheRewriter.setSourceMgr(Context->getSourceManager(), 
                           Context->getLangOpts());
  RewriteHelper = RewriteUtils::GetInstance(&TheRewriter);
}

void Transformation::outputTransformedSource(llvm::raw_ostream &OutStream)
{
  FileID MainFileID = SrcManager->getMainFileID();
  const RewriteBuffer *RWBuf = TheRewriter.getRewriteBufferFor(MainFileID);

  // RWBuf is non-empty upon any rewrites
  TransAssert(RWBuf && "Empty RewriteBuffer!");
  OutStream << std::string(RWBuf->begin(), RWBuf->end());
  OutStream.flush();
}

void Transformation::outputOriginalSource(llvm::raw_ostream &OutStream)
{
  FileID MainFileID = SrcManager->getMainFileID();
  const llvm::MemoryBuffer *MainBuf = SrcManager->getBuffer(MainFileID);
  TransAssert(MainBuf && "Empty MainBuf!");
  OutStream << MainBuf->getBufferStart(); 
  OutStream.flush();
}

void Transformation::getTransErrorMsg(std::string &ErrorMsg)
{
  if (TransError == TransSuccess) {
    ErrorMsg = "";
  }
  else if (TransError == TransInternalError) {
    ErrorMsg = "Internal transformation error!";
  }
  else if (TransError == TransMaxInstanceError) {
    ErrorMsg = "The counter value exceeded the number of transformation instances!";
  }
  else if (TransError == TransMaxVarsError) {
    ErrorMsg = "Too many variables!";
  }
  else if (TransError == TransMaxClassesError) {
    ErrorMsg = "Too many classes!";
  }
  else if (TransError == TransNoValidVarsError) {
    ErrorMsg = "No variables need to be renamed!";
  }
  else if (TransError == TransNoValidFunsError) {
    ErrorMsg = "No valid function declarations exist!";
  }
  else if (TransError == TransNoValidParamsError) {
    ErrorMsg = "No valid parameters declarations exist!";
  }
  else if (TransError == TransNoTextModificationError) {
    ErrorMsg = "No modification to the transformed program!";
  }
  else {
    TransAssert(0 && "Unknown transformation error!");
  }
}

const Expr *
Transformation::ignoreSubscriptExprParenCasts(const Expr *E)
{
  const Expr *NewE = E->IgnoreParenCasts();
  const ArraySubscriptExpr *ASE;
  while (true) {
    ASE = dyn_cast<ArraySubscriptExpr>(NewE);
    if (!ASE)
      break;
    NewE = ASE->getBase()->IgnoreParenCasts();
  }
  TransAssert(NewE && "NULL NewE!");
  return NewE;
}

const Expr *Transformation::getInitExprByIndex(IndexVector &Idxs,
                                               const InitListExpr *ILE)
{
  const InitListExpr *SubILE = ILE;
  const Expr *Exp = NULL;
  unsigned int Count = 0;
  for (IndexVector::const_reverse_iterator I = Idxs.rbegin(),
       E = Idxs.rend(); I != E; ++I) {
    Count++;
    unsigned int Idx;

    const Type *T = SubILE->getType().getTypePtr();
    if (T->isUnionType())
      Idx = 0;
    else
      Idx = (*I);

    // Incomplete initialization list
    if (Idx >= SubILE->getNumInits())
      return NULL;

    Exp = SubILE->getInit(Idx);
    TransAssert(Exp && "NULL Exp!");

    SubILE = dyn_cast<InitListExpr>(Exp);
    if (!SubILE)
      break;
  }

  TransAssert(Exp && "Exp cannot be NULL");
  TransAssert(Count == Idxs.size());
  return Exp;
}

const Expr *Transformation::getArrayBaseExprAndIdxs(
        const ArraySubscriptExpr *ASE, IndexVector &Idxs)
{
  const Expr *BaseE = NULL;
  while (ASE) {
    const Expr *IdxE = ASE->getIdx();
    unsigned int Idx = 0;
    llvm::APSInt Result;
    if (IdxE && IdxE->EvaluateAsInt(Result, *Context)) {
      // this will truncate a possible uint64 value to uint32 value
      Idx = (unsigned int)(*Result.getRawData());
    }
    BaseE = ASE->getBase()->IgnoreParenCasts();
    ASE = dyn_cast<ArraySubscriptExpr>(BaseE);
    Idxs.push_back(Idx);
  }
  return BaseE;
}

const Expr *Transformation::getInitExprFromBase(const Expr *BaseE,
                                                IndexVector &Idxs)
{
  TransAssert(BaseE && "Bad Array Base Expression!");
  const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(BaseE);
  if (!DRE)
    return NULL;

  const ValueDecl *OrigDecl = DRE->getDecl();
  const VarDecl *VD = dyn_cast<VarDecl>(OrigDecl);
  TransAssert(VD && "Bad VarDecl!");
  const Type * Ty = VD->getType().getTypePtr();
  if (Ty->isPointerType())
    return NULL;
  const Expr *InitE = VD->getAnyInitializer();

  if (!InitE)
    return NULL;
  // We don't have routine for CXXConstructExpr
  if (dyn_cast<CXXConstructExpr>(InitE))
    return NULL;

  const InitListExpr *ILE = dyn_cast<InitListExpr>(InitE);
  TransAssert(ILE && "Bad InitListExpr!");
  return getInitExprByIndex(Idxs, ILE);
}

const Expr *Transformation::getArraySubscriptElem(
              const ArraySubscriptExpr *ASE)
{
  IndexVector ArrayDims;
  const Expr *BaseE = getArrayBaseExprAndIdxs(ASE, ArrayDims);
  return getInitExprFromBase(BaseE, ArrayDims);
}

const Expr *Transformation::getMemberExprBaseExprAndIdxs(
        const MemberExpr *ME, IndexVector &Idxs)
{
  const Expr *BaseE = NULL;
  while (ME) {
    ValueDecl *VD = ME->getMemberDecl();
    FieldDecl *FD = dyn_cast<FieldDecl>(VD);
    TransAssert(FD && "Bad FD!\n");
    unsigned int Idx = FD->getFieldIndex();
    Idxs.push_back(Idx);
  
    BaseE = ME->getBase()->IgnoreParenCasts();
    const ArraySubscriptExpr *ASE = dyn_cast<ArraySubscriptExpr>(BaseE);
    if (ASE) {
      BaseE = getArrayBaseExprAndIdxs(ASE, Idxs);
      if (!BaseE)
        return NULL;
    }
    ME = dyn_cast<MemberExpr>(BaseE);
  }
  return BaseE;
}

bool Transformation::isCXXMemberExpr(const MemberExpr *ME)
{
  const ValueDecl *VD = ME->getMemberDecl();
  if (dyn_cast<CXXMethodDecl>(VD))
    return true;

  const FieldDecl *FD = dyn_cast<FieldDecl>(VD);
  TransAssert(FD && "Bad FieldDecl!");
  const CXXRecordDecl *CXXRD = dyn_cast<CXXRecordDecl>(FD->getParent());
  if (!CXXRD)
    return false;

  return !(CXXRD->isCLike());
}

const Expr *Transformation::getMemberExprElem(const MemberExpr *ME)
{
  if (isCXXMemberExpr(ME))
    return NULL;

  IndexVector Idxs;
  const Expr *BaseE = getMemberExprBaseExprAndIdxs(ME, Idxs);
  return getInitExprFromBase(BaseE, Idxs);
}

unsigned int Transformation::getArrayDimension(const ArrayType *ArrayTy)
{
  unsigned int Dim = 1;
  const Type *ArrayElemTy = ArrayTy->getElementType().getTypePtr();
  while (ArrayElemTy->isArrayType()) {
    const ArrayType *AT = dyn_cast<ArrayType>(ArrayElemTy);
    ArrayElemTy = AT->getElementType().getTypePtr();
    Dim++;
  }
  return Dim;
}

unsigned int Transformation::getArrayDimensionAndTypes(
               const ArrayType *ArrayTy,
               ArraySubTypeVector &TyVec)
{
  unsigned int Dim = 1;
  const Type *ArrayElemTy = ArrayTy->getElementType().getTypePtr();
  TyVec.push_back(ArrayTy);
  while (ArrayElemTy->isArrayType()) {
    const ArrayType *AT = dyn_cast<ArrayType>(ArrayElemTy);
    TyVec.push_back(AT);
    ArrayElemTy = AT->getElementType().getTypePtr();
    Dim++;
  }
  return Dim;
}

const Type *Transformation::getArrayBaseElemType(const ArrayType *ArrayTy)
{
  const Type *ArrayElemTy = ArrayTy->getElementType().getTypePtr();
  while (ArrayElemTy->isArrayType()) {
    const ArrayType *AT = dyn_cast<ArrayType>(ArrayElemTy);
    ArrayElemTy = AT->getElementType().getTypePtr();
  }
  TransAssert(ArrayElemTy && "Bad Array Element Type!");
  return ArrayElemTy;
}

unsigned int Transformation::getConstArraySize(
               const ConstantArrayType *CstArrayTy)
{
  unsigned int Sz;
  llvm::APInt Result = CstArrayTy->getSize();

  llvm::SmallString<8> IntStr;
  Result.toStringUnsigned(IntStr);

  std::stringstream TmpSS(IntStr.str());

  if (!(TmpSS >> Sz)) {
    TransAssert(0 && "Non-integer value!");
  }
  return Sz;
}

// This is a more complete implementation to deal with mixed
// array and structs/unions
const Expr *Transformation::getBaseExprAndIdxs(const Expr *E,
                                               IndexVector &Idxs)
{
  const Expr *BaseE = NULL;
  while (E) {
    E = E->IgnoreParenCasts();
    BaseE = E;

    Expr::StmtClass SC = E->getStmtClass();
    if (SC == Expr::MemberExprClass) {
      const MemberExpr *ME = dyn_cast<MemberExpr>(E);
      ValueDecl *VD = ME->getMemberDecl();
      FieldDecl *FD = dyn_cast<FieldDecl>(VD);
      TransAssert(FD && "Bad FD!\n");

      unsigned int Idx = FD->getFieldIndex();
      Idxs.push_back(Idx);
      E = ME->getBase();
    }
    else if (SC == Expr::ArraySubscriptExprClass) {
      const ArraySubscriptExpr *ASE = dyn_cast<ArraySubscriptExpr>(E);
      const Expr *IdxE = ASE->getIdx();
      unsigned int Idx = 0;
      llvm::APSInt Result;

      // If we cannot have an integeral index, use 0.
      if (IdxE && IdxE->EvaluateAsInt(Result, *Context)) {
        std::string IntStr = Result.toString(10);
        std::stringstream TmpSS(IntStr);
        if (!(TmpSS >> Idx))
          TransAssert(0 && "Non-integer value!");
      }
      Idxs.push_back(Idx);
      E = ASE->getBase();
    }
    else {
      break;
    }
  }
  return BaseE;
}

const Expr *Transformation::getBaseExprAndIdxExprs(
        const ArraySubscriptExpr *ASE, ExprVector &IdxExprs)
{
  const Expr *BaseE = NULL;
  while (ASE) {
    const Expr *IdxE = ASE->getIdx();
    IdxExprs.push_back(IdxE);
    BaseE = ASE->getBase()->IgnoreParenCasts();
    ASE = dyn_cast<ArraySubscriptExpr>(BaseE);
  }
  return BaseE;
}

const Type *Transformation::getBasePointerElemType(const Type *Ty)
{
  QualType QT = Ty->getPointeeType();;
  while (!QT.isNull()) {
    Ty = QT.getTypePtr();
    QT = Ty->getPointeeType();
  }
  TransAssert(Ty && "NULL Type Ptr!");
  return Ty;
}

int Transformation::getIndexAsInteger(const Expr *E)
{
  llvm::APSInt Result;
  int Idx;
  if (!E->EvaluateAsInt(Result, *Context))
    TransAssert(0 && "Failed to Evaluate index!");

  Idx = (int)(*Result.getRawData());
  return Idx;
}

const Type* Transformation::getBaseType(const Type *T)
{
  if (T->isPointerType() || T->isReferenceType())
    return getBaseType(T->getPointeeType().getTypePtr());
  else if (T->isRecordType())
    T = T->getAs<RecordType>();
  else if (T->isEnumeralType())
    T = T->getAs<EnumType>();
  else if (T->getTypeClass() == Type::Typedef)
    T = T->getAs<TypedefType>();
  else if (T->isArrayType())
    return getBaseType(T->castAsArrayTypeUnsafe()->
        getElementType().getTypePtr());

  return T;
}

// Lookup a function decl from a top-level DeclContext
// It's slow...
const FunctionDecl *Transformation::lookupFunctionDeclInGlobal(
        DeclarationName &DName, const DeclContext *Ctx)
{
  DeclContext::lookup_const_result Result = Ctx->lookup(DName);
  for (DeclContext::lookup_const_iterator I = Result.begin(), E = Result.end();
       I != E; ++I) {
    if (const FunctionDecl *FD = dyn_cast<FunctionDecl>(*I)) {
      return FD;
    }

    const FunctionTemplateDecl *TD = NULL;
    if (const UsingShadowDecl *USD = dyn_cast<UsingShadowDecl>(*I)) {
      TD = dyn_cast<FunctionTemplateDecl>(USD->getTargetDecl());
    }
    else {
      TD = dyn_cast<FunctionTemplateDecl>(*I);
    }
    if (TD)
      return TD->getTemplatedDecl();
  }

  for (DeclContext::decl_iterator I = Ctx->decls_begin(),
       E = Ctx->decls_end(); I != E; ++I) {
    if (const ClassTemplateDecl *ClassTemplate = 
        dyn_cast<ClassTemplateDecl>(*I)) {
      const CXXRecordDecl *CXXRD = ClassTemplate->getTemplatedDecl();
      if (const FunctionDecl *FD = lookupFunctionDeclInGlobal(DName, CXXRD)) {
        return FD;
      }
    }

    const DeclContext *SubCtx = dyn_cast<DeclContext>(*I);
    if (!SubCtx || dyn_cast<LinkageSpecDecl>(SubCtx))
      continue;
    
    if (const FunctionDecl *FD = lookupFunctionDeclInGlobal(DName, SubCtx)) {
      return FD;
    }
  }

  return NULL;
}

const FunctionDecl *Transformation::lookupFunctionDeclFromBases(
        DeclarationName &DName, const CXXRecordDecl *CXXRD)
{
  for (CXXRecordDecl::base_class_const_iterator I =
       CXXRD->bases_begin(), E = CXXRD->bases_end(); I != E; ++I) {

    const CXXBaseSpecifier *BS = I;
    const Type *Ty = BS->getType().getTypePtr();
    const CXXRecordDecl *Base = getBaseDeclFromType(Ty);
    TransAssert(Base && "NULL Base!");
    const CXXRecordDecl *BaseDef = Base->getDefinition();
    if (!BaseDef)
      continue;
    if (const FunctionDecl *FD = lookupFunctionDecl(DName, BaseDef))
      return FD;
  }
  return NULL;
}

const FunctionDecl *Transformation::lookupFunctionDeclFromCtx(
        DeclarationName &DName, const DeclContext *Ctx)
{
  DeclContext::lookup_const_result Result = Ctx->lookup(DName);
  for (DeclContext::lookup_const_iterator I = Result.begin(), E = Result.end();
       I != E; ++I) {
    if (const FunctionDecl *FD = dyn_cast<FunctionDecl>(*I)) {
      return FD;
    }

    const FunctionTemplateDecl *TD = NULL;
    if (const UsingShadowDecl *USD = dyn_cast<UsingShadowDecl>(*I)) {
      TD = dyn_cast<FunctionTemplateDecl>(USD->getTargetDecl());
    }
    else { 
      TD = dyn_cast<FunctionTemplateDecl>(*I);
    }
    if (TD)
      return TD->getTemplatedDecl();

    if (const UnresolvedUsingValueDecl *UUD = 
        dyn_cast<UnresolvedUsingValueDecl>(*I)) {
      const NestedNameSpecifier *NNS = UUD->getQualifier();
      const DeclContext *Ctx = getDeclContextFromSpecifier(NNS);
      if (!Ctx)
        continue;
      if (const FunctionDecl *FD = lookupFunctionDecl(DName, Ctx))
        return FD;
    }
  }
  return NULL;
}

const FunctionDecl *Transformation::lookupFunctionDecl(
        DeclarationName &DName, const DeclContext *Ctx)
{
  if (const FunctionDecl *FD = lookupFunctionDeclFromCtx(DName, Ctx))
    return FD;
 
  // lookup base classes:
  // this would be slow and may re-visit some Ctx. 
  // Probably we should cache those Ctx(s) which have been visited
  // to speedup lookup
  if (Ctx->isRecord()) {
    const RecordDecl *RD = dyn_cast<RecordDecl>(Ctx);
    if (const CXXRecordDecl *CXXRD = dyn_cast<CXXRecordDecl>(RD)) {
      if (const FunctionDecl *FD = lookupFunctionDeclFromBases(DName, CXXRD))
        return FD;
    }
  }

  for (DeclContext::udir_iterator I = Ctx->using_directives_begin(),
       E = Ctx->using_directives_end(); I != E; ++I) {
    const NamespaceDecl *ND = (*I)->getNominatedNamespace();
    // avoid infinite recursion
    if (ND->getLookupParent() == Ctx)
      return NULL;
    if (const FunctionDecl *FD = lookupFunctionDecl(DName, ND))
      return FD;
  }

  const DeclContext *ParentCtx = Ctx->getLookupParent();
  if (!ParentCtx || dyn_cast<LinkageSpecDecl>(ParentCtx))
    return NULL;

  return lookupFunctionDecl(DName, ParentCtx);
}

const DeclContext *Transformation::getDeclContextFromSpecifier(
        const NestedNameSpecifier *NNS)
{
  for (; NNS; NNS = NNS->getPrefix()) {
    NestedNameSpecifier::SpecifierKind Kind = NNS->getKind();

    switch (Kind) {
      case NestedNameSpecifier::Namespace: {
        return NNS->getAsNamespace()->getCanonicalDecl();
      }
      case NestedNameSpecifier::NamespaceAlias: {
        const NamespaceAliasDecl *NAD = NNS->getAsNamespaceAlias();
        return NAD->getNamespace()->getCanonicalDecl();
      }
      case NestedNameSpecifier::TypeSpec: // Fall-through
      case NestedNameSpecifier::TypeSpecWithTemplate: {
        const Type *Ty = NNS->getAsType();
        if (const RecordType *RT = Ty->getAs<RecordType>())
          return RT->getDecl();
        if (const TypedefType *TT = Ty->getAs<TypedefType>()) {
          const TypedefNameDecl *TypeDecl = TT->getDecl();
          const Type *UnderlyingTy = TypeDecl->getUnderlyingType().getTypePtr();
          if (const RecordType *RT = UnderlyingTy->getAs<RecordType>())
            return RT->getDecl();
          if (const TemplateSpecializationType *TST =
              UnderlyingTy->getAs<TemplateSpecializationType>()) {
            return getBaseDeclFromTemplateSpecializationType(TST);
          }
        }
        break;
      }
      default:
        break;
    }
  }
  return NULL;
}

bool Transformation::isSpecialRecordDecl(const RecordDecl *RD)
{
  std::string Name = RD->getNameAsString();
  return (Name == "__va_list_tag");
}

const CXXRecordDecl *Transformation::getBaseDeclFromTemplateSpecializationType(
        const TemplateSpecializationType *TSTy)
{
  TemplateName TplName = TSTy->getTemplateName();
  TemplateDecl *TplD = TplName.getAsTemplateDecl();
  TransAssert(TplD && "Invalid TemplateDecl!");
  NamedDecl *ND = TplD->getTemplatedDecl();
  TransAssert(ND && "Invalid NamedDecl!");

  if (TypedefNameDecl *TdefD = dyn_cast<TypedefNameDecl>(ND)) {
    const Type *UnderlyingTy = TdefD->getUnderlyingType().getTypePtr();
    const CXXRecordDecl *CXXRD = getBaseDeclFromType(UnderlyingTy);
    TransAssert(CXXRD && "Invalid CXXRD from TypedefNameDecl!");
    return CXXRD;
  }

  const CXXRecordDecl *CXXRD = dyn_cast<CXXRecordDecl>(ND);
  TransAssert(CXXRD && "Invalid CXXRD!");
  if (CXXRD->hasDefinition())
    return CXXRD->getDefinition();
  else
    return CXXRD;
}

// This function could return NULL
const CXXRecordDecl *Transformation::getBaseDeclFromType(const Type *Ty)
{
  const CXXRecordDecl *Base = NULL;
  Type::TypeClass TyClass = Ty->getTypeClass();

  switch (TyClass) {
  case Type::TemplateSpecialization: {
    const TemplateSpecializationType *TSTy = 
      dyn_cast<TemplateSpecializationType>(Ty);
    Base = getBaseDeclFromTemplateSpecializationType(TSTy);
    TransAssert(Base && "Bad base class type!");
    return Base;
  }

  case Type::DependentTemplateSpecialization: {
    return NULL;
  }

  case Type::Elaborated: {
    const ElaboratedType *ETy = dyn_cast<ElaboratedType>(Ty);
    const Type *NamedT = ETy->getNamedType().getTypePtr();
    if ( const TemplateSpecializationType *TSTy = 
         dyn_cast<TemplateSpecializationType>(NamedT) ) {
      Base = getBaseDeclFromTemplateSpecializationType(TSTy);
    }
    else if ( const TypedefType * Ty = dyn_cast<TypedefType>(NamedT) ){
      Base = getBaseDeclFromType(Ty);
    }
    else {
      Base = ETy->getAsCXXRecordDecl();
    }
    TransAssert(Base && "Bad base class type from ElaboratedType!");
    return Base;
  }

  case Type::DependentName: {
    // It's not always the case that we could resolve a dependent name type.
    // For example, 
    //   template<typename T1, typename T2>
    //   struct AAA { typedef T2 new_type; };
    //   template<typename T3>
    //   struct BBB : public AAA<int, T3>::new_type { };
    // In the above code, we can't figure out what new_type refers to 
    // until BBB is instantiated
    // Due to this reason, simply return NULL from here.
    return NULL;
  }

  case Type::Typedef: {
    const TypedefType *TdefTy = dyn_cast<TypedefType>(Ty);
    const TypedefNameDecl *TdefD = TdefTy->getDecl();
    const Type *UnderlyingTy = TdefD->getUnderlyingType().getTypePtr();
    if (const TemplateSpecializationType *TSTy = 
         dyn_cast<TemplateSpecializationType>(UnderlyingTy) ) {
      Base = getBaseDeclFromTemplateSpecializationType(TSTy);
    }
    else if (const ElaboratedType *ETy = 
             dyn_cast<ElaboratedType>(UnderlyingTy)) {
      Base = getBaseDeclFromType(ETy);
    }
    else if (dyn_cast<DependentNameType>(UnderlyingTy)) {
      return NULL;
    }
    else if (dyn_cast<TemplateTypeParmType>(UnderlyingTy)) {
      return NULL;
    }
    else {
      Base = UnderlyingTy->getAsCXXRecordDecl();
    }
    TransAssert(Base && "Bad base class type from Typedef!");
    return Base;
  }

  case Type::TemplateTypeParm: {
    // Yet another case we might not know the base class, e.g.,
    // template<typename T1> 
    // class AAA {
    //   struct BBB : T1 {};
    // };
    return NULL;
  }

  case Type::Builtin: {
    return NULL;
  }

  default:
    Base = Ty->getAsCXXRecordDecl();
    TransAssert(Base && "Bad base class type!");

    // getAsCXXRecordDecl could return a ClassTemplateSpecializationDecl.
    // For example:
    //   template <class T> class AAA { };
    //   typedef AAA<int> BBB;
    //   class CCC : BBB { };
    // In the above code, BBB is of type ClassTemplateSpecializationDecl
    if (const ClassTemplateSpecializationDecl *CTSDecl =
        dyn_cast<ClassTemplateSpecializationDecl>(Base)) {
      Base = CTSDecl->getSpecializedTemplate()->getTemplatedDecl();
      TransAssert(Base && 
                  "Bad base decl from ClassTemplateSpecializationDecl!");
    }
  }

  return Base;
}

bool Transformation::isParameterPack(const NamedDecl *ND)
{
  if (const NonTypeTemplateParmDecl *NonTypeD = 
      dyn_cast<NonTypeTemplateParmDecl>(ND)) {
    return NonTypeD->isParameterPack();
  }
  else if (const TemplateTypeParmDecl *TypeD = 
             dyn_cast<TemplateTypeParmDecl>(ND)) {
    return TypeD->isParameterPack();
  }
  else if (const TemplateTemplateParmDecl *TmplD = 
             dyn_cast<TemplateTemplateParmDecl>(ND)) {
    return TmplD->isParameterPack();
  }
  else {
    TransAssert(0 && "Unknown template parameter type!");
    return false;
  }
}

unsigned Transformation::getNumCtorWrittenInitializers(
           const CXXConstructorDecl &Ctor)
{
  unsigned Num = 0;
  for (CXXConstructorDecl::init_const_iterator I = Ctor.init_begin(),
       E = Ctor.init_end(); I != E; ++I) {
    if ((*I)->isWritten())
      Num++;
  }
  return Num;
}

Transformation::~Transformation(void)
{
  RewriteUtils::Finalize();
}

