//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See LICENSE.TXT for details.
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

class SimplifyCommaExpr : public Transformation {
friend class SimplifyCommaExprCollectionVisitor;

public:

  SimplifyCommaExpr(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      TheBinaryOperator(NULL),
      TheStmt(NULL),
      NeedParen(false)
  { }

  ~SimplifyCommaExpr(void);

private:
  
  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void simplifyCommaExpr(void);

  SimplifyCommaExprCollectionVisitor *CollectionVisitor;

  clang::BinaryOperator *TheBinaryOperator;

  clang::Stmt *TheStmt;

  bool NeedParen;

  // Unimplemented
  SimplifyCommaExpr(void);

  SimplifyCommaExpr(const SimplifyCommaExpr &);

  void operator=(const SimplifyCommaExpr &);
};
#endif
