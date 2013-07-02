//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef PARAM_TO_GLOBAL_H
#define PARAM_TO_GLOBAL_H

#include <string>
#include "llvm/ADT/SmallVector.h"
#include "Transformation.h"
#include "CommonParameterRewriteVisitor.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class FunctionDecl;
  class ReturnStmt;
  class ParmVarDecl;
}

class ParamToGlobalASTVisitor;
class ParamToGlobalRewriteVisitor;
template<typename T, typename Trans> class CommonParameterRewriteVisitor;

class ParamToGlobal : public Transformation {
friend class ParamToGlobalASTVisitor;
friend class ParamToGlobalRewriteVisitor;
friend class CommonParameterRewriteVisitor<ParamToGlobalRewriteVisitor, 
                                           ParamToGlobal>;

public:

  ParamToGlobal(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      RewriteVisitor(NULL),
      TheFuncDecl(NULL),
      TheParmVarDecl(NULL),
      TheNewDeclName(""),
      TheParamPos(-1)
  { }

  ~ParamToGlobal(void);

private:
  
  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  bool transformParamVar(clang::FunctionDecl *FD, 
                         const clang::ParmVarDecl *PV);

  bool rewriteFuncDecl(clang::FunctionDecl *FD);

  std::string getNewName(clang::FunctionDecl *FP,
                         const clang::ParmVarDecl *PV);

  bool isValidFuncDecl(clang::FunctionDecl *FD);

  llvm::SmallVector<clang::FunctionDecl *, 10> ValidFuncDecls;

  ParamToGlobalASTVisitor *CollectionVisitor;

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
