//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef COMBINE_GLOBAL_VAR_DECL_H
#define COMBINE_GLOBAL_VAR_DECL_H

#include <string>
#include <vector>
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/DenseMap.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class Type;
}

class CombineGlobalVarDecl : public Transformation {
public:

  CombineGlobalVarDecl(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc)
  { }

  ~CombineGlobalVarDecl(void);

private:
  
  typedef llvm::SmallVector<void *, 20> DeclGroupVector;

  typedef llvm::DenseMap<const clang::Type *, DeclGroupVector *> TypeToDeclMap;

  virtual void Initialize(clang::ASTContext &context);

  virtual bool HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void doCombination();

  TypeToDeclMap AllDeclGroups;

  llvm::SmallVector<void *, 2> TheDeclGroupRefs;

  // Unimplemented
  CombineGlobalVarDecl(void);

  CombineGlobalVarDecl(const CombineGlobalVarDecl &);

  void operator=(const CombineGlobalVarDecl &);
};
#endif
