//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef REMOVE_BASE_CLASS_H
#define REMOVE_BASE_CLASS_H

#include "llvm/ADT/SmallPtrSet.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class CXXBaseSpecifier;
  class CXXConstructorDecl;
}

class RemoveBaseClassBaseVisitor;
class RemoveBaseClassRewriteVisitor;

class RemoveBaseClass : public Transformation {
friend class RemoveBaseClassBaseVisitor;

public:
  RemoveBaseClass(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      RewriteVisitor(NULL),
      TheBaseClass(NULL),
      TheDerivedClass(NULL),
      MaxNumDecls(5)
  { }

  ~RemoveBaseClass(void);

private:
  typedef llvm::SmallPtrSet<const clang::CXXRecordDecl *, 20> CXXRecordDeclSet;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void handleOneCXXRecordDecl(const clang::CXXRecordDecl *CXXRD);

  void copyBaseClassDecls(void);

  void removeBaseSpecifier(void);

  void removeBaseInitializer(void);

  void rewriteOneCtor(const clang::CXXConstructorDecl *Ctor);

  bool isDirectlyDerivedFrom(const clang::CXXRecordDecl *SubC, 
                             const clang::CXXRecordDecl *Base);

  void doRewrite(void);

  bool isTheBaseClass(const clang::CXXBaseSpecifier &Specifier);

  CXXRecordDeclSet VisitedCXXRecordDecls;

  CXXRecordDeclSet AllBaseClasses;

  RemoveBaseClassBaseVisitor *CollectionVisitor;

  RemoveBaseClassRewriteVisitor *RewriteVisitor;

  const clang::CXXRecordDecl *TheBaseClass;

  const clang::CXXRecordDecl *TheDerivedClass;

  const unsigned MaxNumDecls;

  // Unimplemented
  RemoveBaseClass(void);

  RemoveBaseClass(const RemoveBaseClass &);

  void operator=(const RemoveBaseClass &);
};

#endif

