//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef REMOVE_TRIVIAL_BASE_TEMPLATE_H
#define REMOVE_TRIVIAL_BASE_TEMPLATE_H

#include "llvm/ADT/SmallPtrSet.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class CXXBaseSpecifier;
  class ClassTemplateDecl;
}

class RemoveTrivialBaseTemplateBaseVisitor;

class RemoveTrivialBaseTemplate : public Transformation {
friend class RemoveTrivialBaseTemplateBaseVisitor;

public:
  RemoveTrivialBaseTemplate(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      TheDerivedClass(NULL),
      ThePos(0)
  { }

  ~RemoveTrivialBaseTemplate(void);

private:
  typedef llvm::SmallPtrSet<const clang::CXXRecordDecl *, 20> CXXRecordDeclSet;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void handleOneCXXRecordDecl(const clang::CXXRecordDecl *CXXRD);

  void removeBaseSpecifier(void);

  CXXRecordDeclSet VisitedCXXRecordDecls;

  RemoveTrivialBaseTemplateBaseVisitor *CollectionVisitor;

  const clang::CXXRecordDecl *TheDerivedClass;

  unsigned ThePos;

  // Unimplemented
  RemoveTrivialBaseTemplate(void);

  RemoveTrivialBaseTemplate(const RemoveTrivialBaseTemplate &);

  void operator=(const RemoveTrivialBaseTemplate &);
};

#endif

