//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef AGGREGATE_TO_SCALAR_H
#define AGGREGATE_TO_SCALAR_H

#include <string>
#include <set>
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class FunctionDecl;
  class MemberExpr;
  class VarDecl;
  class InitListExpr;
  class DeclStmt;
  class Expr;
  class ArraySubscriptExpr;
}

class ATSCollectionVisitor;

class AggregateToScalar : public Transformation {
friend class ATSCollectionVisitor;

public:

  AggregateToScalar(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      AggregateAccessVisitor(NULL),
      TheVarDecl(NULL),
      TheIdx(NULL)
  { }

  ~AggregateToScalar(void);

private:
  
  typedef std::set<const clang::Expr *> ExprSet;

  typedef llvm::DenseMap<IndexVector *, ExprSet *> 
    IdxToExpr;

  typedef llvm::SmallPtrSet<IndexVector *, 10> IdxVectorSet;

  typedef llvm::DenseMap<const clang::VarDecl *, IdxVectorSet *>
    VarToIdx;

  virtual void Initialize(clang::ASTContext &context);

  virtual bool HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  bool isStructuralEqualVectors(IndexVector *IV1, IndexVector *IV2);

  void addOneIdx(const clang::Expr *E, 
                 const clang::VarDecl *VD,
                 IdxVectorSet *IdxSet, IndexVector *Idx);

  void addOneExpr(const clang::Expr *E);

  bool createNewVar(const clang::Expr *RefE, std::string &VarName);

  bool addTmpVar(const clang::Expr *RefE, const std::string &VarName, 
                 const std::string *InitStr);

  void createNewVarName(std::string &VarName);

  void doRewrite(void);

  llvm::DenseMap<const clang::VarDecl *, clang::DeclStmt *> VarDeclToDeclStmtMap;

  VarToIdx ValidVars;

  IdxToExpr ValidExprs;

  ATSCollectionVisitor *AggregateAccessVisitor;

  const clang::VarDecl *TheVarDecl;

  IndexVector *TheIdx;

  // Unimplemented
  AggregateToScalar(void);

  AggregateToScalar(const AggregateToScalar &);

  void operator=(const AggregateToScalar &);
};
#endif
