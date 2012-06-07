//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
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
  class EnumDecl;
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
      TheNamespaceDecl(NULL),
      NamePrefix("Trans_NS_"),
      AnonNamePrefix("Trans_Anon_NS"),
      AnonNamespaceCounter(0),
      isForUsingNamedDecls(false)
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

  bool isValidNamedDeclKind(const clang::NamedDecl *ND);

  void handleOneEnumDecl(const clang::EnumDecl *ED,
                         const std::string &Prefix,
                         NamedDeclToNameMap &NameMap,
                         const clang::DeclContext *ParentCtx);

  void handleOneUsingShadowDecl(const clang::UsingShadowDecl *UD,
                                const clang::DeclContext *ParentCtx);

  void handleOneUsingDirectiveDecl(const clang::UsingDirectiveDecl *UD,
                                   const clang::DeclContext *ParentCtx);

  void rewriteNamedDecls(void);

  bool getNewNameFromNameMap(const clang::NamedDecl *ND,
                             std::string &Name,
                             const NamedDeclToNameMap &NameMap);

  bool getNewNameByNameFromNameMap(const std::string &Name,
                                   std::string &NewName,
                                   const NamedDeclToNameMap &NameMap);

  bool getNewNamedDeclName(const clang::NamedDecl *ND, std::string &Name);

  bool getNewUsingNamedDeclName(const clang::NamedDecl *ND, std::string &Name);

  bool getNewName(const clang::NamedDecl *ND, std::string &Name);

  bool getNewNameByName(const std::string &Name, std::string &NewName);

  bool isGlobalNamespace(clang::NestedNameSpecifierLoc Loc);

  bool isTheNamespaceSpecifier(const clang::NestedNameSpecifier *NNS);

  void removeLastNamespaceFromUsingDecl(const clang::UsingDirectiveDecl *D,
                                        const clang::NamespaceDecl *ND);

  void replaceFirstNamespaceFromUsingDecl(const clang::UsingDirectiveDecl *D,
                                          const std::string &Name);

  bool isSuffix(std::string &Name, std::string &SpecifierName);

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

  clang::NamespaceDecl *TheNamespaceDecl;

  const std::string NamePrefix;

  // Prefix for anonymous namespaces
  const std::string AnonNamePrefix;

  unsigned AnonNamespaceCounter;

  // a flag to indicate if we are working on renaming UsingNamedDecl
  // The purpose of this flag is to avoid double visit. 
  // The RemoveNamespaceRewriteVisitor are used twice:
  // * one for TheNamespaceDecl. In this case, we only rename UsingNamedDecls
  //   without doing any other work
  // * another is for NamedDecls belonging to TheNamespaceDecl
  bool isForUsingNamedDecls;

  // Unimplemented
  RemoveNamespace(void);

  RemoveNamespace(const RemoveNamespace &);

  void operator=(const RemoveNamespace &);
};
#endif
