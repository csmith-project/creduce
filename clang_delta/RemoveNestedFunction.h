//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See LICENSE.TXT for details.
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
}

class RNFCollectionVisitor;

class RemoveNestedFunction : public Transformation {
friend class RNFCollectionVisitor;

public:

  RemoveNestedFunction(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      NestedInvocationVisitor(NULL),
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

  bool addNewTmpVariable(void);

  bool addNewAssignStmt(void);

  bool replaceCallExpr(void);

  const clang::FunctionDecl *lookupFunctionDecl(clang::DeclarationName &DName,
                                                const clang::DeclContext *Ctx);

  const clang::DeclContext *getDeclContextFromSpecifier(
          const clang::NestedNameSpecifier *Qualifier);

  void setTmpVarName(std::string &Name) {
    TmpVarName = Name;
  }

  std::string getTmpVarName(void) {
    return TmpVarName;
  }

  clang::SmallVector<clang::CallExpr *, 5> CallExprQueue;

  clang::SmallVector<clang::CallExpr *, 10> ValidCallExprs;

  RNFCollectionVisitor *NestedInvocationVisitor;

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
