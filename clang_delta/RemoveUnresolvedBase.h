//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef REMOVE_UNRESOLVED_BASE_H
#define REMOVE_UNRESOLVED_BASE_H

#include "llvm/ADT/SmallPtrSet.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class CXXBaseSpecifier;
  class CXXRecordDecl;
}

class RemoveUnresolvedBaseASTVisitor;
class RemoveUnresolvedBase : public Transformation {
friend class RemoveUnresolvedBaseASTVisitor;

public:
  RemoveUnresolvedBase(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      TheDerivedClass(NULL),
      TheBaseSpecifier(NULL),
      TheIndex(0)
  { }

  ~RemoveUnresolvedBase(void);

private:
  typedef llvm::SmallPtrSet<const clang::CXXRecordDecl *, 20> CXXRecordDeclSet;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void removeBaseSpecifier(void);

  CXXRecordDeclSet VisitedCXXRecordDecls;

  RemoveUnresolvedBaseASTVisitor *CollectionVisitor;

  const clang::CXXRecordDecl *TheDerivedClass;

  const clang::CXXBaseSpecifier *TheBaseSpecifier;

  unsigned TheIndex;

  // Unimplemented
  RemoveUnresolvedBase(void);

  RemoveUnresolvedBase(const RemoveUnresolvedBase &);

  void operator=(const RemoveUnresolvedBase &);
};

#endif

