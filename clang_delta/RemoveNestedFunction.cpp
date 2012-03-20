//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "RemoveNestedFunction.h"

#include <sstream>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Remove a nested function invocation from its enclosing \
expression. The transformation will create a temporary \
variable with the correct type, assgin the return value \
of the selected nested function to the created variable, \
and replace the function invocation with this temporary \
variable. (Note that this transformation is unsound because \
it changes the semantics of loop executions in some cases. \n";


static RegisterTransformation<RemoveNestedFunction>
         Trans("remove-nested-function", DescriptionMsg);

class RNFCollectionVisitor : public RecursiveASTVisitor<RNFCollectionVisitor> {
public:

  typedef RecursiveASTVisitor<RNFCollectionVisitor> Super;

  explicit RNFCollectionVisitor(RemoveNestedFunction *Instance)
    : ConsumerInstance(Instance),
      CurrentFuncDecl(NULL),
      CurrentStmt(NULL),
      NeedParen(false)
  { }

  bool VisitCompoundStmt(CompoundStmt *S);

  bool VisitIfStmt(IfStmt *IS);

  bool VisitForStmt(ForStmt *FS);

  bool VisitWhileStmt(WhileStmt *WS);

  bool VisitDoStmt(DoStmt *DS);

  bool VisitCaseStmt(CaseStmt *CS);

  bool VisitDefaultStmt(DefaultStmt *DS);

  void visitNonCompoundStmt(Stmt *S);

  bool VisitCallExpr(CallExpr *CallE);

  void setCurrentFuncDecl(FunctionDecl *FD) {
    CurrentFuncDecl = FD;
  }

private:

  RemoveNestedFunction *ConsumerInstance;

  FunctionDecl *CurrentFuncDecl;

  Stmt *CurrentStmt;

  bool NeedParen;

};

bool RNFCollectionVisitor::VisitCompoundStmt(CompoundStmt *CS)
{
  for (CompoundStmt::body_iterator I = CS->body_begin(),
       E = CS->body_end(); I != E; ++I) {
    CurrentStmt = (*I);
    TraverseStmt(*I);
  }
  return false;
}

void RNFCollectionVisitor::visitNonCompoundStmt(Stmt *S)
{
  if (!S)
    return;

  CompoundStmt *CS = dyn_cast<CompoundStmt>(S);
  if (CS) {
    VisitCompoundStmt(CS);
    return;
  }

  CurrentStmt = (S);
  NeedParen = true;
  TraverseStmt(S);
  NeedParen = false;
}

// It is used to handle the case where if-then or else branch
// is not treated as a CompoundStmt. So it cannot be traversed
// from VisitCompoundStmt, e.g.,
//   if (x)
//     foo(bar())
bool RNFCollectionVisitor::VisitIfStmt(IfStmt *IS)
{
  Expr *E = IS->getCond();
  TraverseStmt(E);

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
bool RNFCollectionVisitor::VisitForStmt(ForStmt *FS)
{
  Stmt *Init = FS->getInit();
  TraverseStmt(Init);

  Expr *Cond = FS->getCond();
  TraverseStmt(Cond);

  Expr *Inc = FS->getInc();
  TraverseStmt(Inc);

  Stmt *Body = FS->getBody();
  visitNonCompoundStmt(Body);
  return false;
}

bool RNFCollectionVisitor::VisitWhileStmt(WhileStmt *WS)
{
  Expr *E = WS->getCond();
  TraverseStmt(E);

  Stmt *Body = WS->getBody();
  visitNonCompoundStmt(Body);
  return false;
}

bool RNFCollectionVisitor::VisitDoStmt(DoStmt *DS)
{
  Expr *E = DS->getCond();
  TraverseStmt(E);

  Stmt *Body = DS->getBody();
  visitNonCompoundStmt(Body);
  return false;
}

bool RNFCollectionVisitor::VisitCaseStmt(CaseStmt *CS)
{
  Stmt *Body = CS->getSubStmt();
  visitNonCompoundStmt(Body);
  return false;
}

bool RNFCollectionVisitor::VisitDefaultStmt(DefaultStmt *DS)
{
  Stmt *Body = DS->getSubStmt();
  visitNonCompoundStmt(Body);
  return false;
}

bool RNFCollectionVisitor::VisitCallExpr(CallExpr *CallE) 
{
  if ((std::find(ConsumerInstance->ValidCallExprs.begin(), 
                 ConsumerInstance->ValidCallExprs.end(), CallE) 
          == ConsumerInstance->ValidCallExprs.end()) && 
      !ConsumerInstance->CallExprQueue.empty()) {

    ConsumerInstance->ValidInstanceNum++;
    ConsumerInstance->ValidCallExprs.push_back(CallE);

    if (ConsumerInstance->ValidInstanceNum == 
        ConsumerInstance->TransformationCounter) {
      ConsumerInstance->TheFuncDecl = CurrentFuncDecl;
      ConsumerInstance->TheStmt = CurrentStmt;
      ConsumerInstance->TheCallExpr = CallE;
      ConsumerInstance->NeedParen = NeedParen;
    }
  }

  ConsumerInstance->CallExprQueue.push_back(CallE);

  for (CallExpr::arg_iterator I = CallE->arg_begin(),
       E = CallE->arg_end(); I != E; ++I) {
      TraverseStmt(*I);
  }

  ConsumerInstance->CallExprQueue.pop_back();

  return true;
}

void RemoveNestedFunction::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  NestedInvocationVisitor = new RNFCollectionVisitor(this);
  NameQueryWrap = 
    new TransNameQueryWrap(RewriteHelper->getTmpVarNamePrefix());
}

bool RemoveNestedFunction::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    FunctionDecl *FD = dyn_cast<FunctionDecl>(*I);
    if (FD && FD->isThisDeclarationADefinition()) {
      NestedInvocationVisitor->setCurrentFuncDecl(FD);
      NestedInvocationVisitor->TraverseDecl(FD);
      NestedInvocationVisitor->setCurrentFuncDecl(NULL);
    }
  }
  return true;
}
 
void RemoveNestedFunction::HandleTranslationUnit(ASTContext &Ctx)
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
  TransAssert(TheCallExpr && "NULL TheCallExpr");

  NameQueryWrap->TraverseDecl(Ctx.getTranslationUnitDecl());

  addNewTmpVariable();
  addNewAssignStmt();
  replaceCallExpr();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

bool RemoveNestedFunction::addNewTmpVariable(void)
{
  QualType QT = TheCallExpr->getCallReturnType();
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

bool RemoveNestedFunction::addNewAssignStmt(void)
{
  return RewriteHelper->addNewAssignStmtBefore(TheStmt,
                                              getTmpVarName(),
                                              TheCallExpr, 
                                              NeedParen);

}

bool RemoveNestedFunction::replaceCallExpr(void)
{
  return RewriteHelper->replaceExpr(TheCallExpr, TmpVarName);
}

RemoveNestedFunction::~RemoveNestedFunction(void)
{
  if (NestedInvocationVisitor)
    delete NestedInvocationVisitor;

  if (NameQueryWrap)
    delete NameQueryWrap;
}

