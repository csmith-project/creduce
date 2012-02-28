//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "BinOpSimplification.h"

#include <sstream>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"

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

  typedef RecursiveASTVisitor<BSCollectionVisitor> Super;

  explicit BSCollectionVisitor(BinOpSimplification *Instance)
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

  bool VisitBinaryOperator(BinaryOperator *BinOp);

  void setCurrentFuncDecl(FunctionDecl *FD) {
    CurrentFuncDecl = FD;
  }

private:

  BinOpSimplification *ConsumerInstance;

  FunctionDecl *CurrentFuncDecl;

  Stmt *CurrentStmt;

  bool NeedParen;

  void handleSubExpr(Expr *E);

};

bool BSCollectionVisitor::VisitCompoundStmt(CompoundStmt *CS)
{
  for (CompoundStmt::body_iterator I = CS->body_begin(),
       E = CS->body_end(); I != E; ++I) {
    CurrentStmt = (*I);
    TraverseStmt(*I);
  }
  return false;
}

void BSCollectionVisitor::visitNonCompoundStmt(Stmt *S)
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
bool BSCollectionVisitor::VisitIfStmt(IfStmt *IS)
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
bool BSCollectionVisitor::VisitForStmt(ForStmt *FS)
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

bool BSCollectionVisitor::VisitWhileStmt(WhileStmt *WS)
{
  Expr *E = WS->getCond();
  TraverseStmt(E);

  Stmt *Body = WS->getBody();
  visitNonCompoundStmt(Body);
  return false;
}

bool BSCollectionVisitor::VisitDoStmt(DoStmt *DS)
{
  Expr *E = DS->getCond();
  TraverseStmt(E);

  Stmt *Body = DS->getBody();
  visitNonCompoundStmt(Body);
  return false;
}

bool BSCollectionVisitor::VisitCaseStmt(CaseStmt *CS)
{
  Stmt *Body = CS->getSubStmt();
  visitNonCompoundStmt(Body);
  return false;
}

bool BSCollectionVisitor::VisitDefaultStmt(DefaultStmt *DS)
{
  Stmt *Body = DS->getSubStmt();
  visitNonCompoundStmt(Body);
  return false;
}

void BSCollectionVisitor::handleSubExpr(Expr *E)
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

bool BSCollectionVisitor::VisitBinaryOperator(BinaryOperator *BinOp) 
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
  NameQueryWrap = 
    new TransNameQueryWrap(RewriteHelper->getTmpVarNamePrefix());
}

void BinOpSimplification::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    FunctionDecl *FD = dyn_cast<FunctionDecl>(*I);
    if (FD && FD->isThisDeclarationADefinition()) {
      BinOpCollectionVisitor->setCurrentFuncDecl(FD);
      BinOpCollectionVisitor->TraverseDecl(FD);
      BinOpCollectionVisitor->setCurrentFuncDecl(NULL);
    }
  }
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
  if (BinOpCollectionVisitor)
    delete BinOpCollectionVisitor;

  if (NameQueryWrap)
    delete NameQueryWrap;
}
