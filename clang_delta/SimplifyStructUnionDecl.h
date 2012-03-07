//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef SIMPLIFY_STRUCT_UNION_DECL_H
#define SIMPLIFY_STRUCT_UNION_DECL_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class RecordDecl;
}

class SimplifyStructUnionDecl : public Transformation {

public:

  SimplifyStructUnionDecl(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc)
  { }

  ~SimplifyStructUnionDecl(void);

private:
  
  typedef llvm::DenseMap<const clang::Decl*, void *> 
            RecordDeclToDeclGroupMap;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void addOneRecordDecl(const clang::RecordDecl *RD, clang::DeclGroupRef DGR);

  void doCombination();

  RecordDeclToDeclGroupMap RecordDeclToDeclGroup;

  llvm::SmallVector<void *, 2> TheDeclGroupRefs;

  // Unimplemented
  SimplifyStructUnionDecl(void);

  SimplifyStructUnionDecl(const SimplifyStructUnionDecl &);

  void operator=(const SimplifyStructUnionDecl &);
};
#endif
