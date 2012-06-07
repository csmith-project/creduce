//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef REMOVE_CTOR_INITIALIZER_H
#define REMOVE_CTOR_INITIALIZER_H

#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class CXXConstructorDecl;
  class CXXCtorInitializer;
}

class RemoveCtorInitializerASTVisitor;

class RemoveCtorInitializer : public Transformation {
friend class RemoveCtorInitializerASTVisitor;

public:
  RemoveCtorInitializer(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      TheCtorDecl(NULL),
      TheInitializer(NULL),
      TheIndex(0)
  { }

  ~RemoveCtorInitializer(void);

private:

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  bool isValidType(const clang::Type *Ty);

  RemoveCtorInitializerASTVisitor *CollectionVisitor;

  const clang::CXXConstructorDecl *TheCtorDecl;

  const clang::CXXCtorInitializer *TheInitializer;

  unsigned TheIndex;

  // Unimplemented
  RemoveCtorInitializer(void);

  RemoveCtorInitializer(const RemoveCtorInitializer &);

  void operator=(const RemoveCtorInitializer &);

};

#endif

