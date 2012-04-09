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
#include "clang/AST/NestedNameSpecifier.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class NamespaceDecl;
  class NamedDecl;
  class DeclContext;
  class UsingDirectiveDecl;
  class UsingShadowDecl;
  class UsingDecl;
}

class RemoveNamespaceASTVisitor;
class RemoveNamespaceRewriteVisitor;
class RemoveNamespaceRewriteNamespaceVisitor;

class RemoveNamespace : public Transformation {
friend class RemoveNamespaceASTVisitor;
friend class RemoveNamespaceRewriteVisitor;
friend class RemoveNamespaceRewriteNamespaceVisitor;

public:

  RemoveNamespace(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      RewriteVisitor(NULL),
      NamespaceRewriteVisitor(NULL),
      TheNamespaceDecl(NULL),
      NamePrefix("__trans_"),
      AnonNamePrefix("anon_"),
      AnonNamespaceCounter(0)
  { }

  ~RemoveNamespace(void);

private:
  
  typedef llvm::SmallPtrSet<const clang::NamespaceDecl *, 15> NamespaceDeclSet;

  typedef llvm::SmallPtrSet<const clang::UsingDecl *, 10> UsingDeclSet;

  typedef llvm::SmallPtrSet<const clang::UsingDirectiveDecl *, 10> 
            UsingDirectiveDeclSet;

  typedef llvm::DenseMap<const clang::NamedDecl *, std::string>
            NamedDeclToNameMap;

  virtual void Initialize(clang::ASTContext &context);

  virtual bool HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void addNamedDeclsFromNamespace(const clang::NamespaceDecl *ND);

  bool handleOneNamespaceDecl(clang::NamespaceDecl *ND);

  void removeNamespace(const clang::NamespaceDecl *ND);

  void removeUsingOrUsingDirectiveDecl(const clang::Decl *D);

  void handleOneNamedDecl(const clang::NamedDecl *ND, 
                          const clang::DeclContext *ParentCtx,
                          const std::string &NamespaceName);

  bool hasNameConflict(const clang::NamedDecl *ND,
                       const clang::DeclContext *ParentCtx);

  void handleOneUsingShadowDecl(const clang::UsingShadowDecl *UD,
                                const clang::DeclContext *ParentCtx);

  void handleOneUsingDirectiveDecl(const clang::UsingDirectiveDecl *UD,
                                   const clang::DeclContext *ParentCtx);

  void getQualifierAsString(clang::NestedNameSpecifierLoc Loc,
                            std::string &Str);

  bool getNewName(const clang::NamedDecl *ND, std::string &Name);

  NamespaceDeclSet VisitedND;

  UsingDeclSet UselessUsingDecls;

  UsingDirectiveDeclSet UselessUsingDirectiveDecls;

  // a mapping from NamedDecls in TheNamespaceDecl used in TheNamespaceDecl
  // to their new names after TheNamespaceDecl is removed. This map only 
  // stores those NamedDecls which need to be renamed.
  NamedDeclToNameMap NamedDeclToNewName;

  // a mapping from NamedDecl in other namespaces used in TheNamespaceDecl
  // to their new names after TheNamespaceDecl is removed.
  NamedDeclToNameMap UsingNamedDeclToNewName;

  RemoveNamespaceASTVisitor *CollectionVisitor;

  RemoveNamespaceRewriteVisitor *RewriteVisitor;

  RemoveNamespaceRewriteNamespaceVisitor *NamespaceRewriteVisitor;

  clang::NamespaceDecl *TheNamespaceDecl;

  const std::string NamePrefix;

  // Prefix for anonymous namespaces
  const std::string AnonNamePrefix;

  unsigned AnonNamespaceCounter;

  // Unimplemented
  RemoveNamespace(void);

  RemoveNamespace(const RemoveNamespace &);

  void operator=(const RemoveNamespace &);
};
#endif
