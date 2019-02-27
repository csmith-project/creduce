//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2015, 2016, 2017 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "RemoveTrivialBaseTemplate.h"

#include "clang/Basic/SourceManager.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg = 
"This pass removes a base class if it is an instantiation from a class \
template which doesn't have definition. \n";

static RegisterTransformation<RemoveTrivialBaseTemplate>
         Trans("remove-trivial-base-template", DescriptionMsg);

class RemoveTrivialBaseTemplateBaseVisitor : public 
  RecursiveASTVisitor<RemoveTrivialBaseTemplateBaseVisitor> {

public:
  explicit RemoveTrivialBaseTemplateBaseVisitor(
             RemoveTrivialBaseTemplate *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitCXXRecordDecl(CXXRecordDecl *CXXRD);

private:
  RemoveTrivialBaseTemplate *ConsumerInstance;
};

bool RemoveTrivialBaseTemplateBaseVisitor::VisitCXXRecordDecl(
       CXXRecordDecl *CXXRD)
{
  ConsumerInstance->handleOneCXXRecordDecl(CXXRD);
  return true;
}

void RemoveTrivialBaseTemplate::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new RemoveTrivialBaseTemplateBaseVisitor(this);
}

void RemoveTrivialBaseTemplate::HandleTranslationUnit(ASTContext &Ctx)
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

  TransAssert(TheDerivedClass && "TheDerivedClass is NULL!");
  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  removeBaseSpecifier();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void RemoveTrivialBaseTemplate::handleOneCXXRecordDecl(
       const CXXRecordDecl *CXXRD)
{
  if (isInIncludedFile(CXXRD) || isSpecialRecordDecl(CXXRD) ||
      !CXXRD->hasDefinition())
    return;

  const CXXRecordDecl *CanonicalRD = CXXRD->getCanonicalDecl();
  if (VisitedCXXRecordDecls.count(CanonicalRD))
    return;
  VisitedCXXRecordDecls.insert(CanonicalRD);
  unsigned Idx = 0;
  for (CXXRecordDecl::base_class_const_iterator I = CanonicalRD->bases_begin(),
       E = CanonicalRD->bases_end(); I != E; ++I, ++Idx) {

    const CXXBaseSpecifier *BS = I;
    const Type *Ty = BS->getType().getTypePtr();
    const CXXRecordDecl *Base = getBaseDeclFromType(Ty);
    if (!Base || getNumExplicitDecls(Base)) {
      continue;
    }
    const ClassTemplateDecl *TmplD = Base->getDescribedClassTemplate();
    if (!TmplD)
      continue;
    ValidInstanceNum++;
    if (ValidInstanceNum == TransformationCounter) {
      TheDerivedClass = CanonicalRD;
      ThePos = Idx;
    }
  }
}

void RemoveTrivialBaseTemplate::removeBaseSpecifier(void)
{
  unsigned NumBases = TheDerivedClass->getNumBases();
  TransAssert((NumBases >= 1) && "TheDerivedClass doesn't have any base!");
  if (NumBases == 1) {
    SourceLocation StartLoc = TheDerivedClass->getLocation();
    StartLoc = RewriteHelper->getLocationUntil(StartLoc, ':');
    SourceLocation EndLoc = RewriteHelper->getLocationUntil(StartLoc, '{');
    EndLoc = EndLoc.getLocWithOffset(-1);

    TheRewriter.RemoveText(SourceRange(StartLoc, EndLoc));
    return;
  }

  TransAssert((ThePos < NumBases) && "Invalid ThePos for the base specifier!");
  CXXRecordDecl::base_class_const_iterator I = TheDerivedClass->bases_begin();
  if (ThePos == 0) {
    RewriteHelper->removeTextUntil((*I).getSourceRange(), ',');
    return;
  }

  ++I;
  CXXRecordDecl::base_class_const_iterator E = TheDerivedClass->bases_end();
  unsigned Idx = 1;
  for (; I != E; ++I, ++Idx) {
    if (Idx == ThePos) {
      SourceRange Range = (*I).getSourceRange();
      SourceLocation EndLoc = RewriteHelper->getEndLocationFromBegin(Range);
      RewriteHelper->removeTextFromLeftAt(Range, ',', EndLoc);
      return;
    }
  }
  TransAssert(0 && "Unreachable code!");
}

RemoveTrivialBaseTemplate::~RemoveTrivialBaseTemplate(void)
{
  delete CollectionVisitor;
}

