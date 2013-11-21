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

#include "ReduceArraySize.h"

#include <sstream>
#include <cctype>
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;
using namespace llvm;

static const char *DescriptionMsg =
"Reduce the size of an array to the maximum index of \
accessing this array. Each transformation iteration \
works on one dimension for multidimensional arrays. \
This transformation is legitimate for an array if: \n\
  * this array is ConstantArrayType; \n\
  * and all indeices to this array are constants. \n";

static RegisterTransformation<ReduceArraySize>
         Trans("reduce-array-size", DescriptionMsg);

class ReduceArraySizeCollectionVisitor : public 
  RecursiveASTVisitor<ReduceArraySizeCollectionVisitor> {

public:

  explicit ReduceArraySizeCollectionVisitor(ReduceArraySize *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitVarDecl(VarDecl *VD);

  bool VisitArraySubscriptExpr(ArraySubscriptExpr *ASE);

private:

  ReduceArraySize *ConsumerInstance;
};

bool ReduceArraySizeCollectionVisitor::VisitVarDecl(VarDecl *VD)
{
  ConsumerInstance->handleOneVar(VD);
  return true;
}

bool ReduceArraySizeCollectionVisitor::VisitArraySubscriptExpr(
       ArraySubscriptExpr *ASE)
{
  ConsumerInstance->handleOneASE(ASE);
  return true;
}

void ReduceArraySize::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new ReduceArraySizeCollectionVisitor(this);
}

bool ReduceArraySize::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    CollectionVisitor->TraverseDecl(*I);
  }
  return true;
}
 
void ReduceArraySize::HandleTranslationUnit(ASTContext &Ctx)
{
  doAnalysis();
  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  TransAssert(CollectionVisitor && "NULL CollectionVisitor!");
  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);
  TransAssert(TheVarDecl && "NULL TheVarDecl!");
  TransAssert((TheDimValue >= 0) && "Bad TheDimValue!");

  rewriteArrayVarDecl();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void ReduceArraySize::doAnalysis(void)
{
  for (VarDeclToDimMap::iterator I = VarDeclToDim.begin(),
       E = VarDeclToDim.end(); I != E; ++I) {

    const VarDecl *VD = (*I).first;
    DimValueVector *DimVec = (*I).second;

    if (!DimVec)
      continue;

    DimValueVector *OrigDimVec = OrigVarDeclToDim[VD];
    TransAssert(OrigDimVec && "Null OrigDimVec!");

    unsigned int DimSz = DimVec->size();
    TransAssert((DimSz == OrigDimVec->size()) &&
                "Two DimValueVectors should have the same size!");
    for (unsigned int I = 0; I < DimSz; ++I) {
      int DimV = (*DimVec)[I];
      int OrigDimV = (*OrigDimVec)[I];
      if ((DimV == -1) || (OrigDimV == 0) || ((DimV+1) == OrigDimV))
        continue;

      ValidInstanceNum++;
      if (TransformationCounter != ValidInstanceNum)
        continue;

      TheVarDecl = VD;
      TheDimValue = DimV;
      TheDimIdx = I;
    }
  }
}

void ReduceArraySize::getBracketLocPair(const VarDecl *VD,
                                        unsigned int Dim,
                                        unsigned int DimIdx,
                                        BracketLocPair &LocPair)
{
  SourceLocation StartLoc = VD->getLocation();
  SourceLocation BPStartLoc, BPEndLoc;
  for (unsigned int I = 0; I < Dim; ++I) {
    BPStartLoc = RewriteHelper->getLocationUntil(StartLoc, '[');
    BPEndLoc = RewriteHelper->getLocationUntil(BPStartLoc, ']');

    if (I == DimIdx) {
      LocPair.first = BPStartLoc;
      LocPair.second = BPEndLoc;
      break;
    }

    StartLoc = BPEndLoc;
  }
}

