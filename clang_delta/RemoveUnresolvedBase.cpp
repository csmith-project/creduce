//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2015, 2017 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "RemoveUnresolvedBase.h"

#include "clang/Basic/SourceManager.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg = 
"This pass tries to remove a base specifier if we cannot \
resolve it. \n";

static RegisterTransformation<RemoveUnresolvedBase>
         Trans("remove-unresolved-base", DescriptionMsg);

class RemoveUnresolvedBaseASTVisitor : public 
  RecursiveASTVisitor<RemoveUnresolvedBaseASTVisitor> {

public:
  explicit RemoveUnresolvedBaseASTVisitor(RemoveUnresolvedBase *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitCXXRecordDecl(CXXRecordDecl *CXXRD);

private:
  RemoveUnresolvedBase *ConsumerInstance;
};

bool RemoveUnresolvedBaseASTVisitor::VisitCXXRecordDecl(CXXRecordDecl *CXXRD)
{
  if (ConsumerInstance->isInIncludedFile(CXXRD) || !CXXRD->hasDefinition())
    return true;

  const CXXRecordDecl *CanonicalRD = CXXRD->getCanonicalDecl();
  if (ConsumerInstance->VisitedCXXRecordDecls.count(CanonicalRD))
    return true;
  ConsumerInstance->VisitedCXXRecordDecls.insert(CanonicalRD);

  unsigned Idx = 0;
  for (CXXRecordDecl::base_class_const_iterator I = 
       CanonicalRD->bases_begin(), E = CanonicalRD->bases_end(); I != E; ++I) {
    const CXXBaseSpecifier *BS = I;
    const Type *Ty = BS->getType().getTypePtr();
    const CXXRecordDecl *Base = ConsumerInstance->getBaseDeclFromType(Ty);
    if (Base) {
      Idx++;
      continue;
    }

    ConsumerInstance->ValidInstanceNum++;
    if (ConsumerInstance->ValidInstanceNum == 
        ConsumerInstance->TransformationCounter) {
      ConsumerInstance->TheDerivedClass = CanonicalRD;
      ConsumerInstance->TheBaseSpecifier = BS;
      ConsumerInstance->TheIndex = Idx;
    }
    Idx++;
  }
  return true;
}

void RemoveUnresolvedBase::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new RemoveUnresolvedBaseASTVisitor(this);
}

void RemoveUnresolvedBase::HandleTranslationUnit(ASTContext &Ctx)
{
  if (TransformationManager::isCLangOpt() ||
      TransformationManager::isOpenCLLangOpt()) {
    ValidInstanceNum = 0;
  }
  else {
    CollectionVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
  }

  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  TransAssert(TheDerivedClass && "NULL TheDerivedClass!");
  TransAssert(TheBaseSpecifier && "NULL TheBaseSpecifier!");
  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  removeBaseSpecifier();
  
  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void RemoveUnresolvedBase::removeBaseSpecifier(void)
{
  unsigned NumBases = TheDerivedClass->getNumBases();
  TransAssert((NumBases >= 1) && "TheDerivedClass doesn't have any base!");
  CXXRecordDecl::base_class_const_iterator I = TheDerivedClass->bases_begin();

  if (NumBases == 1) {
    TransAssert((TheIndex == 0) && "Invalid Index for the base specifier!");
    TransAssert((I == TheBaseSpecifier) && "Unmatched base specifier!"); 
    (void)I;
    SourceLocation StartLoc = TheDerivedClass->getLocation();
    StartLoc = RewriteHelper->getLocationUntil(StartLoc, ':');
    SourceLocation EndLoc = RewriteHelper->getLocationUntil(StartLoc, '{');
    EndLoc = EndLoc.getLocWithOffset(-1);

    TheRewriter.RemoveText(SourceRange(StartLoc, EndLoc));
    return;
  }

  SourceRange Range = TheBaseSpecifier->getSourceRange();
  if (TheIndex == 0) {
    RewriteHelper->removeTextUntil(Range, ',');
  }
  else {
    SourceLocation EndLoc = RewriteHelper->getEndLocationFromBegin(Range);
    RewriteHelper->removeTextFromLeftAt(Range, ',', EndLoc);
  } 
}

RemoveUnresolvedBase::~RemoveUnresolvedBase(void)
{
  delete CollectionVisitor;
}

