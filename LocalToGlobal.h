//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LOCAL_TO_GLOBAL_H
#define LOCAL_TO_GLOBAL_H

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

class LToGASTVisitor;
class CollectionVisitor;

class LocalToGlobal : public Transformation {
friend class LToGASTVisitor;
friend class CollectionVisitor;

public:

  LocalToGlobal(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      LocalVarCollectionVisitor(NULL),
      TransformationASTVisitor(NULL),
      TheFuncDecl(NULL),
      TheVarDecl(NULL),
      TheNewDeclName("")
  { }

  ~LocalToGlobal(void);

private:
  
  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void setNewName(clang::FunctionDecl *FD, const clang::VarDecl *VD);

  std::string getNewName(void) {
    return TheNewDeclName;
  }

  CollectionVisitor *LocalVarCollectionVisitor;

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
