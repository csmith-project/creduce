//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef PARAM_TO_GLOBAL_H
#define PARAM_TO_GLOBAL_H

#include <string>
#include "llvm/ADT/SmallVector.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class FunctionDecl;
  class ReturnStmt;
  class ParmVarDecl;
}

class ParamToGlobalRewriteVisitor;

class ParamToGlobal : public Transformation {
friend class ParamToGlobalRewriteVisitor;

public:

  ParamToGlobal(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      RewriteVisitor(NULL),
      TheFuncDecl(NULL),
      TheParmVarDecl(NULL),
      TheNewDeclName(""),
      TheParamPos(-1)
  { }

  ~ParamToGlobal(void);

private:
  
  virtual void Initialize(clang::ASTContext &context);

  virtual bool HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  bool isValidFuncDecl(clang::FunctionDecl *FD);

  llvm::SmallVector<clang::FunctionDecl *, 10> ValidFuncDecls;

  ParamToGlobalRewriteVisitor *RewriteVisitor;

  clang::FunctionDecl *TheFuncDecl;

  const clang::ParmVarDecl *TheParmVarDecl;

  std::string TheNewDeclName;

  int TheParamPos;

  // Unimplemented
  ParamToGlobal(void);

  ParamToGlobal(const ParamToGlobal &);

  void operator=(const ParamToGlobal &);
};
#endif
