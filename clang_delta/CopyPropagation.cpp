//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2014, 2015, 2016 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "CopyPropagation.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"A simply copy propagation pass. \
Copy propagation only happens to: \n\
  * variables \n\
  * member expressions \n\
  * array subscript expressions \n\
Only those value obtained through simple \
assignment or initilizer will be propagated. \
Here \"simple assignment\" is defined as: \n\
  x = y \n\
where both x and y are either variables, member expressions \
or array subscript expressions (y could be a constant). \
For example, x = foo() is not considered as a simple assignment. \
Copy propagation of LHS will stop upon any non-simple assignment. \
Therefore, in the above example, foo() will not be propagated. \n";

static RegisterTransformation<CopyPropagation>
         Trans("copy-propagation", DescriptionMsg);

namespace {
class ArraySubscriptVisitor : public
        RecursiveASTVisitor<ArraySubscriptVisitor> {
public:
  explicit ArraySubscriptVisitor(const VarDecl *VD)
    : ReferencedVD(VD), HasReference(false)
  { }

  bool VisitVarDecl(VarDecl *VD);

  bool hasReferencedVD() {
    return HasReference;
  }

private:
  const VarDecl *ReferencedVD;

  bool HasReference;
};

bool ArraySubscriptVisitor::VisitVarDecl(VarDecl *VD) {
  if (VD->getCanonicalDecl() == ReferencedVD)
    HasReference = true;
  return true;
}

} // End anonymous namespace

class CopyPropCollectionVisitor : public
        RecursiveASTVisitor<CopyPropCollectionVisitor> {
public:

  explicit CopyPropCollectionVisitor(CopyPropagation *Instance)
    : ConsumerInstance(Instance),
      BeingWritten(false),
      BeingAddrTaken(false),
      BeingIncDec(false),
      BeingPartial(false)
  { }

  bool VisitVarDecl(VarDecl *VD);

  bool VisitBinaryOperator(BinaryOperator *BO);

  bool VisitUnaryOperator(UnaryOperator *UO);

  bool VisitDeclRefExpr(DeclRefExpr *DRE);

  bool VisitMemberExpr(MemberExpr *ME);

  bool VisitArraySubscriptExpr(ArraySubscriptExpr *ASE);

private:

  void resetFlags(void);

  CopyPropagation *ConsumerInstance;

  // Indicate if a var/memexpr/arraysubexpr is being written.
  // Set by updateExpr and reset by VisitDeclRefExpr
  bool BeingWritten;

  bool BeingAddrTaken;

  // It will be true for ++i, --i
  // In this case, we cannot copy-propagate a constant to i
  bool BeingIncDec;

  // For MemberExpr and ArraySubscriptExpr, we don't want to
  // track the partial element. For example, a[0][0][0],
  // VisitArraySubscriptExpr will visit:
  //   a[0][0][0]
  //   a[0][0]
  //   a[0]
  // We only want to track the first one. This flag will be
  // reset when we reach a.
  bool BeingPartial;
};

void CopyPropCollectionVisitor::resetFlags(void)
{
  BeingWritten = false;
  BeingAddrTaken = false;
  BeingIncDec = false;
  BeingPartial = false;
}

bool CopyPropCollectionVisitor::VisitVarDecl(VarDecl *VD)
{
  if (!VD->hasInit())
    return true;

  const Expr *Init = VD->getInit();

  if (ConsumerInstance->isValidExpr(Init)) {
    const VarDecl *CanonicalVD = VD->getCanonicalDecl();
    ConsumerInstance->VarToExpr[CanonicalVD] = Init;
  }

  return true;
}

bool CopyPropCollectionVisitor::VisitBinaryOperator(BinaryOperator *BO)
{
  if (!BO->isAssignmentOp() && !BO->isCompoundAssignmentOp())
    return true;

  const Expr *Lhs = BO->getLHS()->IgnoreParenCasts();
  if (!ConsumerInstance->isValidLhs(Lhs))
    return true;

  const Expr *Rhs = BO->getRHS()->IgnoreParenCasts();

  BeingWritten = true;
  if (!ConsumerInstance->isValidExpr(Rhs)) {
    ConsumerInstance->invalidateExpr(Lhs);
    return true;
  }

  ConsumerInstance->updateExpr(Lhs, Rhs);
  return true;
}

bool CopyPropCollectionVisitor::VisitUnaryOperator(UnaryOperator *UO)
{
  UnaryOperator::Opcode Op = UO->getOpcode();

  if (Op == UO_AddrOf) {
    BeingAddrTaken = true;
    return true;
  }

  if (UO->isIncrementDecrementOp())
    BeingIncDec = true;

  return true;
}

