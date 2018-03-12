//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2014, 2015, 2016, 2017 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "EmptyStructToInt.h"

#include "clang/Basic/SourceManager.h"

#include "clang/Lex/Lexer.h"
#include "clang/AST/RecordLayout.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "llvm/ADT/StringRef.h"
#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Replace an empty struct with type of int. A struct is defined to be empty if \
it: \
  * does not have any field; \n\
  * does not have any base class; \n\
  * is not a base class of another class; \n\
  * is not described by any template; \n\
  * has only one unreferenced field; \n\
  * doesn't have self pointer reference\n";

static RegisterTransformation<EmptyStructToInt>
         Trans("empty-struct-to-int", DescriptionMsg);

class EmptyStructToIntASTVisitor : public 
  RecursiveASTVisitor<EmptyStructToIntASTVisitor> {

public:
  explicit EmptyStructToIntASTVisitor(EmptyStructToInt *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitRecordDecl(RecordDecl *RD);

  bool VisitCXXRecordDecl(CXXRecordDecl *CXXRD);

private:
  EmptyStructToInt *ConsumerInstance;
};

class EmptyStructToIntRewriteVisitor : public 
  RecursiveASTVisitor<EmptyStructToIntRewriteVisitor> {

public:
  explicit EmptyStructToIntRewriteVisitor(EmptyStructToInt *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitRecordTypeLoc(RecordTypeLoc RTLoc);

  bool VisitElaboratedTypeLoc(ElaboratedTypeLoc Loc);

  bool VisitRecordDecl(RecordDecl *RD);

  bool VisitVarDecl(VarDecl *VD);

private:
  EmptyStructToInt *ConsumerInstance;
};

bool EmptyStructToIntASTVisitor::VisitRecordDecl(RecordDecl *RD)
{
  if (ConsumerInstance->isInIncludedFile(RD) ||
      !ConsumerInstance->isValidRecordDecl(RD))
    return true;
 
  const RecordDecl *CanonicalRD = dyn_cast<RecordDecl>(RD->getCanonicalDecl());
  if (ConsumerInstance->VisitedRecordDecls.count(CanonicalRD))
    return true;

  ConsumerInstance->VisitedRecordDecls.insert(CanonicalRD);
  return true;
}

bool EmptyStructToIntASTVisitor::VisitCXXRecordDecl(CXXRecordDecl *CXXRD)
{
  const CXXRecordDecl *CanonicalRD = CXXRD->getCanonicalDecl();
  if (ConsumerInstance->VisitedRecordDecls.count(CanonicalRD))
    return true;

  if (!CanonicalRD->hasDefinition())
    return true;

  for (CXXRecordDecl::base_class_const_iterator I = 
       CanonicalRD->bases_begin(), E = CanonicalRD->bases_end(); I != E; ++I) {
    const CXXBaseSpecifier *BS = I;
    const Type *Ty = BS->getType().getTypePtr();
    const CXXRecordDecl *Base = ConsumerInstance->getBaseDeclFromType(Ty);
    if (Base)
      ConsumerInstance->BaseClassDecls.insert(Base->getCanonicalDecl());
  }
  return true;
}

bool EmptyStructToIntRewriteVisitor::VisitRecordTypeLoc(RecordTypeLoc RTLoc)
{
  const RecordDecl *RD = RTLoc.getDecl();

  if (RD->getCanonicalDecl() == ConsumerInstance->TheRecordDecl) {
    SourceLocation LocStart = RTLoc.getLocStart();
    void *LocPtr = LocStart.getPtrEncoding();
    if (ConsumerInstance->VisitedLocs.count(LocPtr))
      return true;
    ConsumerInstance->VisitedLocs.insert(LocPtr);

    // handle a special case -
    // struct S1 {
    //   struct { } S;
    // };
    const IdentifierInfo *TypeId = RTLoc.getType().getBaseTypeIdentifier();
    if (!TypeId)
      return true;
    ConsumerInstance->RewriteHelper->replaceRecordType(RTLoc, "int");
    ConsumerInstance->Rewritten = true;
  }
  return true;
}

bool EmptyStructToIntRewriteVisitor::VisitElaboratedTypeLoc(
       ElaboratedTypeLoc Loc)
{
  const ElaboratedType *ETy = dyn_cast<ElaboratedType>(Loc.getTypePtr());
  const Type *NamedTy = ETy->getNamedType().getTypePtr();
  const RecordType *RDTy = NamedTy->getAs<RecordType>();
  if (!RDTy)
    return true;

  const RecordDecl *RD = RDTy->getDecl();
  TransAssert(RD && "NULL RecordDecl!");
  if (RD->getCanonicalDecl() != ConsumerInstance->TheRecordDecl) {
    return true;
  }

  SourceLocation StartLoc = Loc.getLocStart();
  if (StartLoc.isInvalid())
    return true;
  TypeLoc TyLoc = Loc.getNamedTypeLoc();
  SourceLocation EndLoc = TyLoc.getLocStart();
  if (EndLoc.isInvalid())
    return true;
  EndLoc = EndLoc.getLocWithOffset(-1);
  const char *StartBuf = 
    ConsumerInstance->SrcManager->getCharacterData(StartLoc);
  const char *EndBuf = ConsumerInstance->SrcManager->getCharacterData(EndLoc);
  ConsumerInstance->Rewritten = true;
  // It's possible, e.g., 
  // struct S1 {
  //   struct { } S;
  // };
  // Clang will translate struct { } S to
  // struct {
  // };
  //  struct <anonymous struct ...> S;
  // the last declaration is injected by clang.
  // We need to omit it.
  if (StartBuf > EndBuf) {
    SourceLocation KeywordLoc = Loc.getElaboratedKeywordLoc();
    const llvm::StringRef Keyword = 
      TypeWithKeyword::getKeywordName(ETy->getKeyword());
    ConsumerInstance->TheRewriter.ReplaceText(KeywordLoc, 
                                              Keyword.size(), "int");
    return true;
  }
  
  ConsumerInstance->TheRewriter.RemoveText(SourceRange(StartLoc, EndLoc));
  return true;
}

bool EmptyStructToIntRewriteVisitor::VisitRecordDecl(RecordDecl *RD)
{
  //if (ConsumerInstance->isSpecialRecordDecl(RD))
  //  return true;

  // Skip forward declarations
  const RecordDecl *RDDef = RD->getDefinition();
  if (!RDDef)
    return true;

  // Skip the struct which will be deleted
  // It is already in the list
  if (RD->getCanonicalDecl() == ConsumerInstance->TheRecordDecl) {
    return true;
  }

  unsigned Idx = 0;
  for (RecordDecl::field_iterator I = RDDef->field_begin(),
       E = RDDef->field_end(); I != E; ++I) {
    const FieldDecl *FD = (*I);
    const Type *FDTy = FD->getType().getTypePtr();
    const RecordDecl *BaseRD = ConsumerInstance->getBaseRecordDef(FDTy);

    // Handle all fields of struct type
    if (BaseRD)
      ConsumerInstance->handleOneRecordDecl(RDDef, BaseRD, FD, Idx);
    Idx++;
  }
  return true;
}

bool EmptyStructToIntRewriteVisitor::VisitVarDecl(VarDecl *VD)
{
  if (!VD->hasInit()) {
    return true;
  }

  ConsumerInstance->handleOneVarDecl(VD);
  return true;
}

void EmptyStructToInt::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new EmptyStructToIntASTVisitor(this);
  RewriteVisitor = new EmptyStructToIntRewriteVisitor(this);
}

void EmptyStructToInt::HandleTranslationUnit(ASTContext &Ctx)
{
  CollectionVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
  doAnalysis();

  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);
  removeRecordDecls();
  RewriteVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
  
  // sanity check that we actually
  // have done some text modifications. 
  // It could be false due to invalid code being transformed.
  if (!Rewritten) {
    TransError = TransNoTextModificationError;
    return;
  }
  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void EmptyStructToInt::handleOneRecordDecl(const RecordDecl *RD,
                                           const RecordDecl *BaseRD,
                                           const FieldDecl *FD,
                                           unsigned int Idx)
{
  // Skip field if it is not of the type of one of
  // the structs that are affected by the change
  // Works recursively through chained structs
  if (!RecordDeclToField[BaseRD] && BaseRD != TheRecordDecl) {
    return;
  }

  IndexVector *NewIdxVec = RecordDeclToField[RD];

  if (!NewIdxVec) {
    NewIdxVec = new IndexVector();
    RecordDeclToField[RD] = NewIdxVec;
  }

  NewIdxVec->push_back(Idx);
}

void EmptyStructToInt::handleOneVarDecl(const VarDecl *VD)
{
  const Type *Ty = VD->getType().getTypePtr();
  const RecordDecl *RD = getBaseRecordDef(Ty);

  // Skip all variables which are not of struct type
  if (!RD) {
    return;
  }

  IndexVector *IdxVec = RecordDeclToField[RD];

  // Skip variables for which the struct is not changed
  if (!IdxVec && RD != TheRecordDecl) {
    return;
  }

  ExprVector InitExprs;

  getInitExprs(Ty, VD->getInit(), IdxVec, InitExprs);

  for (ExprVector::iterator I = InitExprs.begin(), E = InitExprs.end(); I != E; ++I) {
      RewriteHelper->replaceExpr(*I, "0");
  }
}

void EmptyStructToInt::doAnalysis(void)
{
  for (RecordDeclSet::const_iterator I = VisitedRecordDecls.begin(),
       E = VisitedRecordDecls.end(); I != E; ++I) {
    const RecordDecl *RD = (*I);
    if (const CXXRecordDecl *CXXRD = dyn_cast<CXXRecordDecl>(RD)) {
      if (BaseClassDecls.count(CXXRD->getCanonicalDecl()))
        continue;
    }
    ValidInstanceNum++;
    if (ValidInstanceNum == TransformationCounter)
      TheRecordDecl = RD;
  }
}

// ISSUE: we will have bad transformation for the case below:
// typedef struct S;
// S *s;
// ==>
// typedef
// int *s;
// This is bad because we don't catch the implicit declaration of struct S.
// But hopefully peephole pass will remove the keyword typedef,
// then we will be fine.
void EmptyStructToInt::removeRecordDecls(void)
{
  for (RecordDecl::redecl_iterator I = TheRecordDecl->redecls_begin(),
      E = TheRecordDecl->redecls_end(); I != E; ++I) {

    const RecordDecl *RD = dyn_cast<RecordDecl>(*I);
    SourceRange Range = RD->getSourceRange();
    SourceLocation LocEnd = Range.getEnd();
    SourceLocation SemiLoc = 
      Lexer::findLocationAfterToken(LocEnd, 
                                    tok::semi,
                                    *SrcManager,
                                    Context->getLangOpts(),
                                    /*SkipTrailingWhitespaceAndNewLine=*/true);
    // handle cases such as 
    // struct S {} s;
    if (SemiLoc.isInvalid()) {
      if (!RD->isThisDeclarationADefinition())
        return;
      SourceLocation RBLoc = RD->getBraceRange().getEnd();
      if (RBLoc.isInvalid())
        return;
      RewriteHelper->removeTextFromLeftAt(SourceRange(RBLoc, RBLoc),
                                          '{', RBLoc);
      Rewritten = true;
    }
    else {
      LocEnd = RewriteHelper->getEndLocationUntil(Range, ';');
      TheRewriter.RemoveText(SourceRange(Range.getBegin(), LocEnd));
      Rewritten = true;
    }
  }
}

bool EmptyStructToInt::pointToSelf(const FieldDecl *FD)
{
  const Type *Ty = FD->getType().getTypePtr();
  if (!Ty->isPointerType())
    return false;
  const Type *PointeeTy = getBasePointerElemType(Ty);
  if (TransformationManager::isCXXLangOpt()) {
    const CXXRecordDecl *Base = getBaseDeclFromType(Ty);
    if (!Base)
      return false;
    const CXXRecordDecl *Parent = dyn_cast<CXXRecordDecl>(FD->getParent());
    TransAssert(Parent && "Invalid Parent!");
    return (Parent->getCanonicalDecl() == Base->getCanonicalDecl());
  }

  const RecordType *RT = PointeeTy->getAs<RecordType>();
  if (!RT)
    return false;
  const RecordDecl *RD = RT->getDecl();
  const RecordDecl *Parent = FD->getParent();
  return (Parent->getCanonicalDecl() == RD->getCanonicalDecl());
}

bool EmptyStructToInt::isValidRecordDecl(const RecordDecl *RD)
{
  const CXXRecordDecl *CXXRD = dyn_cast<CXXRecordDecl>(RD);
  if (!CXXRD) {
    const RecordDecl *Def = RD->getDefinition();
    if (!Def) {
      return true;
    }
    else if (Def->field_empty()) {
      return true;
    }
    else {
      // skip invalid decl, which causes clang assertion errors
      if (Def->isInvalidDecl())
        return false;
      // handle another special case where a struct has an unreferenced
      // field. In some cases, we cannot simply remove this field
      // because an empty struct would make a bug disappear.
      const ASTRecordLayout &Info = Context->getASTRecordLayout(Def);
      unsigned Count = Info.getFieldCount();
      if (Count != 1)
        return false;
      const FieldDecl *FD = *(Def->field_begin());
      TransAssert(FD && "Invalid FieldDecl");
      // skip case such as 
      // struct S { struct S *p; };
      if (pointToSelf(FD))
        return false;
      return !FD->isReferenced();
    }
  }

  if (dyn_cast<ClassTemplateSpecializationDecl>(CXXRD) ||
      CXXRD->getDescribedClassTemplate() ||
      CXXRD->getInstantiatedFromMemberClass())
    return false;

  // It's possible that the described template does not
  // have definition, so we test hasDefinition after the
  // above `if' guard
  const CXXRecordDecl *CXXDef = CXXRD->getDefinition();
  if (!CXXDef)
    return true;

  if(CXXDef->getNumBases())
    return false;

  const DeclContext *Ctx = dyn_cast<DeclContext>(CXXDef);
  TransAssert(Ctx && "Invalid DeclContext!");
  int count = 0;
  for (DeclContext::decl_iterator I = Ctx->decls_begin(),
       E = Ctx->decls_end(); I != E; ++I) {
    if (!(*I)->isImplicit()) {
      if ((*I)->isReferenced())
        return false;
      ++count;
    }
  }

  if (count > 1)
    return false;

  return true;
}

const RecordDecl *EmptyStructToInt::getBaseRecordDef(const Type *Ty)
{
  const ArrayType *ArrayTy = dyn_cast<ArrayType>(Ty);
  if (ArrayTy) {
    Ty = getArrayBaseElemType(ArrayTy);
  }

  if (!Ty->isStructureType())
    return NULL;

  const RecordType *RT = Ty->getAsStructureType();
  return RT->getDecl()->getDefinition();
}

void EmptyStructToInt::getInitExprs(const Type *Ty, 
                                    const Expr *E,
                                    const IndexVector *IdxVec,
                                    ExprVector &InitExprs)
{
  const ArrayType *ArrayTy = dyn_cast<ArrayType>(Ty);
  if (ArrayTy) {
    if (isa<ImplicitValueInitExpr>(E) || isa<CXXConstructExpr>(E))
      return;
    const InitListExpr *ILE = dyn_cast<InitListExpr>(E);
    TransAssert(ILE && "Invalid array initializer!");
    unsigned int NumInits = ILE->getNumInits();
    Ty = ArrayTy->getElementType().getTypePtr();
    
    for (unsigned I = 0; I < NumInits; ++I) {
      const Expr *Init = ILE->getInit(I);
      getInitExprs(Ty, Init, IdxVec, InitExprs);
    }
    return;
  }
 
  const InitListExpr *ILE = dyn_cast<InitListExpr>(E);
  if (!ILE)
    return;

  const RecordType *RT = NULL;
  if (Ty->isUnionType()) {
    RT = Ty->getAsUnionType();
  }
  else if (Ty->isStructureType()) {
    RT = Ty->getAsStructureType();
  }
  else {
    TransAssert(0 && "Bad RecordType!");
  }

  const RecordDecl *RD = RT->getDecl();

  if (RD->getCanonicalDecl() == TheRecordDecl) {
    InitExprs.push_back(E);
  }
  else {
    for (IndexVector::const_iterator FI = IdxVec->begin(), FE = IdxVec->end();
         FI != FE; ++FI) {
      const FieldDecl *FD = getFieldDeclByIdx(RD, (*FI));
      TransAssert(FD && "NULL FieldDecl!");

      Ty = FD->getType().getTypePtr();

      const RecordDecl *BaseRD = getBaseRecordDef(Ty);
      unsigned int InitListIdx;

      if (BaseRD->isUnion()) {
        InitListIdx = 0;
      }
      else {
        InitListIdx = (*FI);
      }

      if (InitListIdx >= ILE->getNumInits()) {
        return;
      }

      getInitExprs(Ty, ILE->getInit(InitListIdx),
                   RecordDeclToField[BaseRD], InitExprs);
    }
  }
}

const FieldDecl *EmptyStructToInt::getFieldDeclByIdx(
                   const RecordDecl *RD, unsigned int Idx)
{
  unsigned I = 0;
  for (RecordDecl::field_iterator RI = RD->field_begin(),
       RE = RD->field_end(); RI != RE; ++RI, ++I) {
    if (I == Idx)
      return (*RI);
  }
  return NULL;
}

EmptyStructToInt::~EmptyStructToInt(void)
{
  delete CollectionVisitor;
  delete RewriteVisitor;

  for (RecordDeclToFieldIdxVectorMap::iterator I = RecordDeclToField.begin(),
       E = RecordDeclToField.end(); I != E; ++I) {
    delete (*I).second;
  }
}

