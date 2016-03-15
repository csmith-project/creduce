//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2015 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef SIMPLIFY_NESTED_CLASS_H
#define SIMPLIFY_NESTED_CLASS_H

#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class CXXRecordDecl;
  class Decl;
}

class SimplifyNestedClassVisitor;
class SimplifyNestedClassRewriteVisitor;

class SimplifyNestedClass : public Transformation {
friend class SimplifyNestedClassVisitor;
friend class SimplifyNestedClassRewriteVisitor;

public:
  SimplifyNestedClass(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      RewriteVisitor(NULL),
      TheBaseCXXRD(NULL)
  { }

  ~SimplifyNestedClass(void);

private:
  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void handleOneCXXRecordDecl(const clang::CXXRecordDecl *CXXRD);

  void removeOuterClass();

  SimplifyNestedClassVisitor *CollectionVisitor;

  SimplifyNestedClassRewriteVisitor *RewriteVisitor;

  const clang::CXXRecordDecl *TheBaseCXXRD;

  const clang::Decl *TheInnerDecl;

  // Unimplemented
  SimplifyNestedClass(void);

  SimplifyNestedClass(const SimplifyNestedClass &);

  void operator=(const SimplifyNestedClass &);
};

#endif

