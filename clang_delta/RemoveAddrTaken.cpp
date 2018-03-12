//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2015, 2016, 2017 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "RemoveAddrTaken.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Remove an addr-taken operator if \n\
   * the subexpr is type of pointer, or \n\
   * the subexpr is type of integer and the addr-taken operator is \
an operand of a comparison operator, or \n\
   * the entire addr-taken expr is an argument of a function, \
and the argument doesn't have a correponding parameter in function's \
declaration.\n";

static RegisterTransformation<RemoveAddrTaken>
         Trans("remove-addr-taken", DescriptionMsg);

class RemoveAddrTakenCollectionVisitor : public 
  RecursiveASTVisitor<RemoveAddrTakenCollectionVisitor> {

public:

  explicit RemoveAddrTakenCollectionVisitor(RemoveAddrTaken *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitUnaryOperator(UnaryOperator *UO);

  bool VisitBinaryOperator(BinaryOperator *BO);

  bool VisitCallExpr(CallExpr *CE);

private:

  void handleOneAddrTakenOp(const UnaryOperator *UO);

  void handleOneOperand(const Expr *E);

  RemoveAddrTaken *ConsumerInstance;

};

void RemoveAddrTakenCollectionVisitor::handleOneAddrTakenOp(
       const UnaryOperator *UO)
{
  if (ConsumerInstance->isInIncludedFile(UO) ||
      ConsumerInstance->VisitedAddrTakenOps.count(UO))
    return;

  ConsumerInstance->VisitedAddrTakenOps.insert(UO);
  ConsumerInstance->ValidInstanceNum++;
  if (ConsumerInstance->TransformationCounter == 
      ConsumerInstance->ValidInstanceNum)
    ConsumerInstance->TheUO = UO; 
}

bool RemoveAddrTakenCollectionVisitor::VisitUnaryOperator(UnaryOperator *UO)
{
  if (UO->getOpcode() != UO_AddrOf)
    return true;

  const Expr *E = UO->getSubExpr();
  const Type *Ty = E->getType().getTypePtr();
  if (!Ty->isPointerType())
    return true;

  handleOneAddrTakenOp(UO);
  return true;
}

void RemoveAddrTakenCollectionVisitor::handleOneOperand(const Expr *E)
{
  const UnaryOperator *UO = dyn_cast<UnaryOperator>(E);
  if (!UO)
    return;

  if (UO->getOpcode() != UO_AddrOf)
    return;

  const Expr *SubE = UO->getSubExpr();
  const Type *Ty = SubE->getType().getTypePtr();
  if (!Ty->isIntegerType())
    return;

  handleOneAddrTakenOp(UO);
}

bool RemoveAddrTakenCollectionVisitor::VisitBinaryOperator(BinaryOperator *BO)
{
  if (!BO->isComparisonOp())
    return true;

  handleOneOperand(BO->getLHS());
  handleOneOperand(BO->getRHS());
  return true;
}

// handle special cases like
// void f1();
// void f2(void) {
//   f1(xxx);
// }
bool RemoveAddrTakenCollectionVisitor::VisitCallExpr(CallExpr *CE)
{
  const FunctionDecl *FD = CE->getDirectCallee();
  if (!FD)
    return true;

  unsigned NumParams = FD->getNumParams();
  if (NumParams != 0)
    return true;

  for (CallExpr::arg_iterator I = CE->arg_begin(),
       E = CE->arg_end(); I != E; ++I) {
    const Expr *Arg = *I;
    const UnaryOperator *UO = dyn_cast<UnaryOperator>(Arg);
    if (!UO || (UO->getOpcode() != UO_AddrOf))
      continue;
    handleOneAddrTakenOp(UO);
  }
  return true;
}

void RemoveAddrTaken::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new RemoveAddrTakenCollectionVisitor(this);
}

bool RemoveAddrTaken::HandleTopLevelDecl(DeclGroupRef D) 
{
  TransAssert(CollectionVisitor && "NULL CollectionVisitor!");
  if (TransformationManager::isCXXLangOpt()) {
    ValidInstanceNum = 0;
    return true;
  }

  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    FunctionDecl *FD = dyn_cast<FunctionDecl>(*I);
    if (!FD || !FD->isThisDeclarationADefinition())
      continue;
    CollectionVisitor->TraverseDecl(*I); 
  }

  return true;
}
 
void RemoveAddrTaken::HandleTranslationUnit(ASTContext &Ctx)
{
  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  TransAssert(TheUO && "NULL UnaryOperator!");
  rewriteAddrTakenOp(TheUO);

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void RemoveAddrTaken::rewriteAddrTakenOp(const UnaryOperator *UO)
{
  SourceLocation Loc = UO->getOperatorLoc();
  TheRewriter.RemoveText(Loc, 1);
}

RemoveAddrTaken::~RemoveAddrTaken(void)
{
  delete CollectionVisitor;
}

