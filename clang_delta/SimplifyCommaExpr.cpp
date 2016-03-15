//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2014, 2015 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "SimplifyCommaExpr.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"
#include "CommonStatementVisitor.h"

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
    : ConsumerInstance(Instance)
  { }

  bool VisitFunctionDecl(FunctionDecl *FD);

private:
  SimplifyCommaExpr *ConsumerInstance;
};

class SimplifyCommaExprStmtVisitor : public 
        CommonStatementVisitor<SimplifyCommaExprStmtVisitor> {
public:

  explicit SimplifyCommaExprStmtVisitor(SimplifyCommaExpr *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitForStmt(ForStmt *FS);

  bool VisitWhileStmt(WhileStmt *WS);

  bool VisitDoStmt(DoStmt *DS);

  bool VisitBinaryOperator(BinaryOperator *BO);

private:

  SimplifyCommaExpr *ConsumerInstance;
};

bool SimplifyCommaExprCollectionVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  if (ConsumerInstance->isInIncludedFile(FD) ||
      !FD->isThisDeclarationADefinition())
    return true;

  ConsumerInstance->StmtVisitor->TraverseDecl(FD);
  return true;
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
bool SimplifyCommaExprStmtVisitor::VisitForStmt(ForStmt *FS)
{
  Stmt *Body = FS->getBody();
  visitNonCompoundStmt(Body);
  return false;
}

bool SimplifyCommaExprStmtVisitor::VisitWhileStmt(WhileStmt *WS)
{
  Stmt *Body = WS->getBody();
  visitNonCompoundStmt(Body);
  return false;
}

bool SimplifyCommaExprStmtVisitor::VisitDoStmt(DoStmt *DS)
{
  Stmt *Body = DS->getBody();
  visitNonCompoundStmt(Body);
  return false;
}

bool SimplifyCommaExprStmtVisitor::VisitBinaryOperator(
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
  StmtVisitor = new SimplifyCommaExprStmtVisitor(this);
}

bool SimplifyCommaExpr::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    CollectionVisitor->TraverseDecl(*I);
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
  SourceLocation EndLoc;
  if (StartLoc.isMacroID()) {
    StartLoc = SrcManager->getFileLoc(StartLoc);
    EndLoc = LHSRange.getEnd();
    TransAssert(EndLoc.isMacroID() && "EndLoc is not from a macro!");
    LHSRange = SourceRange(StartLoc, SrcManager->getFileLoc(EndLoc));
  }
  EndLoc = RewriteHelper->getEndLocationUntil(LHSRange, ',');
  TheRewriter.RemoveText(SourceRange(StartLoc, EndLoc));

  LHSStr += ";";
  RewriteHelper->addStringBeforeStmt(TheStmt, LHSStr, NeedParen);
}

SimplifyCommaExpr::~SimplifyCommaExpr(void)
{
  delete CollectionVisitor;
  delete StmtVisitor;
}

