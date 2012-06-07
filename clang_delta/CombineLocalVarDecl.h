//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef COMBINE_LOCAL_VAR_DECL_H
#define COMBINE_LOCAL_VAR_DECL_H

#include <string>
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/DenseMap.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class DeclStmt;
  class CompoundStmt;
  class Type;
}

class CombLocalVarCollectionVisitor;
class CombineLocalVarDecl : public Transformation {
friend class CombLocalVarCollectionVisitor;

public:

  CombineLocalVarDecl(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL)
  { }

  ~CombineLocalVarDecl(void);

private:
  
  virtual void Initialize(clang::ASTContext &context);

  virtual bool HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void doCombination();

  llvm::DenseMap<const clang::Type *, clang::DeclStmt *> DeclStmts;

  llvm::SmallVector<clang::DeclStmt *, 2> TheDeclStmts;

  CombLocalVarCollectionVisitor *CollectionVisitor;

  // Unimplemented
  CombineLocalVarDecl(void);

  CombineLocalVarDecl(const CombineLocalVarDecl &);

  void operator=(const CombineLocalVarDecl &);
};
#endif
