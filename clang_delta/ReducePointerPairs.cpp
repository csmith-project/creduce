//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "ReducePointerPairs.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;
using namespace llvm;

static const char *DescriptionMsg =
"Reduce a pair of pointers at the same time if they have the following shape:\n\
  int **p1;\n\
  int ***p2 = &p1;\n\
and both p1 and p2 are not used anywhere except comparisons between \
p2 and &p1.\n";

static RegisterTransformation<ReducePointerPairs>
         Trans("reduce-pointer-pairs", DescriptionMsg);

class ReducePointerPairsCollectionVisitor : public 
  RecursiveASTVisitor<ReducePointerPairsCollectionVisitor> {

public:

  explicit ReducePointerPairsCollectionVisitor(ReducePointerPairs *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitVarDecl(VarDecl *VD);

  bool VisitBinaryOperator(BinaryOperator *BO);

private:

  ReducePointerPairs *ConsumerInstance;
};

class ReducePointerPairsInvalidatingVisitor : public 
  RecursiveASTVisitor<ReducePointerPairsInvalidatingVisitor> {

public:

  typedef SmallVector<const DeclRefExpr *, 5> DeclRefExprQueue;

  explicit ReducePointerPairsInvalidatingVisitor(ReducePointerPairs *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitVarDecl(VarDecl *VD);

  bool VisitDeclRefExpr(DeclRefExpr *DRE);

  bool VisitBinaryOperator(BinaryOperator *BO);

private:

  void handleOneOperand(const Expr *E);

  ReducePointerPairs *ConsumerInstance;
  
  DeclRefExprQueue CurrentDeclRefExprs;

  DeclRefExprQueue CurrentPairedDeclRefExprs;
};

bool ReducePointerPairsCollectionVisitor::VisitVarDecl(VarDecl *VD)
{
  if (!ConsumerInstance->isValidVD(VD))
    return true;

  const Expr *Init = VD->getAnyInitializer();
  if (!Init)
    return true;
 
  ConsumerInstance->handleOnePair(VD->getCanonicalDecl(), 
                                  Init->IgnoreParenCasts());
  return true;
}

bool ReducePointerPairsCollectionVisitor::VisitBinaryOperator(
       BinaryOperator *BO)
{
  if (BO->getOpcode() != BO_Assign)
    return true;

  const Expr *Lhs = BO->getLHS()->IgnoreParenCasts();
  const VarDecl *VD = ConsumerInstance->getVarDeclFromDRE(Lhs);
  if (!VD || !ConsumerInstance->isValidVD(VD))
    return true;
 
  ConsumerInstance->handleOnePair(VD, BO->getRHS()->IgnoreParenCasts());
  return true;
}

bool ReducePointerPairsInvalidatingVisitor::VisitDeclRefExpr(DeclRefExpr *DRE)
{
  const DeclRefExpr *CurrentDeclRefExpr = NULL;
  const DeclRefExpr *CurrentPairedDeclRefExpr = NULL;
  
  if (CurrentDeclRefExprs.size())
    CurrentDeclRefExpr = CurrentDeclRefExprs.back();
  if (CurrentPairedDeclRefExprs.size())
    CurrentPairedDeclRefExpr = CurrentPairedDeclRefExprs.back();

  if (DRE == CurrentDeclRefExpr) {
    CurrentDeclRefExprs.pop_back();
    return true;
  }
  else if (DRE == CurrentPairedDeclRefExpr) {
    CurrentPairedDeclRefExprs.pop_back();
    return true;
  }

  const VarDecl *VD = ConsumerInstance->getVarDeclFromDRE(DRE);
  if (VD) {
    ConsumerInstance->invalidateVarDecl(VD);
    ConsumerInstance->invalidatePairedVarDecl(VD);
  }
  return true;
}

bool ReducePointerPairsInvalidatingVisitor::VisitVarDecl(VarDecl *VD)
{
  const VarDecl *CanonicalVD = VD->getCanonicalDecl();
  if (!ConsumerInstance->isMappedVarDecl(CanonicalVD))
    return true;

  const Expr *Init = CanonicalVD->getAnyInitializer();
  if (!Init)
    return true;
 
  if (ConsumerInstance->invalidatePairedVarDecl(CanonicalVD))
    return true;

  const UnaryOperator *UO = dyn_cast<UnaryOperator>(Init->IgnoreParenCasts());
  if (!UO || (UO->getOpcode() != UO_AddrOf))
    return true;

  const Expr *SubE = UO->getSubExpr()->IgnoreParenCasts();
  const VarDecl *PairedVD = ConsumerInstance->getVarDeclFromDRE(SubE);
  if (!PairedVD)
    return true;

  if (ConsumerInstance->ValidPointerPairs[CanonicalVD] == PairedVD) {
    const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(SubE);
    TransAssert(DRE && "Invalid DRE for Paired VarDecl!");
    CurrentPairedDeclRefExprs.push_back(DRE);
  }
  return true;
}

void ReducePointerPairsInvalidatingVisitor::handleOneOperand(const Expr *E)
{
  const VarDecl *VD = ConsumerInstance->getVarDeclFromDRE(E);
  if (VD && ConsumerInstance->isMappedVarDecl(VD)) {
    const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E);
    TransAssert(DRE && "Invalid DeclRefExpr!");
    CurrentDeclRefExprs.push_back(DRE);
    return;
  }

  if (const UnaryOperator *UO = dyn_cast<UnaryOperator>(E)) {
    if (UO && (UO->getOpcode() == UO_AddrOf)) {
      const Expr *SubE = UO->getSubExpr()->IgnoreParenCasts();
      const VarDecl *PairedVD = ConsumerInstance->getVarDeclFromDRE(SubE);
      if (ConsumerInstance->isMappedVarDecl(PairedVD)) {
        const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(SubE);
        TransAssert(DRE && "Invalid DeclRefExpr with UnaryOperator!");
        CurrentPairedDeclRefExprs.push_back(DRE);
      }
    }
  }
}

bool ReducePointerPairsInvalidatingVisitor::VisitBinaryOperator(
       BinaryOperator *BO)
{
  if (BO->getOpcode() == BO_Assign) {
    const Expr *Lhs = BO->getLHS()->IgnoreParenCasts();
    const VarDecl *VD = ConsumerInstance->getVarDeclFromDRE(Lhs);
    if (!VD)
      return true;
    const VarDecl *PairedVD = ConsumerInstance->ValidPointerPairs[VD];
    if (!PairedVD)
      return true;

    const Expr *Rhs = BO->getRHS()->IgnoreParenCasts();
    const UnaryOperator *UO = dyn_cast<UnaryOperator>(Rhs);
    if (!UO || (UO->getOpcode() != UO_AddrOf)) {
      ConsumerInstance->ValidPointerPairs.erase(VD);
      return true;
    }

    const Expr *SubE = UO->getSubExpr()->IgnoreParenCasts();
    const VarDecl *RhsVD = ConsumerInstance->getVarDeclFromDRE(SubE);
    if (!RhsVD || (RhsVD != PairedVD))
      ConsumerInstance->ValidPointerPairs.erase(VD);
    const DeclRefExpr *LhsDRE = dyn_cast<DeclRefExpr>(Lhs);
    TransAssert(LhsDRE && "Invalid LhsDRE!");
    CurrentDeclRefExprs.push_back(LhsDRE);
    const DeclRefExpr *SubDRE = dyn_cast<DeclRefExpr>(SubE);
    TransAssert(SubDRE && "Invalid SubDRE!");
    CurrentPairedDeclRefExprs.push_back(SubDRE);
    return true;
  }
  
  if (BO->isComparisonOp()) {
    handleOneOperand(BO->getLHS()->IgnoreParenCasts());
    handleOneOperand(BO->getRHS()->IgnoreParenCasts());
  }

  return true;
}

void ReducePointerPairs::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new ReducePointerPairsCollectionVisitor(this);
  InvalidatingVisitor = new ReducePointerPairsInvalidatingVisitor(this);
}

