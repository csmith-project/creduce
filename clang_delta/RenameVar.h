//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2016 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef RENAME_VAR_H
#define RENAME_VAR_H

#include <string>
#include <vector>
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/DenseMap.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class VarDecl;
}

class RNVCollectionVisitor;
class RenameVarVisitor;

class RenameVar : public Transformation {
friend class RNVCollectionVisitor;
friend class RenameVarVisitor;

public:

  RenameVar(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      VarCollectionVisitor(NULL),
      RenameVisitor(NULL),
      RenamePrefix('t')
  { }

  ~RenameVar(void);

  virtual bool skipCounter(void) {
    return true;
  }

private:
  
  virtual void Initialize(clang::ASTContext &context);

  virtual bool HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void addVar(clang::VarDecl *VD);

  bool allValidNames();

  void collectVars(void);

  RNVCollectionVisitor *VarCollectionVisitor;

  RenameVarVisitor *RenameVisitor;

  char RenamePrefix;

  std::vector<clang::VarDecl *> ValidVars;

  llvm::SmallVector<char, 26> AvailableNames;

  llvm::DenseMap<clang::VarDecl *, std::string> VarToNameMap;

  llvm::SmallPtrSet<clang::DeclRefExpr *, 10> VisitedDREs;

  // Unimplemented
  RenameVar(void);

  RenameVar(const RenameVar &);

  void operator=(const RenameVar &);
};
#endif