bool CopyPropCollectionVisitor::VisitDeclRefExpr(DeclRefExpr *DRE)
{
  if (BeingWritten || BeingAddrTaken || BeingPartial) {
    resetFlags();
    return true;
  }

  const ValueDecl *OrigDecl = DRE->getDecl();
  const VarDecl *VD = dyn_cast<VarDecl>(OrigDecl);
  // DRE could refer to FunctionDecl, etc
  if (!VD)
    return true;

  const VarDecl *CanonicalVD = VD->getCanonicalDecl();

  const Expr *CopyE = ConsumerInstance->VarToExpr[CanonicalVD];
  if (!CopyE || (BeingIncDec && ConsumerInstance->isConstantExpr(CopyE))) {
    BeingIncDec = false;
    return true;
  }

  ConsumerInstance->addOneDominatedExpr(CopyE, DRE);
  return true;
}

bool CopyPropCollectionVisitor::VisitMemberExpr(MemberExpr *ME)
{
  if (BeingWritten || BeingAddrTaken || BeingPartial) {
    return true;
  }

  if (!BeingPartial)
    BeingPartial = true;

  const Expr *CopyE = ConsumerInstance->MemberExprToExpr[ME];
  if (!CopyE) {
    if (!ConsumerInstance->VisitedMEAndASE.count(ME)) {
      CopyE = ConsumerInstance->getMemberExprElem(ME);
    }
    else {
      return true;
    }
  }

  if (!CopyE || (BeingIncDec && ConsumerInstance->isConstantExpr(CopyE))) {
    BeingIncDec = false;
    return true;
  }

  ConsumerInstance->addOneDominatedExpr(CopyE, ME);
  return true;
}

bool
CopyPropCollectionVisitor::VisitArraySubscriptExpr(ArraySubscriptExpr *ASE)
{
  if (BeingWritten || BeingAddrTaken || BeingPartial) {
    return true;
  }

  if (!BeingPartial)
    BeingPartial = true;

  const Expr *CopyE = ConsumerInstance->ArraySubToExpr[ASE];
  if (!CopyE) {
    if (!ConsumerInstance->VisitedMEAndASE.count(ASE)) {
      CopyE = ConsumerInstance->getArraySubscriptElem(ASE);
    }
    else {
      return true;
    }
  }

  if (!CopyE || (BeingIncDec && ConsumerInstance->isConstantExpr(CopyE))) {
    BeingIncDec = false;
    return true;
  }

  ConsumerInstance->addOneDominatedExpr(CopyE, ASE);
  return true;
}

void CopyPropagation::Initialize(ASTContext &context)
{
  Transformation::Initialize(context);
  CollectionVisitor = new CopyPropCollectionVisitor(this);
}

bool CopyPropagation::HandleTopLevelDecl(DeclGroupRef D)
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I)
    CollectionVisitor->TraverseDecl(*I);
  return true;
}

void CopyPropagation::HandleTranslationUnit(ASTContext &Ctx)
{
  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  TransAssert(CollectionVisitor && "NULL CollectionVisitor!");

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);
  TransAssert(TheCopyExpr && "NULL TheCopyExpr!");

  doCopyPropagation();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

// Note we skip InitListExpr, so some likely valid cases won't be handled:
//   int i = {1};
// But this kind of code almost couldn't happen,
// because c_delta will remove {} pair.
bool CopyPropagation::isValidExpr(const Expr *Exp)
{
  const Expr *E = Exp->IgnoreParenCasts();

  switch(E->getStmtClass()) {
  case Expr::FloatingLiteralClass:
  case Expr::StringLiteralClass:
  case Expr::IntegerLiteralClass:
  case Expr::GNUNullExprClass:
  case Expr::CharacterLiteralClass:
  case Expr::DeclRefExprClass:
  case Expr::MemberExprClass:
  case Expr::ArraySubscriptExprClass: // Fall-through
    return true;

  default:
    return false;
  }
  TransAssert(0 && "Unreachable code!");
  return false;
}

bool CopyPropagation::isValidLhs(const Expr *Lhs)
{
  const Expr *E = Lhs->IgnoreParenCasts();

  switch(E->getStmtClass()) {
  case Expr::DeclRefExprClass:
  case Expr::MemberExprClass:
  case Expr::ArraySubscriptExprClass: // Fall-through
    return true;

  default:
    return false;
  }
  TransAssert(0 && "Unreachable code!");
  return false;
}