void ReducePointerPairs::HandleTranslationUnit(ASTContext &Ctx)
{
  TransAssert(CollectionVisitor && "NULL CollectionVisitor!");
  TransAssert(InvalidatingVisitor && "NULL InvalidatingVisitor!");
  CollectionVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
  InvalidatingVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
  doAnalysis();

  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);
  TransAssert(TheVarDecl && "NULL TheVarDecl!");
  TransAssert(ThePairedVarDecl && "NULL ThePairedVarDecl!");
  doRewriting(TheVarDecl);
  doRewriting(ThePairedVarDecl);

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void ReducePointerPairs::doAnalysis(void)
{
  for (PointerMap::iterator I = ValidPointerPairs.begin(),
       E = ValidPointerPairs.end(); I != E; ++I) {
    const VarDecl *PairedVD = (*I).second;
    if (!PairedVD)
      continue;

    ValidInstanceNum++;
    if (TransformationCounter == ValidInstanceNum) {
      TheVarDecl = (*I).first;
      ThePairedVarDecl = PairedVD;
    }
  }
}

void ReducePointerPairs::doRewriting(const VarDecl *VD)
{
  const VarDecl *FirstVD = VD->getCanonicalDecl();
  for(VarDecl::redecl_iterator RI = FirstVD->redecls_begin(),
      RE = FirstVD->redecls_end(); RI != RE; ++RI) {
    RewriteHelper->removeAStarBefore(*RI); 
  }
}

