//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef REMOVE_UNUSED_OUTER_CLASS_H
#define REMOVE_UNUSED_OUTER_CLASS_H

#include "llvm/ADT/SmallPtrSet.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class CXXRecordDecl;
}

class RemoveUnusedOuterClassVisitor;

class RemoveUnusedOuterClass : public Transformation {
friend class RemoveUnusedOuterClassVisitor;

public:
  RemoveUnusedOuterClass(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      TheCXXRDDef(NULL)
  { }

  ~RemoveUnusedOuterClass(void);

private:
  typedef llvm::SmallPtrSet<const clang::CXXRecordDecl *, 10> CXXRecordDeclSet;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void analyzeCXXRDSet();

  void removeOuterClass();

  CXXRecordDeclSet UsedCXXRDSet;

  CXXRecordDeclSet CXXRDDefSet;

  RemoveUnusedOuterClassVisitor *CollectionVisitor;

  const clang::CXXRecordDecl *TheCXXRDDef;

  // Unimplemented
  RemoveUnusedOuterClass(void);

  RemoveUnusedOuterClass(const RemoveUnusedOuterClass &);

  void operator=(const RemoveUnusedOuterClass &);
};

#endif

