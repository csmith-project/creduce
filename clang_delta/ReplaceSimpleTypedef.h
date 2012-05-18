//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef REPLACE_SIMPLE_TYPEDEF_H
#define REPLACE_SIMPLE_TYPEDEF_H

#include "Transformation.h"
#include "llvm/ADT/SmallPtrSet.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class TypedefDecl;
};

class ReplaceSimpleTypedefCollectionVisitor;
class ReplaceSimpleTypedefRewriteVisitor;

class ReplaceSimpleTypedef : public Transformation {
friend class ReplaceSimpleTypedefCollectionVisitor;
friend class ReplaceSimpleTypedefRewriteVisitor;

public:
  ReplaceSimpleTypedef(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      RewriteVisitor(NULL),
      TyName(""),
      IsScalarType(false)
  {}

  ~ReplaceSimpleTypedef(void);

private:

  typedef llvm::SmallPtrSet<const clang::TypedefDecl *, 20> TypedefDeclsSet;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void handleOneTypedefDecl(const clang::TypedefDecl *CanonicalD);

  bool isValidType(const clang::Type *Ty, const clang::TypedefDecl *D);

  void removeTypedefs(void);

  TypedefDeclsSet VisitedTypedefDecls;

  ReplaceSimpleTypedefCollectionVisitor *CollectionVisitor;

  ReplaceSimpleTypedefRewriteVisitor *RewriteVisitor;

  const clang::TypedefDecl *TheTypedefDecl;

  std::string TyName;

  bool IsScalarType;

  // Unimplemented
  ReplaceSimpleTypedef(void);

  ReplaceSimpleTypedef(const ReplaceSimpleTypedef &);

  void operator=(const ReplaceSimpleTypedef &);
};

#endif