bool ReducePointerPairs::isValidVD(const VarDecl *VD)
{
  if (dyn_cast<ParmVarDecl>(VD))
    return false;

  const Type *Ty = VD->getType().getTypePtr();
  if (!Ty->isPointerType())
    return false;
  return true;
}

const VarDecl *ReducePointerPairs::getVarDeclFromDRE(const Expr *E)
{
  TransAssert(E && "NULL Expr!");

  const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E);
  if (!DRE)
    return NULL;

  const ValueDecl *OrigDecl = DRE->getDecl();
  const VarDecl *VD = dyn_cast<VarDecl>(OrigDecl);
  if (!VD || dyn_cast<ParmVarDecl>(VD))
    return NULL;
  return VD->getCanonicalDecl();
}

void ReducePointerPairs::handleOnePair(const VarDecl *VD, const Expr *E)
{
  const UnaryOperator *UO = dyn_cast<UnaryOperator>(E);
  if (!UO || (UO->getOpcode() != UO_AddrOf))
    return;

  const Expr *SubE = UO->getSubExpr()->IgnoreParenCasts();
  const Type *Ty = SubE->getType().getTypePtr();
  if (!Ty->isPointerType())
    return;
  
  const VarDecl *PairedVD = getVarDeclFromDRE(SubE);
  if (PairedVD && !ValidPointerPairs[VD]) {
    ValidPointerPairs[VD] = PairedVD;
  }
}

bool ReducePointerPairs::isMappedVarDecl(const VarDecl *VD)
{
  if (!VD)
    return false;

  for (PointerMap::iterator I = ValidPointerPairs.begin(), 
       E = ValidPointerPairs.end(); I != E; ++I) {
    if ((VD == (*I).first) || (VD == (*I).second))
      return true;
  }
  return false;
}

void ReducePointerPairs::invalidateVarDecl(const VarDecl *VD)
{
  PointerMap::iterator I = ValidPointerPairs.begin();
  PointerMap::iterator E = ValidPointerPairs.end();
  for (; I != E; ++I) {
    const VarDecl *D = (*I).first;
    if (VD == D)
      ValidPointerPairs[D] = NULL;
  }
}

bool ReducePointerPairs::invalidatePairedVarDecl(const VarDecl *VD)
{
  bool RV = false;
  PointerMap::iterator I = ValidPointerPairs.begin();
  PointerMap::iterator E = ValidPointerPairs.end();
  for (; I != E; ++I) {
    const VarDecl *D = (*I).first;
    if (VD == (*I).second) {
      ValidPointerPairs[D] = NULL;
      RV = true;
    }
  }
  return RV;
}

ReducePointerPairs::~ReducePointerPairs(void)
{
  delete CollectionVisitor;
  delete InvalidatingVisitor;
}