void ReduceArraySize::rewriteArrayVarDecl(void)
{
  const Type *Ty = TheVarDecl->getType().getTypePtr();
  const ArrayType *ArrayTy = dyn_cast<ArrayType>(Ty);
  TransAssert(ArrayTy && "Invalid ArrayType!");
  unsigned int Dim = getArrayDimension(ArrayTy);
  TransAssert((Dim > TheDimIdx) && "Bad Dimension Index!");

  const VarDecl *FirstVD = TheVarDecl->getCanonicalDecl();
  for(VarDecl::redecl_iterator RI = FirstVD->redecls_begin(),
      RE = FirstVD->redecls_end(); RI != RE; ++RI) {
    BracketLocPair LocPair;
    getBracketLocPair((*RI), Dim, TheDimIdx, LocPair);

    std::stringstream TmpSS;
    SourceLocation StartLoc = (LocPair.first).getLocWithOffset(1);
    SourceLocation EndLoc = (LocPair.second).getLocWithOffset(-1);
    TmpSS << (TheDimValue + 1);
    TheRewriter.ReplaceText(SourceRange(StartLoc, EndLoc), TmpSS.str());
  }
}

void ReduceArraySize::handleOneVar(const VarDecl *VD)
{
  const Type *Ty = VD->getType().getTypePtr();
  const ArrayType *ArrayTy = dyn_cast<ArrayType>(Ty);
  if (!ArrayTy)
    return;

  const VarDecl *CanonicalVD = VD->getCanonicalDecl();
  DimValueVector *DimVec = VarDeclToDim[CanonicalVD];
  if (DimVec)
    return;

  ArraySubTypeVector TyVec;
  unsigned int Dim = getArrayDimensionAndTypes(ArrayTy, TyVec);
  DimVec = new DimValueVector(Dim, -1);
  DimValueVector *OrigDimVec = new DimValueVector(Dim, -1);

  TransAssert((Dim == TyVec.size()) && 
              "Unmatched Dimension and Array Sub Types!");
  for (unsigned int I = 0; I < Dim; ++I) {
    const ConstantArrayType *CstArrayTy = 
      dyn_cast<ConstantArrayType>(TyVec[I]);
    if (!CstArrayTy) {
      continue;
    }

    unsigned int InitSz = getConstArraySize(CstArrayTy);
    (*DimVec)[I] = 0;
    (*OrigDimVec)[I] = InitSz;
  }

  VarDeclToDim[CanonicalVD] = DimVec;
  OrigVarDeclToDim[CanonicalVD] = OrigDimVec;
}

bool ReduceArraySize::isIntegerExpr(const Expr *Exp)
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

void ReduceArraySize::handleOneASE(const ArraySubscriptExpr *ASE)
{
  const Type *ASETy = ASE->getType().getTypePtr();
  if (!ASETy->isScalarType() && !ASETy->isStructureType() && 
      !ASETy->isUnionType())
    return;
  
  ExprVector IdxExprs;
  const Expr *BaseE = getBaseExprAndIdxExprs(ASE, IdxExprs);
  TransAssert(BaseE && "Empty Base expression!");

  const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(BaseE);
  if (!DRE)
    return;

  const ValueDecl *OrigDecl = DRE->getDecl();
  const VarDecl *VD = dyn_cast<VarDecl>(OrigDecl);
  if (!VD)
    return;

  const VarDecl *CanonicalVD = VD->getCanonicalDecl();
  DimValueVector *DimVec = VarDeclToDim[CanonicalVD];
  // It's possible DimVec is NULL, e.g.,
  // int main(..., char *argv[]) {
  //  ... argv[1] ...
  // }
  if (!DimVec)
    return;

  TransAssert((DimVec->size() >= IdxExprs.size()) &&
              "More indices than it should be!");

  unsigned int DimIdx = 0;
  for (ExprVector::reverse_iterator I = IdxExprs.rbegin(), 
       E = IdxExprs.rend(); I != E; ++I) {

    int OldIdx = (*DimVec)[DimIdx];
    if (OldIdx == -1) {
      DimIdx++;
      continue;
    }

    const Expr *IdxE = (*I);
    if (isIntegerExpr(IdxE)) {
      int Idx = getIndexAsInteger(IdxE);
      if (Idx > OldIdx)
        (*DimVec)[DimIdx] = Idx;
    }
    else {
      (*DimVec)[DimIdx] = -1;
    }
    DimIdx++;
  }
}

ReduceArraySize::~ReduceArraySize(void)
{
  if (CollectionVisitor)
    delete CollectionVisitor;

  for (VarDeclToDimMap::iterator I = VarDeclToDim.begin(),
       E = VarDeclToDim.end(); I != E; ++I) {
    delete (*I).second;
  }

  for (VarDeclToDimMap::iterator I = OrigVarDeclToDim.begin(),
       E = OrigVarDeclToDim.end(); I != E; ++I) {
    delete (*I).second;
  }
}

