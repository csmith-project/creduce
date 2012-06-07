//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef SIMPLIFY_COMMA_EXPR_H
#define SIMPLIFY_COMMA_EXPR_H

#include <string>
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class Stmt;
  class BinaryOperator;
}

class SimplifyCommaExprCollectionVisitor;
class SimplifyCommaExprStmtVisitor;

class SimplifyCommaExpr : public Transformation {
friend class SimplifyCommaExprCollectionVisitor;
friend class SimplifyCommaExprStmtVisitor;

public:

  SimplifyCommaExpr(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      StmtVisitor(NULL),
      TheBinaryOperator(NULL),
      TheStmt(NULL),
      NeedParen(false)
  { }

  ~SimplifyCommaExpr(void);

private:
  
  virtual void Initialize(clang::ASTContext &context);

  virtual bool HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void simplifyCommaExpr(void);

  SimplifyCommaExprCollectionVisitor *CollectionVisitor;

  SimplifyCommaExprStmtVisitor *StmtVisitor;

  clang::BinaryOperator *TheBinaryOperator;

  clang::Stmt *TheStmt;

  bool NeedParen;

  // Unimplemented
  SimplifyCommaExpr(void);

  SimplifyCommaExpr(const SimplifyCommaExpr &);

  void operator=(const SimplifyCommaExpr &);
};
#endif
