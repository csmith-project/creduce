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

#include "SimplifyIf.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"
#include "CommonStatementVisitor.h"

using namespace clang;

static const char *DescriptionMsg =
"Simplify an if-else statement. It transforms the following code: \n\
  if (guard1) \n\
  {... } \n\
  else if (guard2) \n\
  else \n\
  {...} \n\
to \n\
  (guard1) \n\
  {... } \n\
  if (guard2) \n\
  else \n\
  {...} \n\
if there is no else-if left, the last else keyword will be removed. \n";  

class SimplifyIfCollectionVisitor : public 
        RecursiveASTVisitor<SimplifyIfCollectionVisitor> {
public:

  explicit SimplifyIfCollectionVisitor(SimplifyIf *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitFunctionDecl(FunctionDecl *FD);

private:
  SimplifyIf *ConsumerInstance;
  
};

static RegisterTransformation<SimplifyIf>
         Trans("simplify-if", DescriptionMsg);

class SimplifyIfStatementVisitor : public 
        CommonStatementVisitor<SimplifyIfStatementVisitor> {
public:

  explicit SimplifyIfStatementVisitor(SimplifyIf *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitIfStmt(IfStmt *IS);

  bool VisitForStmt(ForStmt *FS);

  bool VisitWhileStmt(WhileStmt *WS);

  bool VisitDoStmt(DoStmt *DS);

private:

  SimplifyIf *ConsumerInstance;

};

bool SimplifyIfCollectionVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  if (ConsumerInstance->isInIncludedFile(FD) ||
      !FD->isThisDeclarationADefinition())
    return true;

  ConsumerInstance->StmtVisitor->TraverseDecl(FD);
  return true;
}

// It is used to handle the case where if-then or else branch
// is not treated as a CompoundStmt. So it cannot be traversed
// from VisitCompoundStmt, e.g.,
//   if (x)
//     foo(bar())
bool SimplifyIfStatementVisitor::VisitIfStmt(IfStmt *IS)
{
  if (IS->getLocStart().isMacroID()) {
    return false;
  }

  ConsumerInstance->ValidInstanceNum++;
  if (ConsumerInstance->ValidInstanceNum == 
      ConsumerInstance->TransformationCounter) {
    ConsumerInstance->TheIfStmt = IS;
    ConsumerInstance->NeedParen = NeedParen;
  }

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
bool SimplifyIfStatementVisitor::VisitForStmt(ForStmt *FS)
{
  Stmt *Body = FS->getBody();
  visitNonCompoundStmt(Body);
  return false;
}

bool SimplifyIfStatementVisitor::VisitWhileStmt(WhileStmt *WS)
{
  Stmt *Body = WS->getBody();
  visitNonCompoundStmt(Body);
  return false;
}

bool SimplifyIfStatementVisitor::VisitDoStmt(DoStmt *DS)
{
  Stmt *Body = DS->getBody();
  visitNonCompoundStmt(Body);
  return false;
}

void SimplifyIf::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new SimplifyIfCollectionVisitor(this);
  StmtVisitor = new SimplifyIfStatementVisitor(this);
}

bool SimplifyIf::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    CollectionVisitor->TraverseDecl(*I);
  }
  return true;
}
 
void SimplifyIf::HandleTranslationUnit(ASTContext &Ctx)
{
  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  TransAssert(TheIfStmt && "NULL TheIfStmt");

  simplifyIfStmt();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void SimplifyIf::simplifyIfStmt(void)
{
  const Expr *Cond = TheIfStmt->getCond();
  TransAssert(Cond && "Bad Cond Expr!");
  std::string CondStr;
  RewriteHelper->getExprString(Cond, CondStr);
  CondStr += ";";
  RewriteHelper->addStringBeforeStmt(TheIfStmt, CondStr, NeedParen);

  RewriteHelper->removeIfAndCond(TheIfStmt);

  const Stmt *ElseS = TheIfStmt->getElse();
  if (ElseS) {
    SourceLocation ElseLoc = TheIfStmt->getElseLoc();
    std::string ElseStr = "else";
    TheRewriter.RemoveText(ElseLoc, ElseStr.size());
  }
}

SimplifyIf::~SimplifyIf(void)
{
  delete CollectionVisitor;
  delete StmtVisitor;
}

