//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef SIMPLIFY_DEPENDENT_TYPEDEF_H
#define SIMPLIFY_DEPENDENT_TYPEDEF_H

#include "Transformation.h"
#include "llvm/ADT/SmallPtrSet.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class TypedefDecl;
  class ClassTemplateDecl;
  class TemplateTypeParmDecl;
}

class DependentTypedefCollectionVisitor;
class DependentTypedefTemplateTypeParmTypeVisitor;

class SimplifyDependentTypedef : public Transformation {
friend class DependentTypedefCollectionVisitor;
friend class DependentTypedefTemplateTypeParmTypeVisitor;

public:
  SimplifyDependentTypedef(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      TemplateTypeParmTypeVisitor(NULL),
      FirstTmplTypeParmD(NULL)
  {}

  ~SimplifyDependentTypedef(void);

private:

  typedef llvm::SmallPtrSet<const clang::TypedefDecl *, 20> TypedefDeclsSet;

  typedef llvm::SmallPtrSet<const clang::Type *, 5> TemplateTypeParmTypeSet;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void handleOneTypedefDecl(const clang::TypedefDecl *D);

  void rewriteTypedefDecl(void);

  TypedefDeclsSet VisitedTypedefDecls;

  DependentTypedefCollectionVisitor *CollectionVisitor;

  DependentTypedefTemplateTypeParmTypeVisitor *TemplateTypeParmTypeVisitor;

  const clang::TemplateTypeParmDecl *FirstTmplTypeParmD;

  const clang::TypedefDecl *TheTypedefDecl;

  // Unimplemented
  SimplifyDependentTypedef(void);

  SimplifyDependentTypedef(const SimplifyDependentTypedef &);

  void operator=(const SimplifyDependentTypedef &);
};

#endif
