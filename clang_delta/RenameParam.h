//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef RENAME_PARAM_H
#define RENAME_PARAM_H

#include <string>
#include <vector>
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallSet.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class FunctionDecl;
  class ParmVarDecl;
}

class ExistingVarCollectionVisitor;
class RenameParamVisitor;

class RenameParam : public Transformation {
friend class ExistingVarCollectionVisitor;
friend class RenameParamVisitor;

public:

  RenameParam(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      VarCollectionVisitor(NULL),
      RenameVisitor(NULL),
      ParamNamePrefix("p"),
      HasValidParams(false)
  { }

  ~RenameParam(void);

  virtual bool skipCounter(void) {
    return true;
  }

private:
  
  typedef llvm::SmallSet<unsigned int, 5> ExistingNumberSet;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void addGlobalVar(clang::VarDecl *VD);

  void addLocalVar(clang::VarDecl *VD);

  bool getPostfixValue(const std::string &Name, unsigned int &Value);

  bool isValidPostfix(ExistingNumberSet *LocalSet, unsigned int Postfix);

  unsigned int validatePostfix(clang::FunctionDecl *FD, 
                               unsigned int CurrPostfix);

  void validateParam(clang::ParmVarDecl *PD);

  ExistingVarCollectionVisitor *VarCollectionVisitor;

  RenameParamVisitor *RenameVisitor;

  llvm::DenseMap<clang::FunctionDecl *, ExistingNumberSet *> FunExistingVarsMap;

  ExistingNumberSet ExistingGlobalVars;

  const std::string ParamNamePrefix;

  bool HasValidParams;

  // Unimplemented
  RenameParam(void);

  RenameParam(const RenameParam &);

  void operator=(const RenameParam &);
};
#endif
