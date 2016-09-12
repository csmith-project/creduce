//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2016 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef REMOVE_POINTER_H
#define REMOVE_POINTER_H

#include <utility>
#include "Transformation.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallPtrSet.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class VarDecl;
  class DeclRefExpr;
}

class RemovePointerCollectionVisitor;

class RemovePointer : public Transformation {
friend class RemovePointerCollectionVisitor;

public:

  RemovePointer(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      TheVarDecl(NULL)
  { }

  ~RemovePointer(void);

private:

  typedef llvm::SmallPtrSet<const clang::VarDecl *, 10> VarDeclSet;
  typedef llvm::SetVector<const clang::VarDecl *> VarDeclSetVector;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void doAnalysis(void);

  void doRewriting(void);

  void handleOneVarDecl(const clang::VarDecl *VD);

  void invalidateOneVarDecl(const clang::DeclRefExpr *DRE);

  VarDeclSetVector AllPointerVarDecls;

  VarDeclSet AllInvalidPointerVarDecls;

  RemovePointerCollectionVisitor *CollectionVisitor;

  const clang::VarDecl *TheVarDecl;

  // Unimplemented
  RemovePointer(void);

  RemovePointer(const RemovePointer &);

  void operator=(const RemovePointer &);
};

#endif
