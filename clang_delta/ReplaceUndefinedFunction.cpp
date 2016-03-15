//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2015 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "ReplaceUndefinedFunction.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"If there are more than one undefined function with the same type,\
pick up one and then replace all others with the selected one. \n";

static RegisterTransformation<ReplaceUndefinedFunction>
         Trans("replace-undefined-function", DescriptionMsg);

class ReplaceUndefFuncCollectionVisitor : public 
  RecursiveASTVisitor<ReplaceUndefFuncCollectionVisitor> {

public:

  explicit ReplaceUndefFuncCollectionVisitor(ReplaceUndefinedFunction *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitFunctionDecl(FunctionDecl *FD);

private:

  ReplaceUndefinedFunction *ConsumerInstance;
};

class ReplaceUndefFuncRewriteVisitor : public 
  RecursiveASTVisitor<ReplaceUndefFuncRewriteVisitor> {

public:

  explicit ReplaceUndefFuncRewriteVisitor(ReplaceUndefinedFunction *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitFunctionDecl(FunctionDecl *FD);

  bool VisitCallExpr(CallExpr *CE);

private:

  ReplaceUndefinedFunction *ConsumerInstance;
};

bool ReplaceUndefFuncCollectionVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  if (ConsumerInstance->isInIncludedFile(FD) || FD->hasBody())
    return true;
  ConsumerInstance->handleOneFunctionDecl(FD->getCanonicalDecl());

  return true;
}

bool ReplaceUndefFuncRewriteVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  if (FD->getCanonicalDecl() != ConsumerInstance->ReplacedFunctionDecl)
    return true;

  TransAssert(!FD->hasBody() && "FD cannot have any definition!");
  ConsumerInstance->RewriteHelper->removeDecl(FD);
  return true;
}

bool ReplaceUndefFuncRewriteVisitor::VisitCallExpr(CallExpr *CE)
{
  FunctionDecl *FD = CE->getDirectCallee();
  // skip CXXMethodDecl for now
  if (!FD || dyn_cast<CXXMethodDecl>(FD))
    return true;

  if (FD->getCanonicalDecl() == ConsumerInstance->ReplacedFunctionDecl) {
    ConsumerInstance->TheRewriter.ReplaceText(CE->getLocStart(),
      ConsumerInstance->ReplacedFunctionDecl->getNameAsString().size(),
      ConsumerInstance->ReplacingFunctionDecl->getNameAsString());
  }
  return true;
}

void ReplaceUndefinedFunction::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new ReplaceUndefFuncCollectionVisitor(this);
  RewriteVisitor = new ReplaceUndefFuncRewriteVisitor(this);
}

void ReplaceUndefinedFunction::HandleTranslationUnit(ASTContext &Ctx)
{
  TransAssert(CollectionVisitor && "NULL CollectionVisitor!");
  CollectionVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
  doAnalysis();

  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);
  TransAssert(RewriteVisitor && "NULL RewriteVisitor!");
  RewriteVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void ReplaceUndefinedFunction::doAnalysis(void)
{
  for (FunctionSetMap::iterator I = ReplaceableFunctions.begin(),
       E = ReplaceableFunctions.end(); I != E; ++I) {
    const FunctionDeclSet *FDSet = (*I).second;
    if (!FDSet)
      continue;
    for (FunctionDeclSet::iterator FI = FDSet->begin(),
         FE = FDSet->end(); FI != FE; ++FI) {
      ValidInstanceNum++;
      if (ValidInstanceNum == TransformationCounter) {
        ReplacingFunctionDecl = (*I).first;
        ReplacedFunctionDecl = (*FI);
      }
    }
  }
}

void ReplaceUndefinedFunction::handleOneFunctionDecl(const FunctionDecl *FD)
{
  QualType FDQualTy = FD->getType();
  for (FunctionSetMap::iterator I = ReplaceableFunctions.begin(),
       E = ReplaceableFunctions.end(); I != E; ++I) {
    const FunctionDecl *ReplaceableFD = (*I).first;
    FunctionDeclSet *FDSet = (*I).second;
    QualType QualTy = ReplaceableFD->getType();

    if (!Context->hasSameType(FDQualTy, QualTy))
      continue;

    TransAssert(FDSet && "NULL FDSet");
    FDSet->insert(FD);
    return;
  }

  FunctionDeclSet *FDSet = new FunctionDeclSet();
  ReplaceableFunctions[FD] = FDSet;
}

ReplaceUndefinedFunction::~ReplaceUndefinedFunction(void)
{
  delete CollectionVisitor;
  delete RewriteVisitor;

  for (FunctionSetMap::iterator I = ReplaceableFunctions.begin(),
       E = ReplaceableFunctions.end(); I != E; ++I) {
    const FunctionDeclSet *FDSet = (*I).second;
    if (!FDSet)
      delete FDSet;
  }
}

