//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "RemoveNamespace.h"

#include <sstream>
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;
using namespace llvm;

static const char *DescriptionMsg =
"Remove namespaces. This pass tries to remove namespace without \
introducing name conflicts. \n";

static RegisterTransformation<RemoveNamespace>
         Trans("remove-namespace", DescriptionMsg);

class RemoveNamespaceASTVisitor : public 
  RecursiveASTVisitor<RemoveNamespaceASTVisitor> {

public:
  explicit RemoveNamespaceASTVisitor(RemoveNamespace *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitNamespaceDecl(NamespaceDecl *ND);

private:
  RemoveNamespace *ConsumerInstance;

};

class RemoveNamespaceRewriteVisitor : public 
  RecursiveASTVisitor<RemoveNamespaceRewriteVisitor> {

public:
  explicit RemoveNamespaceRewriteVisitor(RemoveNamespace *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitNamespaceDecl(NamespaceDecl *ND);

private:
  RemoveNamespace *ConsumerInstance;

};

bool RemoveNamespaceASTVisitor::VisitNamespaceDecl(NamespaceDecl *ND)
{
  ConsumerInstance->handleOneNamespaceDecl(ND);
  return true;
}

bool RemoveNamespaceRewriteVisitor::VisitNamespaceDecl(NamespaceDecl *ND)
{
  const NamespaceDecl *CanonicalND = ND->getCanonicalDecl();
  if (CanonicalND != ConsumerInstance->TheNamespaceDecl)
    return true;

  ConsumerInstance->removeNamespace(ND);
  return true;
}

void RemoveNamespace::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new RemoveNamespaceASTVisitor(this);
  RewriteVisitor = new RemoveNamespaceRewriteVisitor(this);
}

bool RemoveNamespace::HandleTopLevelDecl(DeclGroupRef D) 
{
  if (TransformationManager::isCLangOpt()) {
    return true;
  }

  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    CollectionVisitor->TraverseDecl(*I);
  }
  return true;
}
 
