//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2015, 2017 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "ReduceArrayDim.h"

#include <sstream>
#include <cctype>
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Reduce the dimension of an array. Each transformation iteration \
reduces one dimension in the following way: \n\
  int a[2][3][4]; \n\
  void foo(void) {... a[1][2][3] ... }\n\
===> \n\
  int a[2][3 * 4]; \n\
  void foo(void) {... a[1][3 * 2 + 3] ... }\n\
The binary operations will be computed to constant during the \
transformation if possible. Array fields are not handled right now. \
Also, this pass only works with ConstantArrayType and IncompleteArrayType.\n";

static RegisterTransformation<ReduceArrayDim>
         Trans("reduce-array-dim", DescriptionMsg);

class ReduceArrayDimCollectionVisitor : public 
  RecursiveASTVisitor<ReduceArrayDimCollectionVisitor> {

public:

  explicit ReduceArrayDimCollectionVisitor(ReduceArrayDim *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitVarDecl(VarDecl *VD);

private:

  ReduceArrayDim *ConsumerInstance;
};

bool ReduceArrayDimCollectionVisitor::VisitVarDecl(VarDecl *VD)
{
  ConsumerInstance->addOneVar(VD);
  return true;
}

class ReduceArrayDimRewriteVisitor : public 
  RecursiveASTVisitor<ReduceArrayDimRewriteVisitor> {

public:

  explicit ReduceArrayDimRewriteVisitor(ReduceArrayDim *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitVarDecl(VarDecl *VD);

  bool VisitArraySubscriptExpr(ArraySubscriptExpr *DRE);

private:

  ReduceArrayDim *ConsumerInstance;

};

bool ReduceArrayDimRewriteVisitor::VisitVarDecl(VarDecl *VD)
{
  const VarDecl *CanonicalVD = VD->getCanonicalDecl();
  if (CanonicalVD != ConsumerInstance->TheVarDecl)
    return true;

  ConsumerInstance->rewriteOneVarDecl(VD);
  return true;
}

bool ReduceArrayDimRewriteVisitor::VisitArraySubscriptExpr(
       ArraySubscriptExpr *ASE)
{
  ConsumerInstance->handleOneArraySubscriptExpr(ASE);
  return true;
}

void ReduceArrayDim::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new ReduceArrayDimCollectionVisitor(this);
  RewriteVisitor = new ReduceArrayDimRewriteVisitor(this);
}

bool ReduceArrayDim::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    CollectionVisitor->TraverseDecl(*I);
  }
  return true;
}
 
void ReduceArrayDim::HandleTranslationUnit(ASTContext &Ctx)
{
  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  TransAssert(RewriteVisitor && "NULL RewriteVisitor!");
  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);
  TransAssert(TheVarDecl && "NULL TheVarDecl!");

  RewriteVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
  if (!Rewritten) {
    TransError = TransNoTextModificationError;
    return;
  }

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void ReduceArrayDim::addOneVar(const VarDecl *VD)
{
  if (isInIncludedFile(VD))
    return;

  const Type *Ty = VD->getType().getTypePtr();
  const ArrayType *ArrayTy = dyn_cast<ArrayType>(Ty);
  if (!ArrayTy)
    return;

  unsigned int ArrayDim = getArrayDimension(ArrayTy);
  if (ArrayDim <= 1)
    return;

  // Skip these two array types
  if (dyn_cast<DependentSizedArrayType>(ArrayTy) ||
      dyn_cast<VariableArrayType>(ArrayTy))
    return;

  const VarDecl *CanonicalVD = VD->getCanonicalDecl();
  if (VisitedVarDecls.count(CanonicalVD))
    return;

  VisitedVarDecls.insert(CanonicalVD);
  ValidInstanceNum++;
  if (TransformationCounter == ValidInstanceNum) {
    TheVarDecl = CanonicalVD;
  }
}

