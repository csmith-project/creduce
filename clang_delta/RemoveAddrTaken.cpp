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

#include "RemoveAddrTaken.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;
using namespace llvm;

static const char *DescriptionMsg =
"Remove an addr-taken operator if \n\
   * the subexpr is type of pointer, or \n\
   * the subexpr is type of integer and the addr-taken operator is \
an operand of a comparison operator. \n";

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

private:

  void handleOneOperand(const Expr *E);

  RemoveAddrTaken *ConsumerInstance;

};

bool RemoveAddrTakenCollectionVisitor::VisitUnaryOperator(UnaryOperator *UO)
{
  if (UO->getOpcode() != UO_AddrOf)
    return true;

  const Expr *E = UO->getSubExpr();
  const Type *Ty = E->getType().getTypePtr();
  if (!Ty->isPointerType())
    return true;

  ConsumerInstance->ValidInstanceNum++;
  if (ConsumerInstance->TransformationCounter == 
      ConsumerInstance->ValidInstanceNum)
    ConsumerInstance->TheUO = UO; 
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

  ConsumerInstance->ValidInstanceNum++;
  if (ConsumerInstance->TransformationCounter == 
      ConsumerInstance->ValidInstanceNum)
    ConsumerInstance->TheUO = UO; 
}

bool RemoveAddrTakenCollectionVisitor::VisitBinaryOperator(BinaryOperator *BO)
{
  if (!BO->isComparisonOp())
    return true;

  handleOneOperand(BO->getLHS());
  handleOneOperand(BO->getRHS());
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
  if (CollectionVisitor)
    delete CollectionVisitor;
}

