//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef REMOVE_UNUSED_VAR_H
#define REMOVE_UNUSED_VAR_H

#include <string>
#include "llvm/ADT/DenseMap.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class VarDecl;
  class LinkageSpecDecl;
}

class RemoveUnusedVarAnalysisVisitor;

class RemoveUnusedVar : public Transformation {
friend class RemoveUnusedVarAnalysisVisitor;

public:

  RemoveUnusedVar(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      AnalysisVisitor(NULL),
      TheVarDecl(NULL)
  { }

  ~RemoveUnusedVar(void);

private:
  
  virtual void Initialize(clang::ASTContext &context);

  virtual bool HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void removeVarDecl(void);

  void removeVarDeclFromLinkageSpecDecl(const clang::LinkageSpecDecl *LinkageD,
                                        const clang::VarDecl *VD);

  llvm::DenseMap<const clang::VarDecl *, clang::DeclGroupRef> VarToDeclGroup;

  RemoveUnusedVarAnalysisVisitor *AnalysisVisitor;

  clang::VarDecl *TheVarDecl;

  // Unimplemented
  RemoveUnusedVar(void);

  RemoveUnusedVar(const RemoveUnusedVar &);

  void operator=(const RemoveUnusedVar &);
};
#endif
