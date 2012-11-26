//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef REPLACE_DERIVED_CLASS_H
#define REPLACE_DERIVED_CLASS_H

#include "llvm/ADT/SmallPtrSet.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class CXXRecordDecl;
}

class ReplaceDerivedClassASTVisitor;
class ReplaceDerivedClassRewriteVisitor;

class ReplaceDerivedClass : public Transformation {
friend class ReplaceDerivedClassASTVisitor;
friend class ReplaceDerivedClassRewriteVisitor;

public:

  ReplaceDerivedClass(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      RewriteVisitor(NULL),
      TheDerivedClass(NULL),
      TheBaseClass(NULL)
  { }

  ~ReplaceDerivedClass(void);

private:

  typedef llvm::SmallPtrSet<const clang::CXXRecordDecl *, 10> CXXRecordDeclSet;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  bool isValidBaseDerivedPair(const clang::CXXRecordDecl *Base,
                              const clang::CXXRecordDecl *Derived);

  bool isEmptyClass(const clang::CXXRecordDecl *CXXDef);

  void handleOneCXXRecordDecl(const clang::CXXRecordDecl *CXXRD);

  void doRewrite(void);

  CXXRecordDeclSet VisitedCXXRecordDecls;

  ReplaceDerivedClassASTVisitor *CollectionVisitor;

  ReplaceDerivedClassRewriteVisitor *RewriteVisitor;

  const clang::CXXRecordDecl *TheDerivedClass;

  const clang::CXXRecordDecl *TheBaseClass;

  // Unimplemented
  ReplaceDerivedClass(void);

  ReplaceDerivedClass(const ReplaceDerivedClass &);

  void operator=(const ReplaceDerivedClass &);
};

#endif

