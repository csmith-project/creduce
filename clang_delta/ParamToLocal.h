//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef PARAM_TO_LOCAL_H
#define PARAM_TO_LOCAL_H

#include <string>
#include "llvm/ADT/SmallVector.h"
#include "Transformation.h"
#include "CommonParameterRewriteVisitor.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class FunctionDecl;
  class ParmVarDecl;
}

class ParamToLocalASTVisitor;
class ParamToLocalRewriteVisitor;
template<typename T, typename Trans> class CommonParameterRewriteVisitor;

class ParamToLocal : public Transformation {
friend class ParamToLocalASTVisitor;
friend class ParamToLocalRewriteVisitor;
friend class CommonParameterRewriteVisitor<ParamToLocalRewriteVisitor, 
                                           ParamToLocal>;

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

  bool transformParamVar(clang::FunctionDecl *FD, const clang::ParmVarDecl *PV);

  bool rewriteFuncDecl(clang::FunctionDecl *FD);

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
