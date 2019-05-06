//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2018 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
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
  class ArraySubscriptExpr;
  class ArrayType;
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
      TheVarDecl(NULL),
      ArraySz(0)
  { }

  ~ReduceArrayDim(void);

private:
  
  typedef std::pair<clang::SourceLocation, clang::SourceLocation>
            BracketLocPair;
  
  typedef llvm::SmallVector<BracketLocPair *, 5> BracketLocPairVector;

  typedef llvm::SmallPtrSet<const clang::VarDecl *, 20> VarDeclSet;

  typedef llvm::SmallVector<const clang::InitListExpr *, 20> 
            InitListExprVector;

  virtual void Initialize(clang::ASTContext &context);

  virtual bool HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void addOneVar(const clang::VarDecl *VD);

  void rewriteOneVarDecl(const clang::VarDecl *VD);

  void handleOneArraySubscriptExpr(const clang::ArraySubscriptExpr *ASE);

  void rewriteSubscriptExpr(const ExprVector &IdxExprs);

  void getBracketLocPairs(const clang::VarDecl *VD,
                          unsigned int Dim,
                          BracketLocPairVector &BPVec);

  void freeBracketLocPairs(BracketLocPairVector &BPVec);

  bool isIntegerExpr(const clang::Expr *E);

  void getInitListExprs(InitListExprVector &InitVec,
                        const clang::InitListExpr *ILE,
                        unsigned int Dim);

  void rewriteInitListExpr(const clang::InitListExpr *ILE,
                           unsigned int Dim);

  unsigned getArraySize(const clang::ArrayType *ATy);

  VarDeclSet VisitedVarDecls;

  ReduceArrayDimCollectionVisitor *CollectionVisitor;

  ReduceArrayDimRewriteVisitor *RewriteVisitor;

  const clang::VarDecl *TheVarDecl;

  // for int a[1][2][3][4], ArraySz stores value 3
  // It's used for computing indices of reduced ArraySubscriptExpr
  unsigned int ArraySz;

  // Unimplemented
  ReduceArrayDim(void);

  ReduceArrayDim(const ReduceArrayDim &);

  void operator=(const ReduceArrayDim &);
};

#endif
