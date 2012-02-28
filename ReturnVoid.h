//===----------------------------------------------------------------------===//
// 
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef VOID_RETURN_H
#define VOID_RETURN_H

#include <string>
#include "llvm/ADT/SmallVector.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class FunctionDecl;
  class ReturnStmt;
}

class RVASTVisitor;

class ReturnVoid : public Transformation {
friend class RVASTVisitor;

public:

  ReturnVoid(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      TransformationASTVisitor(NULL),
      TheFuncDecl(NULL),
      FuncDefStartPos(NULL),
      FuncDefEndPos(NULL)
  { }

  ~ReturnVoid(void);

private:
  
  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void keepFuncDefRange(clang::FunctionDecl *FD);

  bool isNonVoidReturnFunction(clang::FunctionDecl *FD);

  bool isInTheFuncDef(clang::ReturnStmt *RS);

  RVASTVisitor *TransformationASTVisitor;

  llvm::SmallVector<clang::FunctionDecl *, 10> ValidFuncDecls;

  clang::FunctionDecl *TheFuncDecl;

  const char *FuncDefStartPos;

  const char *FuncDefEndPos;

  // Unimplemented
  ReturnVoid(void);

  ReturnVoid(const ReturnVoid &);

  void operator=(const ReturnVoid &);
};
#endif
