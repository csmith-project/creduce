//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef REDUCE_ARRAY_DIM_H
#define REDUCE_ARRAY_DIM_H

#include <string>
#include <utility>
#include "llvm/ADT/SmallPtrSet.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class VarDecl;
}

class ReduceArrayDimCollectionVisitor;
class ReduceArrayDimRewriteVisitor;

class ReduceArrayDim : public Transformation {
friend class ReduceArrayDimCollectionVisitor;
friend class ReduceArrayDimRewriteVisitor;

public:

  ReduceArrayDim(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      RewriteVisitor(NULL),
      TheVarDecl(NULL)
  { }

  ~ReduceArrayDim(void);

private:
  
  typedef std::pair<clang::SourceLocation, clang::SourceLocation>
            BracketLocPair;
  
  typedef llvm::SmallVector<BracketLocPair *, 5> BracketLocPairVector;

  typedef llvm::SmallPtrSet<const clang::VarDecl *, 20> VarDeclSet;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void addOneVar(const clang::VarDecl *VD);

  void rewriteOneVarDecl(const clang::VarDecl *VD);

  void getBracketLocPairs(const clang::VarDecl *VD,
                          unsigned int Dim,
                          BracketLocPairVector &BPVec);

  void freeBracketLocPairs(BracketLocPairVector &BPVec);

  VarDeclSet VisitedVarDecls;

  ReduceArrayDimCollectionVisitor *CollectionVisitor;

  ReduceArrayDimRewriteVisitor *RewriteVisitor;

  const clang::VarDecl *TheVarDecl;

  // Unimplemented
  ReduceArrayDim(void);

  ReduceArrayDim(const ReduceArrayDim &);

  void operator=(const ReduceArrayDim &);
};

#endif
