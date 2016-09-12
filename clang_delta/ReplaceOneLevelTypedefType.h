//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2016 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef REPLACE_ONE_LEVEL_TYPEDEF_TYPE_H
#define REPLACE_ONE_LEVEL_TYPEDEF_TYPE_H

#include "Transformation.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/SmallVector.h"
#include "clang/AST/TypeLoc.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class TypedefDecl;
}

class ReplaceOneLevelTypedefTypeCollectionVisitor;

class ReplaceOneLevelTypedefType : public Transformation {
friend class ReplaceOneLevelTypedefTypeCollectionVisitor;

public:
  ReplaceOneLevelTypedefType(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL)
  {}

  ~ReplaceOneLevelTypedefType(void);

private:

  typedef llvm::SmallVector<clang::TypedefTypeLoc, 10>
    TypedefTypeLocVector;

  typedef llvm::MapVector<const clang::TypedefDecl *, 
                         TypedefTypeLocVector *>
    TypedefDeclToRefMap;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void handleOneTypedefTypeLoc(clang::TypedefTypeLoc TLoc);

  void analyzeTypeLocs();

  void rewriteTypedefType();

  void removeTypedefs();

  TypedefDeclToRefMap AllTypeDecls;

  clang::TypedefTypeLoc TheTypeLoc;

  ReplaceOneLevelTypedefTypeCollectionVisitor *CollectionVisitor;

  const clang::TypedefDecl *TheTypedefDecl;

  // Unimplemented
  ReplaceOneLevelTypedefType(void);

  ReplaceOneLevelTypedefType(const ReplaceOneLevelTypedefType &);

  void operator=(const ReplaceOneLevelTypedefType &);
};

#endif