// This function doesn't consider nested brackets, which won't
// exist for the definitions of ConstantArrayType and IncompleteArrayType
void ReduceArrayDim::getBracketLocPairs(const VarDecl *VD,
                                        unsigned int Dim,
                                        BracketLocPairVector &BPVec)
{
  SourceLocation StartLoc = VD->getLocation();
  SourceLocation BPStartLoc, BPEndLoc;
  for (unsigned int I = 0; I < Dim; ++I) {
    BPStartLoc = RewriteHelper->getLocationUntil(StartLoc, '[');
    BPEndLoc = RewriteHelper->getLocationUntil(BPStartLoc, ']');

    BracketLocPair *LocPair = new BracketLocPair();
    LocPair->first = BPStartLoc;
    LocPair->second = BPEndLoc;
    BPVec.push_back(LocPair);
    StartLoc = BPEndLoc;
  }
}

void ReduceArrayDim::freeBracketLocPairs(BracketLocPairVector &BPVec)
{
  for (BracketLocPairVector::iterator I = BPVec.begin(), 
       E = BPVec.end(); I != E; ++I) {
    delete (*I); 
  }
}

void ReduceArrayDim::getInitListExprs(InitListExprVector &InitVec,
                                      const InitListExpr *ILE,
                                      unsigned int Dim)
{
  unsigned int NumInits = ILE->getNumInits();
  for (unsigned int I = 0; I < NumInits; ++I) {
    const Expr *E = ILE->getInit(I);
    const InitListExpr *SubILE = dyn_cast<InitListExpr>(E);
    if (!SubILE)
      continue;

    if (Dim == 1) {
      InitVec.push_back(SubILE);
    }
    else {
      getInitListExprs(InitVec, SubILE, Dim-1);
    }
  }
}

void ReduceArrayDim::rewriteInitListExpr(const InitListExpr *ILE, 
                                         unsigned int Dim)
{
  TransAssert((Dim > 1) && "Invalid array dimension!");
  InitListExprVector InitVec;
  getInitListExprs(InitVec, ILE, Dim-1);

  for (InitListExprVector::reverse_iterator I = InitVec.rbegin(),
       E = InitVec.rend(); I != E; ++I) {
    SourceLocation RBLoc = (*I)->getRBraceLoc();
    SourceLocation RLLoc = (*I)->getLBraceLoc();

    const char *RBBuf = SrcManager->getCharacterData(RBLoc);
    const char *RLBuf = SrcManager->getCharacterData(RLLoc);

    // make sure RBBuf and RLBuf are correct. They could be incorrect
    // for cases like: int a[2][2] = {1}
    if (((*RBBuf) == '}') && ((*RLBuf) == '{')) {
      TheRewriter.RemoveText(RBLoc, 1);
      TheRewriter.RemoveText(RLLoc, 1);
    }
  }
}

void ReduceArrayDim::rewriteOneVarDecl(const VarDecl *VD)
{
  const Type *Ty = VD->getType().getTypePtr();
  const ArrayType *ArrayTy = dyn_cast<ArrayType>(Ty);
  TransAssert(ArrayTy && "Invalid ArrayType!");

  ArraySubTypeVector TyVec;
  unsigned int Dim = getArrayDimensionAndTypes(ArrayTy, TyVec);
  if (VD->hasInit()) {
    const Expr *InitE = VD->getInit();
    const InitListExpr *ILE = dyn_cast<InitListExpr>(InitE);
    if (ILE)
      rewriteInitListExpr(ILE, Dim);
  }

  BracketLocPairVector BPVector;
  getBracketLocPairs(VD, Dim, BPVector);
  TransAssert((BPVector.size() > 1) && "Invalid Bracket Pairs!");
  
  ArraySubTypeVector::const_reverse_iterator TyIdx = TyVec.rbegin();
  const ConstantArrayType *CstArrayTy = dyn_cast<ConstantArrayType>(*TyIdx);
  TransAssert(CstArrayTy && "Non ConstantArrayType!");
  unsigned LastSz = getConstArraySize(CstArrayTy);

  ++TyIdx;
  const ArrayType *SecArrayTy = (*TyIdx);

  BracketLocPairVector::reverse_iterator BIdx = BPVector.rbegin();
  BracketLocPair *LastBracketPair = (*BIdx);
  TheRewriter.RemoveText(SourceRange(LastBracketPair->first,
                                     LastBracketPair->second));

  ++BIdx;
  BracketLocPair *SecBracketPair = (*BIdx);
  // We keep incomplete array
  if (!dyn_cast<IncompleteArrayType>(SecArrayTy)) {
    const ConstantArrayType *SecCstArrayTy = 
      dyn_cast<ConstantArrayType>(SecArrayTy);
    TransAssert(SecCstArrayTy && "Non ConstantArrayType!");

    // Keep this value, which is needed for rewriting ArraySubscriptExpr
    ArraySz = getConstArraySize(SecCstArrayTy);

    std::stringstream TmpSS;
    TmpSS << (LastSz * ArraySz);

    SourceLocation StartLoc = (SecBracketPair->first).getLocWithOffset(1);
    SourceLocation EndLoc = (SecBracketPair->second).getLocWithOffset(-1);
    TheRewriter.ReplaceText(SourceRange(StartLoc, EndLoc), TmpSS.str());
  }

  freeBracketLocPairs(BPVector);
  Rewritten = true;
  return;
}

