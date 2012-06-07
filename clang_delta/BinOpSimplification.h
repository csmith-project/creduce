//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef BINOP_SIMPLIFICATION_H
#define BINOP_SIMPLIFICATION_H

#include <string>
#include "llvm/ADT/SmallVector.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class FunctionDecl;
  class Stmt;
  class BinaryOperator;
}

class BSCollectionVisitor;
class BSStatementVisitor;

class BinOpSimplification : public Transformation {
friend class BSCollectionVisitor;
friend class BSStatementVisitor;

public:

  BinOpSimplification(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      BinOpCollectionVisitor(NULL),
      StmtVisitor(NULL),
      NameQueryWrap(NULL),
      TheFuncDecl(NULL),
      TheStmt(NULL),
      TheBinOp(NULL),
      TmpVarName(""),
      NeedParen(false)
  { }

  ~BinOpSimplification(void);

private:
  
  virtual void Initialize(clang::ASTContext &context);

  virtual bool HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  bool addNewTmpVariable(void);

  bool addNewAssignStmt(void);

  bool replaceBinOp(void);

  void setTmpVarName(std::string &Name) {
    TmpVarName = Name;
  }

  std::string getTmpVarName(void) {
    return TmpVarName;
  }

  clang::SmallVector<clang::BinaryOperator *, 10> ValidBinOps;

  BSCollectionVisitor *BinOpCollectionVisitor;

  BSStatementVisitor *StmtVisitor;

  TransNameQueryWrap *NameQueryWrap;

  clang::FunctionDecl *TheFuncDecl;

  clang::Stmt *TheStmt;

  clang::BinaryOperator *TheBinOp;

  std::string TmpVarName;

  bool NeedParen;

  // Unimplemented
  BinOpSimplification(void);

  BinOpSimplification(const BinOpSimplification &);

  void operator=(const BinOpSimplification &);
};
#endif
