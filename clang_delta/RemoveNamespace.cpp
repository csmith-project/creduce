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

void RemoveNamespace::addNamedDeclsFromNamespace(const NamespaceDecl *ND)
{
  // TODO
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

