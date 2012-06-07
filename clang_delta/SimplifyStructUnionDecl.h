//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef SIMPLIFY_STRUCT_UNION_DECL_H
#define SIMPLIFY_STRUCT_UNION_DECL_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class RecordDecl;
  class Type;
  class VarDecl;
}

class SimplifyStructUnionDeclVisitor;

class SimplifyStructUnionDecl : public Transformation {

friend class SimplifyStructUnionDeclVisitor;

public:

  SimplifyStructUnionDecl(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      AnalysisVisitor(NULL),
      TheRecordDecl(NULL),
      SafeToRemoveName(true)
  { }

  ~SimplifyStructUnionDecl(void);

private:
  
  typedef llvm::SmallPtrSet<const clang::VarDecl *, 5> VarDeclSet;

  typedef llvm::DenseMap<const clang::Decl*, void *> 
            RecordDeclToDeclGroupMap;

  virtual void Initialize(clang::ASTContext &context);

  virtual bool HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void addOneRecordDecl(const clang::RecordDecl *RD, clang::DeclGroupRef DGR);

  void doCombination();

  bool handleOneDeclarator(const clang::Type *Ty);

  const clang::RecordDecl *getBaseRecordDecl(const clang::Type *Ty);

  bool isSafeToRemoveName(void);

  RecordDeclToDeclGroupMap RecordDeclToDeclGroup;

  llvm::SmallVector<void *, 2> TheDeclGroupRefs;

  VarDeclSet CombinedVars;

  SimplifyStructUnionDeclVisitor *AnalysisVisitor;

  const clang::RecordDecl *TheRecordDecl;

  bool SafeToRemoveName;

  // Unimplemented
  SimplifyStructUnionDecl(void);

  SimplifyStructUnionDecl(const SimplifyStructUnionDecl &);

  void operator=(const SimplifyStructUnionDecl &);
};
#endif