bool CopyPropagation::isConstantExpr(const Expr *Exp)
{
  const Expr *E = Exp->IgnoreParenCasts();

  switch(E->getStmtClass()) {
  case Expr::FloatingLiteralClass:
  case Expr::StringLiteralClass:
  case Expr::IntegerLiteralClass:
  case Expr::GNUNullExprClass:
  case Expr::CharacterLiteralClass: // Fall-through
    return true;

  default:
    return false;
  }
  TransAssert(0 && "Unreachable code!");
  return false;
}

void CopyPropagation::updateExpr(const Expr *E, const Expr *CopyE)
{
  switch (E->getStmtClass()) {
  case Expr::DeclRefExprClass: {
    const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E);
    const ValueDecl *OrigDecl = DRE->getDecl();
    const VarDecl *VD = dyn_cast<VarDecl>(OrigDecl);

    TransAssert(VD && "Bad VD!");
    const VarDecl *CanonicalVD = VD->getCanonicalDecl();
    VarToExpr[CanonicalVD] = CopyE;
    return;
  }

  case Expr::MemberExprClass: {
    const MemberExpr *ME = dyn_cast<MemberExpr>(E);
    MemberExprToExpr[ME] = CopyE;
    VisitedMEAndASE.insert(E);
    return;
  }

  case Expr::ArraySubscriptExprClass: {
    const ArraySubscriptExpr *ASE = dyn_cast<ArraySubscriptExpr>(E);
    ArraySubToExpr[ASE] = CopyE;
    VisitedMEAndASE.insert(E);
    return;
  }

  default:
    TransAssert(0 && "Uncatched Expr!");
  }

  TransAssert(0 && "Unreachable code!");
}

void CopyPropagation::invalidateExpr(const Expr *E)
{
  updateExpr(E, NULL);
}

const VarDecl *CopyPropagation::getCanonicalRefVarDecl(const Expr *E)
{
  const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E);
  if (!DRE)
    return NULL;

  const ValueDecl *OrigDecl = DRE->getDecl();
  const VarDecl *VD = dyn_cast<VarDecl>(OrigDecl);
  // DRE could refer to FunctionDecl, etc
  if (!VD)
    return NULL;

  return VD->getCanonicalDecl();
}

bool CopyPropagation::isRefToTheSameVar(const Expr *CopyE,
                                        const Expr *DominatedE)
{
  const VarDecl *DominatedVD  = getCanonicalRefVarDecl(DominatedE);
  if (!DominatedVD)
    return false;
  const VarDecl *CopyVD  = getCanonicalRefVarDecl(CopyE);
  if (CopyVD)
    return CopyVD == DominatedVD;
  if (const ArraySubscriptExpr *ASE = dyn_cast<ArraySubscriptExpr>(CopyE)) {
    ArraySubscriptVisitor V(DominatedVD);
    V.TraverseStmt(const_cast<Expr*>(ASE->getIdx()));
    return !V.hasReferencedVD();
  }
  return false;
}

bool CopyPropagation::hasSameStringRep(const Expr *CopyE,
                                       const Expr *DominatedE)
{
  std::string CopyStr, DominatedStr;
  RewriteHelper->getExprString(CopyE, CopyStr);
  RewriteHelper->getExprString(DominatedE, DominatedStr);
  return (CopyStr == DominatedStr);
}

void CopyPropagation::addOneDominatedExpr(const Expr *CopyE,
                                          const Expr *DominatedE)
{
  if (isInIncludedFile(CopyE) || isInIncludedFile(DominatedE))
    return;

  if ((CopyE == DominatedE) || isRefToTheSameVar(CopyE, DominatedE) ||
      hasSameStringRep(CopyE, DominatedE))
    return;

  ExprSet *ESet = DominatedMap[CopyE];
  if (!ESet) {
    ESet = new ExprSet();
    TransAssert(ESet && "Couldn't new ExprSet");
    DominatedMap[CopyE] = ESet;

    ValidInstanceNum++;
    if (TransformationCounter == ValidInstanceNum)
      TheCopyExpr = CopyE;
  }
  ESet->insert(DominatedE);
}

void CopyPropagation::doCopyPropagation(void)
{
  std::string CopyStr("");
  RewriteHelper->getExprString(TheCopyExpr, CopyStr);
  ExprSet *ESet = DominatedMap[TheCopyExpr];
  TransAssert(ESet && "Empty Expr Set!");
  for (ExprSet::iterator I = ESet->begin(), E = ESet->end(); I != E; ++I) {
    RewriteHelper->replaceExpr((*I), CopyStr);
  }
}

CopyPropagation::~CopyPropagation(void)
{
  delete CollectionVisitor;

  for (ExprToExprsMap::iterator I = DominatedMap.begin(),
       E = DominatedMap.end(); I != E; ++I) {
    delete (*I).second;
  }
}

