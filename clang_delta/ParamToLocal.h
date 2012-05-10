//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef PARAM_TO_LOCAL_H
#define PARAM_TO_LOCAL_H

#include <string>
#include "llvm/ADT/SmallVector.h"
#include "Transformation.h"

namespace clang {
  class FunctionDecl;
  class DeclGroupRef;
  class ASTContext;
}

class ParamToLocalASTVisitor;
class ParamToLocalRewriteVisitor;

class ParamToLocal : public Transformation {
friend class ParamToLocalASTVisitor;
friend class ParamToLocalRewriteVisitor;

public:

  ParamToLocal(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      RewriteVisitor(NULL),
      TheFuncDecl(NULL),
      TheParamPos(-1)
  { }

  ~ParamToLocal(void);

private:
  
  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  bool isValidFuncDecl(clang::FunctionDecl *FD);

  llvm::SmallVector<clang::FunctionDecl *, 10> ValidFuncDecls;

  ParamToLocalASTVisitor *CollectionVisitor;

  ParamToLocalRewriteVisitor *RewriteVisitor;

  clang::FunctionDecl *TheFuncDecl;

  int TheParamPos;

  // Unimplemented
  ParamToLocal(void);

  ParamToLocal(const ParamToLocal &);

  void operator=(const ParamToLocal &);
};

#endif
