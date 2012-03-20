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

#include "SimplifyCommaExpr.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Simplify a comma expression. It transforms the following code: \n\
  (x, y); \n\
to \n\
  x; \n\
  (y); \n";

static RegisterTransformation<SimplifyCommaExpr>
         Trans("simplify-comma-expr", DescriptionMsg);

class SimplifyCommaExprCollectionVisitor : public 
        RecursiveASTVisitor<SimplifyCommaExprCollectionVisitor> {
public:

  explicit SimplifyCommaExprCollectionVisitor(SimplifyCommaExpr *Instance)
    : ConsumerInstance(Instance),
      CurrentStmt(NULL),
      NeedParen(false)
  { }

  bool VisitCompoundStmt(CompoundStmt *S);

  bool VisitBinaryOperator(BinaryOperator *BO);

  bool VisitForStmt(ForStmt *FS);

  bool VisitIfStmt(IfStmt *IS);

  bool VisitWhileStmt(WhileStmt *WS);

  bool VisitDoStmt(DoStmt *DS);

  bool VisitCaseStmt(CaseStmt *CS);

  bool VisitDefaultStmt(DefaultStmt *DS);

  void visitNonCompoundStmt(Stmt *S);

private:

  SimplifyCommaExpr *ConsumerInstance;

  Stmt *CurrentStmt;

  bool NeedParen;
};

bool SimplifyCommaExprCollectionVisitor::VisitCompoundStmt(CompoundStmt *CS)
{
  for (CompoundStmt::body_iterator I = CS->body_begin(),
       E = CS->body_end(); I != E; ++I) {
    CurrentStmt = (*I);
    TraverseStmt(*I);
  }
  return false;
}

void SimplifyCommaExprCollectionVisitor::visitNonCompoundStmt(Stmt *S)
{
  if (!S)
    return;

  CompoundStmt *CS = dyn_cast<CompoundStmt>(S);
  if (CS) {
    VisitCompoundStmt(CS);
    return;
  }

  CurrentStmt = S;
  NeedParen = true;
  TraverseStmt(S);
  NeedParen = false;
}

// It is used to handle the case where if-then or else branch
// is not treated as a CompoundStmt. So it cannot be traversed
// from VisitCompoundStmt, e.g.,
//   if (x)
//     foo(bar())
bool SimplifyCommaExprCollectionVisitor::VisitIfStmt(IfStmt *IS)
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
bool SimplifyCommaExprCollectionVisitor::VisitForStmt(ForStmt *FS)
{
  Stmt *Body = FS->getBody();
  visitNonCompoundStmt(Body);
  return false;
}

bool SimplifyCommaExprCollectionVisitor::VisitWhileStmt(WhileStmt *WS)
{
  Stmt *Body = WS->getBody();
  visitNonCompoundStmt(Body);
  return false;
}

bool SimplifyCommaExprCollectionVisitor::VisitDoStmt(DoStmt *DS)
{
  Stmt *Body = DS->getBody();
  visitNonCompoundStmt(Body);
  return false;
}

bool SimplifyCommaExprCollectionVisitor::VisitCaseStmt(CaseStmt *CS)
{
  Stmt *Body = CS->getSubStmt();
  visitNonCompoundStmt(Body);
  return false;
}

bool SimplifyCommaExprCollectionVisitor::VisitDefaultStmt(DefaultStmt *DS)
{
  Stmt *Body = DS->getSubStmt();
  visitNonCompoundStmt(Body);
  return false;
}

bool SimplifyCommaExprCollectionVisitor::VisitBinaryOperator(
       BinaryOperator *BO)
{
  BinaryOperator::Opcode Op = BO->getOpcode();
  if (Op == clang::BO_Comma) {
    ConsumerInstance->ValidInstanceNum++;
    if (ConsumerInstance->ValidInstanceNum == 
        ConsumerInstance->TransformationCounter) {
      ConsumerInstance->TheBinaryOperator = BO;
      ConsumerInstance->TheStmt = CurrentStmt;
      ConsumerInstance->NeedParen = NeedParen;
    }
  }

  Expr *LHS = BO->getLHS();
  TraverseStmt(LHS);
 
  Expr *RHS = BO->getRHS();
  TraverseStmt(RHS);

  return false;
}

void SimplifyCommaExpr::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new SimplifyCommaExprCollectionVisitor(this);
}

bool SimplifyCommaExpr::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    FunctionDecl *FD = dyn_cast<FunctionDecl>(*I);
    if (FD && FD->isThisDeclarationADefinition()) {
      CollectionVisitor->TraverseDecl(FD);
    }
  }
  return true;
}
 
void SimplifyCommaExpr::HandleTranslationUnit(ASTContext &Ctx)
{
  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  TransAssert(TheStmt && "NULL TheStmt!");
  TransAssert(TheBinaryOperator && "NULL TheBinaryOperator");

  simplifyCommaExpr();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void SimplifyCommaExpr::simplifyCommaExpr(void)
{
  TransAssert((TheBinaryOperator->getOpcode() == clang::BO_Comma) && 
              "Non Comma Operator!");
  const Expr *LHS = TheBinaryOperator->getLHS();
  std::string LHSStr;
  RewriteHelper->getExprString(LHS, LHSStr);

  SourceRange LHSRange = LHS->getSourceRange();
  SourceLocation StartLoc = LHSRange.getBegin();
  SourceLocation EndLoc = RewriteHelper->getEndLocationUntil(LHSRange, ',');
  TheRewriter.RemoveText(SourceRange(StartLoc, EndLoc));

  LHSStr += ";";
  RewriteHelper->addStringBeforeStmt(TheStmt, LHSStr, NeedParen);
}

SimplifyCommaExpr::~SimplifyCommaExpr(void)
{
  if (CollectionVisitor)
    delete CollectionVisitor;
}