bool ReduceArrayDim::isIntegerExpr(const Expr *Exp)
{
  const Expr *E = Exp->IgnoreParenCasts();

  switch(E->getStmtClass()) {
  case Expr::IntegerLiteralClass:
  case Expr::CharacterLiteralClass: // Fall-through
    return true;

  default:
    return false;
  }
  TransAssert(0 && "Unreachable code!");
  return false;
}

void ReduceArrayDim::rewriteSubscriptExpr(const ExprVector &IdxExprs)
{
  ExprVector::const_iterator I = IdxExprs.begin();
  const Expr *LastE = (*I);
  ++I;
  const Expr *SecE = (*I);
  RewriteHelper->removeArraySubscriptExpr(LastE);

  int LastIdx = -1;
  int SecIdx = -1;
  if (isIntegerExpr(LastE))
    LastIdx = getIndexAsInteger(LastE);

  if (isIntegerExpr(SecE))
    SecIdx = getIndexAsInteger(SecE);

  if ((LastIdx >= 0) && (SecIdx >= 0)) {
    int NewIdx = (SecIdx * ArraySz + LastIdx);
    std::stringstream TmpSS;
    TmpSS << NewIdx;
    RewriteHelper->replaceExpr(SecE, TmpSS.str());
    return;
  }

  std::string LastStr, SecStr, newStr;
  RewriteHelper->getExprString(LastE, LastStr);
  RewriteHelper->getExprString(SecE, SecStr);
  std::stringstream TmpSS;
  if (ArraySz == 1) {
    TmpSS << SecStr << "+" << LastStr;
  }
  else if (SecIdx == 1) {
    TmpSS << ArraySz << "+" << LastStr;
  }
  else {
    TmpSS << "(" << SecStr << ")*" << ArraySz << "+" << LastStr;
  }
  RewriteHelper->replaceExpr(SecE, TmpSS.str());
}

void ReduceArrayDim::handleOneArraySubscriptExpr(
       const ArraySubscriptExpr *ASE)
{
  const Type *ASETy = ASE->getType().getTypePtr();
  if (!ASETy->isScalarType() && !ASETy->isStructureType() && 
      !ASETy->isUnionType())
    return;

  ExprVector IdxExprs;
  const Expr *BaseE = getBaseExprAndIdxExprs(ASE, IdxExprs);
  TransAssert(BaseE && "Empty Base expression!");

  if (IdxExprs.size() <= 1)
    return;

  const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(BaseE);
  if (!DRE)
    return;

  const ValueDecl *OrigDecl = DRE->getDecl();
  const VarDecl *VD = dyn_cast<VarDecl>(OrigDecl);
  if (!VD)
    return;

  const VarDecl *CanonicalVD = VD->getCanonicalDecl();
  if (CanonicalVD != TheVarDecl)
    return;

  rewriteSubscriptExpr(IdxExprs); 
  Rewritten = true;
}

ReduceArrayDim::~ReduceArrayDim(void)
{
  delete CollectionVisitor;
  delete RewriteVisitor;
}

