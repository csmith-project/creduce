//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef LOCAL_TO_GLOBAL_H
#define LOCAL_TO_GLOBAL_H

#include <string>
#include "llvm/ADT/SmallPtrSet.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class FunctionDecl;
  class ReturnStmt;
  class ParmVarDecl;
}

class LocalToGlobalFunctionVisitor;
class LToGASTVisitor;
class LocalToGlobalCollectionVisitor;

class LocalToGlobal : public Transformation {
friend class LocalToGlobalFunctionVisitor;
friend class LToGASTVisitor;
friend class LocalToGlobalCollectionVisitor;

public:

  LocalToGlobal(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      FunctionVisitor(NULL),
      LocalVarCollectionVisitor(NULL),
      TransformationASTVisitor(NULL),
      TheFuncDecl(NULL),
      TheVarDecl(NULL),
      TheNewDeclName("")
  { }

  ~LocalToGlobal(void);

private:
  
  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void setNewName(clang::FunctionDecl *FD, const clang::VarDecl *VD);

  std::string getNewName(void) {
    return TheNewDeclName;
  }

  llvm::SmallPtrSet<const clang::VarDecl *, 10> SkippedVars;

  LocalToGlobalFunctionVisitor *FunctionVisitor;

  LocalToGlobalCollectionVisitor *LocalVarCollectionVisitor;

  LToGASTVisitor *TransformationASTVisitor;

  clang::FunctionDecl *TheFuncDecl;

  const clang::VarDecl *TheVarDecl;

  std::string TheNewDeclName;

  // Unimplemented
  LocalToGlobal(void);

  LocalToGlobal(const LocalToGlobal &);

  void operator=(const LocalToGlobal &);
};
#endif
