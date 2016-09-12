//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2016 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef SIMPLE_INLINER_H
#define SIMPLE_INLINER_H

#include <string>
#include <vector>
#include <utility>
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/DenseMap.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class FunctionDecl;
  class CallExpr;
  class DeclRefExpr;
  class ReturnStmt;
  class Expr;
  class Stmt;
}

class SimpleInlinerCollectionVisitor;
class SimpleInlinerFunctionVisitor;
class SimpleInlinerFunctionStmtVisitor;
class SimpleInlinerStmtVisitor;

class SimpleInliner : public Transformation {
friend class SimpleInlinerCollectionVisitor;
friend class SimpleInlinerFunctionVisitor;
friend class SimpleInlinerFunctionStmtVisitor;
friend class SimpleInlinerStmtVisitor;

public:

  SimpleInliner(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      FunctionVisitor(NULL),
      FunctionStmtVisitor(NULL),
      StmtVisitor(NULL),
      NameQueryWrap(NULL),
      TheCallExpr(NULL),
      TheCaller(NULL),
      CurrentFD(NULL),
      TheStmt(NULL),
      SingleMaxNumStmts(25),
      MaxNumStmts(10),
      TmpVarName(""),
      NeedParen(false),
      NamePostfix(0)
  { }

  ~SimpleInliner(void);

private:
  
  typedef llvm::SmallVector<clang::ReturnStmt *, 5> ReturnStmtsVector;

  typedef llvm::SmallVector<const clang::DeclRefExpr *, 5> ParmRefsVector;

  typedef llvm::DenseMap<clang::FunctionDecl *, unsigned int> 
            FunctionDeclToNumCallsMap;

  typedef llvm::DenseMap<clang::FunctionDecl *, unsigned int> 
            FunctionDeclToNumStmtsMap;

  virtual void Initialize(clang::ASTContext &context);

  virtual bool HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void replaceCallExpr(void);

  void doAnalysis(void);

  bool isValidArgExpr(const clang::Expr *E);

  bool hasValidArgExprs(const clang::CallExpr *CE);

  void createReturnVar(void);

  void generateParamStrings(void);

  void copyFunctionBody(void);

  std::string getNewTmpName(void);

  void getValidFunctionDecls(void);

  void removeFunctionBody(void);

  void sortReturnStmtsByOffs(const char *StartBuf, 
    std::vector< std::pair<clang::ReturnStmt *, int> > &SortedReturnStmts);

  void insertReturnStmt
      (std::vector< std::pair<clang::ReturnStmt *, int> > &SortedReturnStmts,
       clang::ReturnStmt *RS, int Off);

  bool hasNameClash(const std::string &ParmName, const clang::Expr *E);

  FunctionDeclToNumCallsMap FunctionDeclNumCalls;

  FunctionDeclToNumStmtsMap FunctionDeclNumStmts;

  llvm::DenseMap<clang::CallExpr *, clang::FunctionDecl *> CalleeToCallerMap;

  llvm::SmallVector<clang::CallExpr *, 10> AllCallExprs;

  llvm::SmallSet<clang::FunctionDecl *, 10> ValidFunctionDecls;

  llvm::SmallVector<std::string, 10> ParmStrings;

  llvm::SmallVector<std::string, 4> ParmsWithNameClash;

  ReturnStmtsVector ReturnStmts;

  ParmRefsVector ParmRefs;

  SimpleInlinerCollectionVisitor *CollectionVisitor;

  SimpleInlinerFunctionVisitor *FunctionVisitor;

  SimpleInlinerFunctionStmtVisitor *FunctionStmtVisitor;

  SimpleInlinerStmtVisitor *StmtVisitor;

  TransNameQueryWrap *NameQueryWrap;

  clang::CallExpr *TheCallExpr;

  clang::FunctionDecl *TheCaller;

  clang::FunctionDecl *CurrentFD;

  clang::Stmt *TheStmt;

  // MaxNumStmts for a function with single call site
  const unsigned int SingleMaxNumStmts;

  // MaxNumStmts for a function with multiple call sites
  const unsigned int MaxNumStmts;

  std::string TmpVarName;

  bool NeedParen;

  unsigned int NamePostfix;

  // Unimplemented
  SimpleInliner(void);

  SimpleInliner(const SimpleInliner &);

  void operator=(const SimpleInliner &);
};
#endif
