//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef SIMPLIFY_CALL_EXPR_H
#define SIMPLIFY_CALL_EXPR_H

#include <string>
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class FunctionDecl;
  class CallExpr;
  class Expr;
}

class SimplifyCallExprVisitor;

class SimplifyCallExpr : public Transformation {
friend class SimplifyCallExprVisitor;

public:

  SimplifyCallExpr(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      NameQueryWrap(NULL),
      TheCallExpr(NULL),
      CurrentFD(NULL),
      NamePostfix(0)
  { }

  ~SimplifyCallExpr(void);

private:
  
  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void handleOneArgStr(const clang::Expr *Arg, std::string &Str);

  void replaceCallExpr(void);

  SimplifyCallExprVisitor *CollectionVisitor;

  TransNameQueryWrap *NameQueryWrap;

  const clang::CallExpr *TheCallExpr;

  const clang::FunctionDecl *CurrentFD;

  unsigned int NamePostfix;

  // Unimplemented
  SimplifyCallExpr(void);

  SimplifyCallExpr(const SimplifyCallExpr &);

  void operator=(const SimplifyCallExpr &);
};
#endif
