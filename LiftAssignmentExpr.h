//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LIFT_ASSIGNMENT_EXPR_H
#define LIFT_ASSIGNMENT_EXPR_H

#include <string>
#include "llvm/ADT/SmallVector.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class FunctionDecl;
  class Stmt;
  class BinaryOperator;
}

class AssignExprCollectionVisitor;

class LiftAssignmentExpr : public Transformation {
friend class AssignExprCollectionVisitor;

public:

  LiftAssignmentExpr(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      TheFuncDecl(NULL),
      TheStmt(NULL),
      TheAssignExpr(NULL),
      NeedParen(false)
  { }

  ~LiftAssignmentExpr(void);

private:
  
  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  bool addNewAssignStmt(void);

  bool replaceAssignExpr(void);

  clang::SmallVector<clang::BinaryOperator *, 20> ValidAssignExprs;

  AssignExprCollectionVisitor *CollectionVisitor;

  clang::FunctionDecl *TheFuncDecl;

  clang::Stmt *TheStmt;

  clang::BinaryOperator *TheAssignExpr;

  bool NeedParen;

  // Unimplemented
  LiftAssignmentExpr(void);

  LiftAssignmentExpr(const LiftAssignmentExpr &);

  void operator=(const LiftAssignmentExpr &);
};
#endif
