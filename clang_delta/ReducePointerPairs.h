//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef REDUCE_POINTER_PAIRS_H
#define REDUCE_POINTER_PAIRS_H

#include "Transformation.h"
#include "llvm/ADT/DenseMap.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class VarDecl;
}

class ReducePointerPairsCollectionVisitor;
class ReducePointerPairsInvalidatingVisitor;

class ReducePointerPairs : public Transformation {
friend class ReducePointerPairsCollectionVisitor;
friend class ReducePointerPairsInvalidatingVisitor;

public:

  ReducePointerPairs(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      InvalidatingVisitor(NULL),
      TheVarDecl(NULL),
      ThePairedVarDecl(NULL)
  { }

  ~ReducePointerPairs(void);

private:

  typedef llvm::DenseMap<const clang::VarDecl *, const clang::VarDecl *> 
            PointerMap;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void handleOnePair(const clang::VarDecl *VD, const clang::Expr *E);

  bool isValidVD(const clang::VarDecl *VD);

  bool invalidatePairedVarDecl(const clang::VarDecl *VD);

  const clang::VarDecl *getVarDeclFromDRE(const clang::Expr *E);

  bool isMappedVarDecl(const clang::VarDecl *VD);

  void doRewriting(const clang::VarDecl *VD);

  void doAnalysis(void);

  void invalidateVarDecl(const clang::VarDecl *VD);

  PointerMap ValidPointerPairs;

  ReducePointerPairsCollectionVisitor *CollectionVisitor;

  ReducePointerPairsInvalidatingVisitor *InvalidatingVisitor;

  const clang::VarDecl *TheVarDecl;

  const clang::VarDecl *ThePairedVarDecl;

  // Unimplemented
  ReducePointerPairs(void);

  ReducePointerPairs(const ReducePointerPairs &);

  void operator=(const ReducePointerPairs &);
};

#endif
