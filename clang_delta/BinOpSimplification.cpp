//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2015 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "BinOpSimplification.h"

#include <sstream>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "CommonStatementVisitor.h"
#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Simplify a complex binary expression to simple ones. \
For example, x = a + (b + c) will be transformed to \
tmp = b + c; x = a + tmp \n";

static RegisterTransformation<BinOpSimplification>
         Trans("binop-simplification", DescriptionMsg);

class BSCollectionVisitor : public RecursiveASTVisitor<BSCollectionVisitor> {
public:

  explicit BSCollectionVisitor(BinOpSimplification *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitFunctionDecl(FunctionDecl *FD);

private:

  BinOpSimplification *ConsumerInstance;
};

class BSStatementVisitor : public CommonStatementVisitor<BSStatementVisitor> {
public:

  explicit BSStatementVisitor(BinOpSimplification *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitBinaryOperator(BinaryOperator *BinOp);

private:
  void handleSubExpr(Expr *E);

  BinOpSimplification *ConsumerInstance;
};

bool BSCollectionVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  if (ConsumerInstance->isInIncludedFile(FD) ||
      !FD->isThisDeclarationADefinition())
    return true;

  ConsumerInstance->StmtVisitor->setCurrentFunctionDecl(FD);
  ConsumerInstance->StmtVisitor->TraverseDecl(FD);
  ConsumerInstance->StmtVisitor->setCurrentFunctionDecl(NULL);
  return true;
}

void BSStatementVisitor::handleSubExpr(Expr *E)
{
  BinaryOperator *BinOp = dyn_cast<BinaryOperator>(E->IgnoreParenCasts());
  if (!BinOp)
    return;

  TransAssert(std::find(ConsumerInstance->ValidBinOps.begin(), 
                        ConsumerInstance->ValidBinOps.end(), BinOp)
              == ConsumerInstance->ValidBinOps.end());

  ConsumerInstance->ValidBinOps.push_back(BinOp);
  ConsumerInstance->ValidInstanceNum++;

  if (ConsumerInstance->ValidInstanceNum == 
      ConsumerInstance->TransformationCounter) {
    ConsumerInstance->TheFuncDecl = CurrentFuncDecl;
    ConsumerInstance->TheStmt = CurrentStmt;
    ConsumerInstance->TheBinOp = BinOp;
    ConsumerInstance->NeedParen = NeedParen;
  }

  TraverseStmt(BinOp);
}

bool BSStatementVisitor::VisitBinaryOperator(BinaryOperator *BinOp) 
{
  if (BinOp->isAssignmentOp() && !BinOp->isCompoundAssignmentOp()) {
    Expr *RHS = BinOp->getRHS();
    return TraverseStmt(RHS);
  }

  Expr *LHS = BinOp->getLHS();
  handleSubExpr(LHS);
 
  Expr *RHS = BinOp->getRHS();
  handleSubExpr(RHS);

  return false;
}

void BinOpSimplification::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  BinOpCollectionVisitor = new BSCollectionVisitor(this);
  StmtVisitor = new BSStatementVisitor(this);
  NameQueryWrap = 
    new TransNameQueryWrap(RewriteHelper->getTmpVarNamePrefix());
}

bool BinOpSimplification::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    BinOpCollectionVisitor->TraverseDecl(*I);
  }
  return true;
}
 
void BinOpSimplification::HandleTranslationUnit(ASTContext &Ctx)
{
  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  TransAssert(TheFuncDecl && "NULL TheFuncDecl!");
  TransAssert(TheStmt && "NULL TheStmt!");
  TransAssert(TheBinOp && "NULL TheBinOp");

  NameQueryWrap->TraverseDecl(Ctx.getTranslationUnitDecl());
  addNewTmpVariable();
  addNewAssignStmt();
  replaceBinOp();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

bool BinOpSimplification::addNewTmpVariable(void)
{
  QualType QT = TheBinOp->getType();
  std::string VarStr;
  std::stringstream SS;
  unsigned int NamePostfix = NameQueryWrap->getMaxNamePostfix();

  SS << RewriteHelper->getTmpVarNamePrefix() << (NamePostfix + 1);
  VarStr = SS.str();
  setTmpVarName(VarStr);

  QT.getAsStringInternal(VarStr,
                         Context->getPrintingPolicy());

  VarStr += ";";
  return RewriteHelper->addLocalVarToFunc(VarStr, TheFuncDecl);
}

bool BinOpSimplification::addNewAssignStmt(void)
{
  return RewriteHelper->addNewAssignStmtBefore(TheStmt,
                                              getTmpVarName(),
                                              TheBinOp, 
                                              NeedParen);
}

bool BinOpSimplification::replaceBinOp(void)
{
  return RewriteHelper->replaceExpr(TheBinOp, TmpVarName);
}

BinOpSimplification::~BinOpSimplification(void)
{
  delete BinOpCollectionVisitor;
  delete StmtVisitor;
  delete NameQueryWrap;
}
