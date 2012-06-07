//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef UNIFY_FUNCTION_DECL_H
#define UNIFY_FUNCTION_DECL_H

#include <string>
#include "llvm/ADT/SmallPtrSet.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class FunctionDecl;
}

class UnifyFunctionDecl : public Transformation {

public:

  UnifyFunctionDecl(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      TheFunctionDecl(NULL),
      TheFunctionDef(NULL)
  { }

  ~UnifyFunctionDecl(void);

private:
  
  typedef llvm::SmallPtrSet<const clang::FunctionDecl *, 10>
            FunctionDeclSet;

  virtual void Initialize(clang::ASTContext &context);

  virtual bool HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void doAnalysis(void);

  void doRewriting(void);

  bool hasStaticKeyword(const clang::FunctionDecl *FD);

  FunctionDeclSet VisitedFunctionDecls;

  const clang::FunctionDecl *TheFunctionDecl;

  const clang::FunctionDecl *TheFunctionDef;

  // Unimplemented
  UnifyFunctionDecl(void);

  UnifyFunctionDecl(const UnifyFunctionDecl &);

  void operator=(const UnifyFunctionDecl &);
};
#endif
