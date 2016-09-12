//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2016 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef REDUCE_ARRAY_SIZE_H
#define REDUCE_ARRAY_SIZE_H

#include <string>
#include <utility>
#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/SmallVector.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class VarDecl;
  class ArraySubscriptExpr;
  class Expr;
}

class ReduceArraySizeCollectionVisitor;

class ReduceArraySize : public Transformation {
friend class ReduceArraySizeCollectionVisitor;

public:

  ReduceArraySize(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      TheVarDecl(NULL),
      TheDimValue(-1),
      TheDimIdx(0)
  { }

  ~ReduceArraySize(void);

private:
  
  typedef llvm::SmallVector<int, 10> DimValueVector;

  typedef llvm::MapVector<const clang::VarDecl *, DimValueVector *> 
            VarDeclToDimMap;

  typedef std::pair<clang::SourceLocation, clang::SourceLocation>
            BracketLocPair;
  
  virtual void Initialize(clang::ASTContext &context);

  virtual bool HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  bool isIntegerExpr(const clang::Expr *Exp);

  void handleOneVar(const clang::VarDecl *VD);

  void handleOneASE(const clang::ArraySubscriptExpr *ASE);

  void doAnalysis(void);

  void rewriteArrayVarDecl(void);

  void getBracketLocPair(const clang::VarDecl *VD, unsigned int Dim,
                         unsigned int DimIdx, BracketLocPair &LocPair);

  // if the value for a DimValue is -1, it means we cannot
  // reduce the size of the corresponding dimension
  VarDeclToDimMap VarDeclToDim;

  // Reference to the original DimValues
  // It is used to check if the maximum index is equal or
  // smaller than the size of an array
  VarDeclToDimMap OrigVarDeclToDim;

  ReduceArraySizeCollectionVisitor *CollectionVisitor;

  const clang::VarDecl *TheVarDecl;

  int TheDimValue;

  unsigned TheDimIdx;

  // Unimplemented
  ReduceArraySize(void);

  ReduceArraySize(const ReduceArraySize &);

  void operator=(const ReduceArraySize &);
};

#endif
