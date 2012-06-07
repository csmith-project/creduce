//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef MOVE_FUNCTION_BODY_H
#define MOVE_FUNCTION_BODY_H

#include <string>
#include "llvm/ADT/DenseMap.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class FunctionDecl;
}

class MoveFunctionBody : public Transformation {

public:

  MoveFunctionBody(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      TheFunctionDecl(NULL),
      TheFunctionDef(NULL),
      PrevFunctionDecl(NULL)
  { }

  ~MoveFunctionBody(void);

private:
  
  typedef llvm::DenseMap<clang::FunctionDecl *, clang::FunctionDecl *>
            FuncDeclToFuncDeclMap;

  virtual void Initialize(clang::ASTContext &context);

  virtual bool HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void doAnalysis(void);

  void doRewriting(void);

  FuncDeclToFuncDeclMap AllValidFunctionDecls;

  FuncDeclToFuncDeclMap FuncDeclToFuncDef;

  clang::FunctionDecl *TheFunctionDecl;

  clang::FunctionDecl *TheFunctionDef;

  clang::FunctionDecl *PrevFunctionDecl;

  // Unimplemented
  MoveFunctionBody(void);

  MoveFunctionBody(const MoveFunctionBody &);

  void operator=(const MoveFunctionBody &);
};
#endif
