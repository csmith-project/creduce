//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef REMOVE_NAMESPACE_H
#define REMOVE_NAMESPACE_H

#include <string>
#include "Transformation.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/DenseMap.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class NamespaceDecl;
}

class RemoveNamespaceASTVisitor;
class RemoveNamespaceRewriteVisitor;

class RemoveNamespace : public Transformation {
friend class RemoveNamespaceASTVisitor;
friend class RemoveNamespaceRewriteVisitor;

public:

  RemoveNamespace(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      RewriteVisitor(NULL),
      TheNamespaceDecl(NULL)
  { }

  ~RemoveNamespace(void);

private:
  
  typedef llvm::SmallPtrSet<const clang::NamespaceDecl *, 15> NamespaceDeclSet;

  typedef llvm::DenseMap<const clang::NamedDecl *, std::string *>
            NamedDeclToNameMap;

  virtual void Initialize(clang::ASTContext &context);

  virtual bool HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void addNamedDeclsFromNamespace(const clang::NamespaceDecl *ND);

  bool handleOneNamespaceDecl(const clang::NamespaceDecl *ND);

  void removeNamespace(const clang::NamespaceDecl *ND);

  NamespaceDeclSet VisitedND;

  // a mapping from NamedDecls in TheNamespaceDecl to their new names
  // after TheNamespaceDecl is removed. This map only stores those
  // NamedDecls which need to be renamed.
  NamedDeclToNameMap NamedDeclToNewName;

  RemoveNamespaceASTVisitor *CollectionVisitor;

  RemoveNamespaceRewriteVisitor *RewriteVisitor;

  const clang::NamespaceDecl *TheNamespaceDecl;

  // Unimplemented
  RemoveNamespace(void);

  RemoveNamespace(const RemoveNamespace &);

  void operator=(const RemoveNamespace &);
};
#endif
