//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef SIMPLIFY_IF_H
#define SIMPLIFY_IF_H

#include <string>
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class Stmt;
  class IfStmt;
}

class SimplifyIfCollectionVisitor;

class SimplifyIf : public Transformation {
friend class SimplifyIfCollectionVisitor;

public:

  SimplifyIf(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      TheIfStmt(NULL),
      NeedParen(false)
  { }

  ~SimplifyIf(void);

private:
  
  virtual void Initialize(clang::ASTContext &context);

  virtual bool HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void simplifyIfStmt(void);

  SimplifyIfCollectionVisitor *CollectionVisitor;

  clang::IfStmt *TheIfStmt;

  bool NeedParen;

  // Unimplemented
  SimplifyIf(void);

  SimplifyIf(const SimplifyIf &);

  void operator=(const SimplifyIf &);
};
#endif
