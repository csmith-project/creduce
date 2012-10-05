//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef REPLACE_ARRAY_INDEX_VAR_H
#define REPLACE_ARRAY_INDEX_VAR_H

#include "Transformation.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
}

class ReplaceArrayIndexVarCollectionVisitor;

class ReplaceArrayIndexVar : public Transformation {
friend class ReplaceArrayIndexVarCollectionVisitor;

public:

  ReplaceArrayIndexVar(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      MaxSize(15),
      TheASESet(NULL),
      CurrIdx(0)
  { }

  ~ReplaceArrayIndexVar(void);

private:

  typedef llvm::SmallPtrSet<const clang::ArraySubscriptExpr *, 10> 
    ArraySubscriptExprSet;

  typedef llvm::DenseMap<const clang::VarDecl *, unsigned> 
    ArrayVarsToSizeMap;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void doRewrite(void);

  ArrayVarsToSizeMap CstArrayVars;

  ReplaceArrayIndexVarCollectionVisitor *CollectionVisitor;

  const unsigned MaxSize;

  ArraySubscriptExprSet *TheASESet;

  unsigned CurrIdx;

  // Unimplemented
  ReplaceArrayIndexVar(void);

  ReplaceArrayIndexVar(const ReplaceArrayIndexVar &);

  void operator=(const ReplaceArrayIndexVar &);
};

#endif
