//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef REMOVE_ADDR_TAKEN_H
#define REMOVE_ADDR_TAKEN_H

#include "Transformation.h"
#include "llvm/ADT/SmallPtrSet.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class UnaryOperator;
}

class RemoveAddrTakenCollectionVisitor;

class RemoveAddrTaken : public Transformation {
friend class RemoveAddrTakenCollectionVisitor;

public:

  RemoveAddrTaken(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL)
  { }

  ~RemoveAddrTaken(void);

private:

  typedef llvm::SmallPtrSet<const clang::UnaryOperator *, 10> UnaryOperatorSet;

  virtual void Initialize(clang::ASTContext &context);

  virtual bool HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void rewriteAddrTakenOp(const clang::UnaryOperator *UO);

  UnaryOperatorSet VisitedAddrTakenOps;

  RemoveAddrTakenCollectionVisitor *CollectionVisitor;

  const clang::UnaryOperator *TheUO;

  // Unimplemented
  RemoveAddrTaken(void);

  RemoveAddrTaken(const RemoveAddrTaken &);

  void operator=(const RemoveAddrTaken &);
};

#endif
