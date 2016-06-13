//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2015, 2016 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "LiftAssignmentExpr.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"
#include "CommonStatementVisitor.h"

using namespace clang;

static const char *DescriptionMsg =
"Lift an assignment expression to an assignment statement. \
An assignment expression will be lifted if it apprears at : \n\
  * the guard position of an if, while or do statement; \n\
  * function argument; \n\
  * init, condition or inc position of a for statement \n";

static RegisterTransformation<LiftAssignmentExpr>
         Trans("lift-assignment-expr", DescriptionMsg);

class AssignExprCollectionVisitor : public 
        RecursiveASTVisitor<AssignExprCollectionVisitor> {
public:

  explicit AssignExprCollectionVisitor(LiftAssignmentExpr *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitFunctionDecl(FunctionDecl *FD);

private:
  LiftAssignmentExpr *ConsumerInstance;
  
};

class AssignExprStatementVisitor : public 
        CommonStatementVisitor<AssignExprStatementVisitor> {
public:

  explicit AssignExprStatementVisitor(LiftAssignmentExpr *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitIfStmt(IfStmt *IS);

  bool VisitForStmt(ForStmt *FS);

  bool VisitWhileStmt(WhileStmt *WS);

  bool VisitDoStmt(DoStmt *DS);

  bool VisitCallExpr(CallExpr *CallE);

private:

  LiftAssignmentExpr *ConsumerInstance;

  void handleSubExpr(Expr *E);

};

bool AssignExprCollectionVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  if (!FD->isThisDeclarationADefinition() ||
      ConsumerInstance->isInIncludedFile(FD))
    return true;

  ConsumerInstance->StmtVisitor->setCurrentFunctionDecl(FD);
  ConsumerInstance->StmtVisitor->TraverseDecl(FD);
  ConsumerInstance->StmtVisitor->setCurrentFunctionDecl(NULL);
  return true;
}

// It is used to handle the case where if-then or else branch
// is not treated as a CompoundStmt. So it cannot be traversed
// from VisitCompoundStmt, e.g.,
//   if (x)
//     foo(bar())
bool AssignExprStatementVisitor::VisitIfStmt(IfStmt *IS)
{
  Expr *E = IS->getCond();
  handleSubExpr(E);

  Stmt *ThenB = IS->getThen();
  visitNonCompoundStmt(ThenB);

  Stmt *ElseB = IS->getElse();
  visitNonCompoundStmt(ElseB);

  return false;
}

// It causes unsound transformation because 
// the semantics of loop execution has been changed. 
// For example,
//   int foo(int x)
//   {
//     int i;
//     for(i = 0; i < bar(bar(x)); i++)
//       ...
//   }
// will be transformed to:
//   int foo(int x)
//   {
//     int i;
//     int tmp_var = bar(x);
//     for(i = 0; i < bar(tmp_var); i++)
//       ...
//   }
bool AssignExprStatementVisitor::VisitForStmt(ForStmt *FS)
{
  Stmt *Init = FS->getInit();
  Expr *InitE = NULL;
  if (Init)
    InitE = dyn_cast<Expr>(Init);

  if (InitE)
    handleSubExpr(InitE);
  else  
    TraverseStmt(Init);

  Expr *Cond = FS->getCond();
  handleSubExpr(Cond);

  Expr *Inc = FS->getInc();
  handleSubExpr(Inc);

  Stmt *Body = FS->getBody();
  visitNonCompoundStmt(Body);
  return false;
}

bool AssignExprStatementVisitor::VisitWhileStmt(WhileStmt *WS)
{
  Expr *E = WS->getCond();
  handleSubExpr(E);

  Stmt *Body = WS->getBody();
  visitNonCompoundStmt(Body);
  return false;
}

bool AssignExprStatementVisitor::VisitDoStmt(DoStmt *DS)
{
  Expr *E = DS->getCond();
  handleSubExpr(E);

  Stmt *Body = DS->getBody();
  visitNonCompoundStmt(Body);
  return false;
}

bool AssignExprStatementVisitor::VisitCallExpr(CallExpr *CallE) 
{
  for (CallExpr::arg_iterator I = CallE->arg_begin(),
       E = CallE->arg_end(); I != E; ++I) {
    Expr *Exp = *I;
    handleSubExpr(Exp);
  }
  return false;
}

void AssignExprStatementVisitor::handleSubExpr(Expr *E)
{
  if (!E)
    return;

  BinaryOperator *BinOp = dyn_cast<BinaryOperator>(E->IgnoreParenCasts());
  if (!BinOp) {
    TraverseStmt(E);
    return;
  }

  if (!BinOp->isAssignmentOp() && !BinOp->isCompoundAssignmentOp()) {
    TraverseStmt(E);
    return;
  }

  TransAssert(std::find(ConsumerInstance->ValidAssignExprs.begin(), 
                        ConsumerInstance->ValidAssignExprs.end(), BinOp)
              == ConsumerInstance->ValidAssignExprs.end());

  ConsumerInstance->ValidAssignExprs.push_back(BinOp);
  ConsumerInstance->ValidInstanceNum++;

  if (ConsumerInstance->ValidInstanceNum != 
      ConsumerInstance->TransformationCounter)
    return;

  ConsumerInstance->TheFuncDecl = CurrentFuncDecl;
  ConsumerInstance->TheStmt = CurrentStmt;
  ConsumerInstance->TheAssignExpr = BinOp;
  ConsumerInstance->NeedParen = NeedParen;
}

void LiftAssignmentExpr::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new AssignExprCollectionVisitor(this);
  StmtVisitor = new AssignExprStatementVisitor(this);
}

bool LiftAssignmentExpr::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    CollectionVisitor->TraverseDecl(*I);
  }
  return true;
}
 
void LiftAssignmentExpr::HandleTranslationUnit(ASTContext &Ctx)
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
  TransAssert(TheAssignExpr && "NULL TheAssignExpr");

  TransAssert(TheAssignExpr->isAssignmentOp() ||
              TheAssignExpr->isCompoundAssignmentOp());
  addNewAssignStmt();
  replaceAssignExpr();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

bool LiftAssignmentExpr::addNewAssignStmt(void)
{
  std::string AssignStr("");
  RewriteHelper->getExprString(TheAssignExpr, AssignStr);
  AssignStr += ";";
  return RewriteHelper->addStringBeforeStmt(TheStmt, AssignStr, NeedParen);
}

bool LiftAssignmentExpr::replaceAssignExpr(void)
{
  const Expr *Lhs = TheAssignExpr->getLHS();
  std::string LhsStr("");
  RewriteHelper->getExprString(Lhs, LhsStr);
  return RewriteHelper->replaceExpr(TheAssignExpr, LhsStr);
}

LiftAssignmentExpr::~LiftAssignmentExpr(void)
{
  delete CollectionVisitor;
  delete StmtVisitor;
}
