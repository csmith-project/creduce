//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "RemoveUnusedStructField.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Remove unreferenced struct fields. This pass also removes the corresponding \
intialization expression for a variable declared as the struct under \
transformation. Currenttly this pass doesn't handle nested struct \
definition well. \n";

static RegisterTransformation<RemoveUnusedStructField>
         Trans("remove-unused-field", DescriptionMsg);

class RemoveUnusedStructFieldVisitor : public
  RecursiveASTVisitor<RemoveUnusedStructFieldVisitor> {
public:

  explicit RemoveUnusedStructFieldVisitor(RemoveUnusedStructField *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitFieldDecl(FieldDecl *FD);

private:

  RemoveUnusedStructField *ConsumerInstance;
};

class RemoveUnusedStructFieldRewriteVisitor : public
  RecursiveASTVisitor<RemoveUnusedStructFieldRewriteVisitor> {
public:

  explicit RemoveUnusedStructFieldRewriteVisitor(
             RemoveUnusedStructField *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitRecordDecl(RecordDecl *RD);

  bool VisitVarDecl(VarDecl *VD);

private:

  RemoveUnusedStructField *ConsumerInstance;
};

bool RemoveUnusedStructFieldVisitor::VisitFieldDecl(FieldDecl *FD)
{
  const RecordDecl *RD = FD->getParent();
  if (FD->isReferenced() || !RD->isStruct() ||
      ConsumerInstance->isSpecialRecordDecl(RD))
    return true;

  ConsumerInstance->ValidInstanceNum++;
  if (ConsumerInstance->ValidInstanceNum == 
      ConsumerInstance->TransformationCounter) {
    ConsumerInstance->setBaseLine(RD, FD);
  }

  return true;
}

bool RemoveUnusedStructFieldRewriteVisitor::VisitRecordDecl(RecordDecl *RD)
{
  if (ConsumerInstance->isSpecialRecordDecl(RD))
    return true;

  const RecordDecl *RDDef = RD->getDefinition();
  if (!RDDef)
    return true;
  if (ConsumerInstance->RecordDeclToField[RD])
    return true;

  unsigned Idx = 0;
  for (RecordDecl::field_iterator I = RDDef->field_begin(),
       E = RDDef->field_end(); I != E; ++I) {
    const FieldDecl *FD = &(*I);
    const Type *FDTy = FD->getType().getTypePtr();
    const RecordDecl *BaseRD = ConsumerInstance->getBaseRecordDef(FDTy);
    if (BaseRD)
      ConsumerInstance->handleOneRecordDecl(RDDef, BaseRD, FD, Idx);
    Idx++;
  }
  return true;
}

bool RemoveUnusedStructFieldRewriteVisitor::VisitVarDecl(VarDecl *VD)
{
  if (!VD->hasInit())
    return true;

  ConsumerInstance->handleOneVarDecl(VD);
  return true;
}

void RemoveUnusedStructField::Initialize(ASTContext &context)
{
  Transformation::Initialize(context);
  CollectionVisitor = new RemoveUnusedStructFieldVisitor(this);
  RewriteVisitor = new RemoveUnusedStructFieldRewriteVisitor(this);
}

void RemoveUnusedStructField::HandleTranslationUnit(ASTContext &Ctx)
{
  CollectionVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  TransAssert(TheRecordDecl && "NULL TheRecordDecl!");
  TransAssert(TheFieldDecl && "NULL TheFunctionDecl!");
  
  RewriteVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
  removeFieldDecl();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void RemoveUnusedStructField::setBaseLine(const RecordDecl *RD, 
                                          const FieldDecl *FD)
{
  TheRecordDecl = RD;
  TheFieldDecl = FD;
    
  IndexVector *IdxVec = new IndexVector();
  unsigned int Idx = FD->getFieldIndex();
  IdxVec->push_back(Idx);
  RecordDeclToField[RD] = IdxVec;
  FieldToIdxVector[FD] = IdxVec;

  // IsLastField = (FD->getNextDeclInContext() == NULL);
  RecordDecl::field_iterator I = RD->field_begin();
  IsFirstField = (FD == &(*I));
  RecordDecl::field_iterator E = RD->field_end();

  for (; I != E; ++I) {
    NumFields++;
  }
}
  
void RemoveUnusedStructField::handleOneRecordDecl(const RecordDecl *RD,
                                                  const RecordDecl *BaseRD,
                                                  const FieldDecl *FD,
                                                  unsigned int Idx)
{
  IndexVector *BaseIdxVec = RecordDeclToField[BaseRD];
  if (!BaseIdxVec)
    return;

  IndexVector *NewIdxVec = RecordDeclToField[RD];
  if (!NewIdxVec) {
    NewIdxVec = new IndexVector();
    RecordDeclToField[RD] = NewIdxVec;
  }
  NewIdxVec->push_back(Idx);
  FieldToIdxVector[FD] = BaseIdxVec;
}

void RemoveUnusedStructField::handleOneVarDecl(const VarDecl *VD)
{
  const Type *Ty = VD->getType().getTypePtr();
  const RecordDecl *RD = getBaseRecordDef(Ty);
  if (!RD)
    return;

  IndexVector *IdxVec = RecordDeclToField[RD];
  if (!IdxVec)
    return;

  const Expr *InitE = VD->getInit();
  TransAssert(InitE && "Need initializer!");

  ExprVector InitExprs;

  getInitExprs(Ty, InitE, IdxVec, InitExprs);

  for (ExprVector::iterator I = InitExprs.begin(),
       E = InitExprs.end(); I != E; ++I) {
    removeOneInitExpr(*I);
  }
}

const FieldDecl *RemoveUnusedStructField::getFieldDeclByIdx(
                   const RecordDecl *RD, unsigned int Idx)
{
  unsigned I = 0;
  for (RecordDecl::field_iterator RI = RD->field_begin(),
       RE = RD->field_end(); RI != RE; ++RI, ++I) {
    if (I == Idx)
      return &(*RI);
  }
  return NULL;
}

void RemoveUnusedStructField::getInitExprs(const Type *Ty, 
                                           const Expr *E,
                                           const IndexVector *IdxVec,
                                           ExprVector &InitExprs)
{
  const ArrayType *ArrayTy = dyn_cast<ArrayType>(Ty);
  if (ArrayTy) {
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
  unsigned int VecSz = IdxVec->size();
  for (IndexVector::const_iterator FI = IdxVec->begin(),
       FE = IdxVec->end(); FI != FE; ++FI)
  {
    const FieldDecl *FD = getFieldDeclByIdx(RD, (*FI));
    TransAssert(FD && "NULL FieldDecl!");
    IndexVector *FieldIdxVec = FieldToIdxVector[FD];
    TransAssert(FieldIdxVec && "Cannot find FieldIdxVec!");

    Ty = FD->getType().getTypePtr();
    const Expr *Init;
    unsigned int InitListIdx;
    if (RD->isUnion())
      InitListIdx = 0;
    else
      InitListIdx = (*FI);

    if (InitListIdx >= ILE->getNumInits())
      return;

    Init = ILE->getInit(InitListIdx);

    if (FD == TheFieldDecl) {
      InitExprs.push_back(Init);
      TransAssert((VecSz == 1) && "Bad IndexVector size!"); (void)VecSz;
    }
    else {
      getInitExprs(Ty, Init, FieldIdxVec, InitExprs);
    }
  }
}

void RemoveUnusedStructField::removeOneInitExpr(const Expr *E)
{
  TransAssert(NumFields && "NumFields cannot be zero!");
  if (NumFields == 1) {
    RewriteHelper->replaceExpr(E, "");
    return;
  }

  SourceRange ExpRange = E->getSourceRange();
  SourceLocation StartLoc = ExpRange.getBegin();
  SourceLocation EndLoc = ExpRange.getEnd();
  if (IsFirstField) {
    EndLoc = RewriteHelper->getEndLocationUntil(ExpRange, ',');
    TheRewriter.RemoveText(SourceRange(StartLoc, EndLoc));
    return;
  }

  const char *Buf = SrcManager->getCharacterData(StartLoc);
  int Offset = 0;
  while (*Buf != ',') {
    Buf--;
    Offset--;
  }
  StartLoc = StartLoc.getLocWithOffset(Offset);
  TheRewriter.RemoveText(SourceRange(StartLoc, EndLoc));
}

const RecordDecl *RemoveUnusedStructField::getBaseRecordDef(const Type *Ty)
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

void RemoveUnusedStructField::removeFieldDecl(void)
{
  // FIXME: we don't handle nested struct definition well.
  // For example,
  // struct S1 {
  //   struct S2 {
  //     int f1;
  //   } f1;
  //   struct S2 f2;
  //  }
  // will be transformed to 
  // struct S1 {
  //   struct S2 f2;
  //  }
  // Thus we will leave an incomplete struct S2
  RewriteHelper->removeFieldDecl(TheFieldDecl);
}

bool RemoveUnusedStructField::isSpecialRecordDecl(const RecordDecl *RD)
{
  std::string Name = RD->getNameAsString();
  return (Name == "__va_list_tag");
}

RemoveUnusedStructField::~RemoveUnusedStructField(void)
{
  if (CollectionVisitor)
    delete CollectionVisitor;
  if (RewriteVisitor)
    delete RewriteVisitor;

  for (RecordDeclToFieldIdxVectorMap::iterator I = RecordDeclToField.begin(),
       E = RecordDeclToField.end(); I != E; ++I) {
    delete (*I).second;
  }
}

