//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2014 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef COPY_PROPAGATION_H
#define COPY_PROPAGATION_H

#include <string>
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/DenseMap.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class Expr;
  class VarDecl;
  class MemberExpr;
  class ArraySubscriptExpr;
}

class CopyPropCollectionVisitor;

class CopyPropagation : public Transformation {
friend class CopyPropCollectionVisitor;

public:

  CopyPropagation(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      TheCopyExpr(NULL)
  { }

  ~CopyPropagation(void);

private:
  
  typedef llvm::DenseMap<const clang::VarDecl *, const clang::Expr *> 
            VarToExprMap;

  typedef llvm::DenseMap<const clang::MemberExpr *, const clang::Expr *>
            MemberExprToExprMap;

  typedef llvm::DenseMap<const clang::ArraySubscriptExpr *, 
                         const clang::Expr *> ArraySubToExprMap;

  typedef llvm::SmallPtrSet<const clang::Expr *, 20> ExprSet;

  typedef llvm::DenseMap<const clang::Expr *, ExprSet *>
            ExprToExprsMap;

  virtual void Initialize(clang::ASTContext &context);

  virtual bool HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  bool isValidExpr(const clang::Expr *E);

  bool isValidLhs(const clang::Expr *Lhs);

  void updateExpr(const clang::Expr *E, const clang::Expr *CopyE);

  void invalidateExpr(const clang::Expr *E);

  void addOneDominatedExpr(const clang::Expr *CopyE, 
                           const clang::Expr *DominatedE);
  
  const clang::VarDecl *getCanonicalRefVarDecl(const clang::Expr *E);

  bool isRefToTheSameVar(const clang::Expr *CopyE,
                         const clang::Expr *DominatedE);

  bool hasSameStringRep(const clang::Expr *CopyE,
                        const clang::Expr *DominatedE);

  void doCopyPropagation(void);

  bool isConstantExpr(const clang::Expr *Exp);

  // A mapping from a var to its value at the current processing point
  VarToExprMap VarToExpr;

  // A mapping from a member expr to its value at the current processing point
  MemberExprToExprMap MemberExprToExpr;

  // A mapping from a arraysubscript expr to its value at
  // the current processing point
  ArraySubToExprMap ArraySubToExpr;

  // Only hold visited MemberExpr and ArraySubscriptExpr. 
  // Used for distinguishing a valid Member/ArraySubscript expr
  // and a Member/ArraySubscript expr which will get a copy from its
  // corresponding initializer. The advantage is that we don't have to
  // set up initial value for all the fields of a var which has an aggregate
  // type. We only need to retrieve the initial value of a field on demand.
  // We don't need to do this for VarDecl because we can directly get its
  // initial value.
  ExprSet VisitedMEAndASE;

  // A mapping from an Expr to its dominating Exprs
  ExprToExprsMap DominatedMap;

  CopyPropCollectionVisitor *CollectionVisitor;

  const clang::Expr *TheCopyExpr;

  // Unimplemented
  CopyPropagation(void);

  CopyPropagation(const CopyPropagation &);

  void operator=(const CopyPropagation &);
};
#endif
