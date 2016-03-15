//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2014, 2016 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef REMOVE_NESTED_FUNCTION_H
#define REMOVE_NESTED_FUNCTION_H

#include <string>
#include "llvm/ADT/SmallVector.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class FunctionDecl;
  class Stmt;
  class CallExpr;
  class DeclarationName;
  class DeclContext;
  class NestedNameSpecifier;
  class CXXOperatorCallExpr;
  class QualType;
}

class RNFCollectionVisitor;
class RNFStatementVisitor;

class RemoveNestedFunction : public Transformation {
friend class RNFCollectionVisitor;
friend class RNFStatementVisitor;

public:

  RemoveNestedFunction(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      NestedInvocationVisitor(NULL),
      StmtVisitor(NULL),
      NameQueryWrap(NULL),
      TheFuncDecl(NULL),
      TheStmt(NULL),
      TheCallExpr(NULL),
      TmpVarName(""),
      NeedParen(false)
  { }

  ~RemoveNestedFunction(void);

private:
  
  virtual void Initialize(clang::ASTContext &context);

  virtual bool HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void getNewTmpVariableStr(clang::ASTContext &ASTCtx, std::string &VarStr);

  void addNewTmpVariable(clang::ASTContext &ASTCtx);

  bool isInvalidOperator(const clang::CXXOperatorCallExpr *OpE);

  void setTmpVarName(std::string &Name) {
    TmpVarName = Name;
  }

  std::string getTmpVarName(void) {
    return TmpVarName;
  }

  void getVarStrForTemplateSpecialization(
         std::string &VarStr,
         const clang::TemplateSpecializationType *TST);

  void getNewTmpVariable(const clang::QualType &QT, std::string &VarStr);

  void getNewIntTmpVariable(std::string &VarStr);

  clang::SmallVector<clang::CallExpr *, 5> CallExprQueue;

  clang::SmallVector<clang::CallExpr *, 10> ValidCallExprs;

  RNFCollectionVisitor *NestedInvocationVisitor;

  RNFStatementVisitor *StmtVisitor;

  TransNameQueryWrap *NameQueryWrap;

  clang::FunctionDecl *TheFuncDecl;

  clang::Stmt *TheStmt;

  clang::CallExpr *TheCallExpr;

  std::string TmpVarName;

  bool NeedParen;

  // Unimplemented
  RemoveNestedFunction(void);

  RemoveNestedFunction(const RemoveNestedFunction &);

  void operator=(const RemoveNestedFunction &);
};
#endif