void RemoveNamespace::HandleTranslationUnit(ASTContext &Ctx)
{
  if (TransformationManager::isCLangOpt()) {
    ValidInstanceNum = 0;
  }

  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  TransAssert(RewriteVisitor && "NULL RewriteVisitor!");
  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  RewriteVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

bool RemoveNamespace::hasNameConflict(const NamedDecl *ND, 
                                      const DeclContext *ParentCtx)
{
  DeclarationName Name = ND->getDeclName();
  DeclContextLookupConstResult Result = ParentCtx->lookup(Name);
  return (Result.first != Result.second);
}

// A using declaration in the removed namespace could cause
// name conflict, e.g.,
// namespace NS1 {
//   void foo(void) {}
// }
// namespace NS2 {
//   using NS1::foo;
//   void bar() { ... foo(); ... }
// }
// void foo() {...}
// void func() {... foo(); ...}
// if we remove NS2, then foo() in func() will become ambiguous.
// In this case, we need to replace the first invocation of foo()
// with NS1::foo()
void RemoveNamespace::handleOneUsingShadowDecl(const UsingShadowDecl *UD,
                                               const DeclContext *ParentCtx)
{
  const NamedDecl *ND = UD->getTargetDecl();
  if (!hasNameConflict(ND, ParentCtx))
    return;
  
  const DeclContext *NDCtx = 
    ND->getDeclContext()->getEnclosingNamespaceContext();
  TransAssert(NDCtx && "Bad DeclContext!");
  const NamespaceDecl *NDNamespace = dyn_cast<NamespaceDecl>(NDCtx);
  TransAssert(NDNamespace && "Bad Namespace!");
  const IdentifierInfo *IdInfo = ND->getIdentifier();
  
  std::string NewName = NDNamespace->getNameAsString();
  NewName += "::";
  NewName += IdInfo->getName();
  NamedDeclToNewName[ND] = NewName;
  
  // the tied UsingDecl becomes useless, and hence it's removable
  const UsingDecl *D = UD->getUsingDecl();
  UselessUsingDecls.insert(D);
}

// For the similar reason as dealing with using declarations,
// we need to resolve the possible name conflicts introduced by
// using directives
void RemoveNamespace::handleOneUsingDirectiveDecl(const UsingDirectiveDecl *UD,
                                                  const DeclContext *ParentCtx)
{
  const NamespaceDecl *ND = UD->getNominatedNamespace();
  TransAssert(!ND->isAnonymousNamespace() && 
              "Cannot have anonymous namespaces!");
  std::string NamespaceName = ND->getNameAsString();

  bool Removable = true;
  for (DeclContext::decl_iterator I = ND->decls_begin(), E = ND->decls_end();
       I != E; ++I) {
    const NamedDecl *NamedD = dyn_cast<NamedDecl>(*I);
    if (!NamedD)
      continue;

    if (!isa<TemplateDecl>(NamedD) && !isa<TypeDecl>(NamedD) && 
        !isa<ValueDecl>(NamedD))
      continue;

    if (!hasNameConflict(NamedD, ParentCtx)) {
      Removable = false;
      continue;
    }

    const IdentifierInfo *IdInfo = NamedD->getIdentifier();
    std::string NewName = ND->getNameAsString();
    NewName += "::";
    NewName += IdInfo->getName();
    NamedDeclToNewName[NamedD] = NewName;
  }

  if (Removable)
    UselessUsingDirectiveDecls.insert(UD);
}

void RemoveNamespace::handleOneNamedDecl(const NamedDecl *ND,
                                         const DeclContext *ParentCtx,
                                         const std::string &NamespaceName)
{
  Decl::Kind K = ND->getKind();
  switch (K) {
  case Decl::UsingShadow: {
    const UsingShadowDecl *D = dyn_cast<UsingShadowDecl>(ND);
    handleOneUsingShadowDecl(D, ParentCtx);
    break;
  }

  case Decl::UsingDirective: {
    const UsingDirectiveDecl *D = dyn_cast<UsingDirectiveDecl>(ND);
    handleOneUsingDirectiveDecl(D, ParentCtx);
    break;
  }

  default:
    if (isa<NamespaceAliasDecl>(ND) || isa<TemplateDecl>(ND) ||
        isa<TypeDecl>(ND) || isa<ValueDecl>(ND)) {
      if (!hasNameConflict(ND, ParentCtx))
        break;

      std::string NewName = NamePrefix + NamespaceName;
      NamedDeclToNewName[ND] = NewName;
    }
  }
}

void RemoveNamespace::addNamedDeclsFromNamespace(const NamespaceDecl *ND)
{
  // We don't care about name-lookup for friend's functions, so just
  // retrive ParentContext rather than LookupParent
  const DeclContext *ParentCtx = ND->getParent();
  std::string NamespaceName;

  if (ND->isAnonymousNamespace()) {
    std::stringstream TmpSS;
    TmpSS << AnonNamePrefix << AnonNamespaceCounter;
    NamespaceName = TmpSS.str();
    AnonNamespaceCounter++;
  }
  else {
    NamespaceName = ND->getNameAsString();
  }

  for (DeclContext::decl_iterator I = ND->decls_begin(), E = ND->decls_end();
       I != E; ++I) {
    if ( const NamedDecl *D = dyn_cast<NamedDecl>(*I) )
      handleOneNamedDecl(D, ParentCtx, NamespaceName);
  }
}

bool RemoveNamespace::handleOneNamespaceDecl(const NamespaceDecl *ND)
{
  const NamespaceDecl *CanonicalND = ND->getCanonicalDecl();
  if (VisitedND.count(CanonicalND)) {
    if (TheNamespaceDecl == CanonicalND) {
      addNamedDeclsFromNamespace(ND);
    }
    return true;
  }

  VisitedND.insert(CanonicalND);
  ValidInstanceNum++;
  if (ValidInstanceNum == TransformationCounter) {
    TheNamespaceDecl = CanonicalND;
    addNamedDeclsFromNamespace(ND);
  }
  return true;
}

void RemoveNamespace::removeNamespace(const NamespaceDecl *ND)
{
  // Remove the right brace first
  SourceLocation StartLoc = ND->getRBraceLoc();
  TheRewriter.RemoveText(StartLoc, 1);

  // Then remove name and the left brace
  StartLoc = ND->getLocStart();
  TransAssert(StartLoc.isValid() && "Invalid Namespace LocStart!");

  const char *StartBuf = SrcManager->getCharacterData(StartLoc);
  SourceRange NDRange = ND->getSourceRange();
  int RangeSize = TheRewriter.getRangeSize(NDRange);
  TransAssert((RangeSize != -1) && "Bad Namespace Range!");

  std::string NDStr(StartBuf, RangeSize);
  size_t Pos = NDStr.find('{');
  TransAssert((Pos != std::string::npos) && "Cannot find LBrace!");
  SourceLocation EndLoc = StartLoc.getLocWithOffset(Pos);
  TheRewriter.RemoveText(SourceRange(StartLoc, EndLoc));
}

RemoveNamespace::~RemoveNamespace(void)
{
  if (CollectionVisitor)
    delete CollectionVisitor;
  if (RewriteVisitor)
    delete RewriteVisitor;
}

