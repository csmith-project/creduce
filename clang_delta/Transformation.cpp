//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

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
      std::string IntStr = Result.toString(10);
      std::stringstream TmpSS(IntStr);
      if (!(TmpSS >> Idx))
        TransAssert(0 && "Non-integer value!");
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
  TransAssert(DRE && "Bad DeclRefExpr!");
  const ValueDecl *OrigDecl = DRE->getDecl();
  const VarDecl *VD = dyn_cast<VarDecl>(OrigDecl);
  TransAssert(VD && "Bad VarDecl!");
  const Expr *InitE = VD->getAnyInitializer();

  if (!InitE)
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

  std::string IntStr = Result.toString(10);
  std::stringstream TmpSS(IntStr);
  if (!(TmpSS >> Idx))
    TransAssert(0 && "Non-integer value!");

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

Transformation::~Transformation(void)
{
  RewriteUtils::Finalize();
}

