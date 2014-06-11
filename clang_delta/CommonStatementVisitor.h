//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef COMMON_STATEMENT_VISITOR_H
#define COMMON_STATEMENT_VISITOR_H

#include "clang/AST/RecursiveASTVisitor.h"

template<typename T>
class CommonStatementVisitor : public clang::RecursiveASTVisitor<T> {
public:

  CommonStatementVisitor()
    : CurrentFuncDecl(NULL),
      CurrentStmt(NULL),
      NeedParen(false)
  { }

  T &getDerived() { return *static_cast<T*>(this); };

  void setCurrentFunctionDecl(clang::FunctionDecl *FD) {
    CurrentFuncDecl = FD;
  }

  bool VisitCompoundStmt(clang::CompoundStmt *S);

  bool VisitIfStmt(clang::IfStmt *IS);

  bool VisitForStmt(clang::ForStmt *FS);

  bool VisitWhileStmt(clang::WhileStmt *WS);

  bool VisitDoStmt(clang::DoStmt *DS);

  bool VisitCaseStmt(clang::CaseStmt *CS);

  bool VisitDefaultStmt(clang::DefaultStmt *DS);

  bool VisitCXXTryStmt(clang::CXXTryStmt *DS);

  void visitNonCompoundStmt(clang::Stmt *S);

protected:

  clang::FunctionDecl *CurrentFuncDecl;

  clang::Stmt *CurrentStmt;

  bool NeedParen;

};

template<typename T>
bool CommonStatementVisitor<T>::VisitCompoundStmt(clang::CompoundStmt *CS)
{
  for (clang::CompoundStmt::body_iterator I = CS->body_begin(),
       E = CS->body_end(); I != E; ++I) {
    CurrentStmt = (*I);
    getDerived().TraverseStmt(*I);
  }
  return false;
}

template<typename T>
void CommonStatementVisitor<T>::visitNonCompoundStmt(clang::Stmt *S)
{
  if (!S)
    return;

  clang::CompoundStmt *CS = llvm::dyn_cast<clang::CompoundStmt>(S);
  if (CS) {
    VisitCompoundStmt(CS);
    return;
  }

  CurrentStmt = (S);
  NeedParen = true;
  getDerived().TraverseStmt(S);
  NeedParen = false;
}

// It is used to handle the case where if-then or else branch
// is not treated as a CompoundStmt. So it cannot be traversed
// from VisitCompoundStmt, e.g.,
//   if (x)
//     foo(bar())
template<typename T>
bool CommonStatementVisitor<T>::VisitIfStmt(clang::IfStmt *IS)
{
  clang::Expr *E = IS->getCond();
  getDerived().TraverseStmt(E);

  clang::Stmt *ThenB = IS->getThen();
  visitNonCompoundStmt(ThenB);

  clang::Stmt *ElseB = IS->getElse();
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
template<typename T>
bool CommonStatementVisitor<T>::VisitForStmt(clang::ForStmt *FS)
{
  clang::Stmt *Init = FS->getInit();
  getDerived().TraverseStmt(Init);

  clang::Expr *Cond = FS->getCond();
  getDerived().TraverseStmt(Cond);

  clang::Expr *Inc = FS->getInc();
  getDerived().TraverseStmt(Inc);

  clang::Stmt *Body = FS->getBody();
  visitNonCompoundStmt(Body);
  return false;
}

template<typename T>
bool CommonStatementVisitor<T>::VisitWhileStmt(clang::WhileStmt *WS)
{
  clang::Expr *E = WS->getCond();
  getDerived().TraverseStmt(E);

  clang::Stmt *Body = WS->getBody();
  visitNonCompoundStmt(Body);
  return false;
}

template<typename T>
bool CommonStatementVisitor<T>::VisitDoStmt(clang::DoStmt *DS)
{
  clang::Expr *E = DS->getCond();
  getDerived().TraverseStmt(E);

  clang::Stmt *Body = DS->getBody();
  visitNonCompoundStmt(Body);
  return false;
}

template<typename T>
bool CommonStatementVisitor<T>::VisitCaseStmt(clang::CaseStmt *CS)
{
  clang::Stmt *Body = CS->getSubStmt();
  visitNonCompoundStmt(Body);
  return false;
}

template<typename T>
bool CommonStatementVisitor<T>::VisitDefaultStmt(clang::DefaultStmt *DS)
{
  clang::Stmt *Body = DS->getSubStmt();
  visitNonCompoundStmt(Body);
  return false;
}

template<typename T>
bool CommonStatementVisitor<T>::VisitCXXTryStmt(clang::CXXTryStmt *CS)
{
  clang::CompoundStmt *TryBlock = CS->getTryBlock();
  visitNonCompoundStmt(TryBlock);

  for (unsigned I = 0; I < CS->getNumHandlers(); ++I) {
    clang::CXXCatchStmt *CatchStmt = CS->getHandler(I);
    clang::Stmt *CatchBlock = CatchStmt->getHandlerBlock();
    visitNonCompoundStmt(CatchBlock);
  }
  return false;
}

#endif
