//===----------------------------------------------------------------------===//
//
// Copyright (c) 2016 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef EXPRESSION_DETECTOR_H
#define EXPRESSION_DETECTOR_H

#include <vector>
#include <map>
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "clang/Basic/SourceLocation.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class FunctionDecl;
  class NamedDecl;
  class VarDecl;
  class Stmt;
  class Expr;
}

class ExprDetectorCollectionVisitor;

class ExpressionDetector : public Transformation {
friend class ExprDetectorCollectionVisitor;
friend class ExprDetectorStmtVisitor;
friend class ExprDetectorTempVarVisitor;

public:
  ExpressionDetector(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL), StaticVarNameQueryWrap(NULL),
      TmpVarNameQueryWrap(NULL), TheFunc(NULL), TheStmt(NULL), TheExpr(NULL),
      StaticVarNamePrefix("__creduce_printed_"),
      TmpVarNamePrefix("__creduce_expr_tmp_"), HasStdio(false),
      HasPrintf(false)
  { }

  ~ExpressionDetector(void);

private:
  typedef std::vector<const clang::Expr *> ExprVector;

  typedef std::map<const clang::Stmt *, ExprVector> StmtToExprMap;

  typedef std::map<const clang::Stmt *,
            llvm::SmallPtrSet<const clang::Expr *, 10> > StmtToExprSetMap;

  typedef std::map<const clang::Stmt *,
            llvm::SmallVector<const clang::VarDecl *, 4> > StmtToVarVecMap;

  typedef llvm::DenseMap<const clang::VarDecl *, const clang::Expr *>
            VarToExprMap;

  virtual void Initialize(clang::ASTContext &context);

  virtual bool HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void addOneTempVar(const clang::VarDecl *VD);

  bool refToTmpVar(const clang::NamedDecl *ND);

  bool isValidExpr(clang::Stmt *S, const clang::Expr *E);

  void doRewrite();

  bool shouldAddPrintf(clang::SourceLocation Loc);

  bool isIdenticalExpr(const clang::Expr *E1, const clang::Expr *E2);

  bool hasIdenticalExpr(
         const llvm::SmallVector<const clang::VarDecl *, 4> &TmpVars,
         const clang::Expr *E);

  StmtToExprMap UniqueExprs;

  VarToExprMap ProcessedExprs;

  StmtToExprSetMap InvalidExprsInUO;

  StmtToVarVecMap TmpVarsInStmt;

  ExprDetectorCollectionVisitor *CollectionVisitor;

  TransNameQueryWrap *StaticVarNameQueryWrap;

  TransNameQueryWrap *TmpVarNameQueryWrap;

  clang::FunctionDecl *TheFunc;

  clang::Stmt *TheStmt;

  clang::Expr *TheExpr;

  std::string StaticVarNamePrefix;

  std::string TmpVarNamePrefix;

  bool HasStdio;

  bool HasPrintf;

  clang::SourceLocation StdioHeaderLoc;

  // Unimplemented
  ExpressionDetector(void);

  ExpressionDetector(const ExpressionDetector &);

  void operator=(const ExpressionDetector &);
};
#endif // EXPRESSION_DETECTOR_H
