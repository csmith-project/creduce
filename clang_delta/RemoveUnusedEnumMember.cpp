//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013 The University of Utah
// Copyright (c) 2012 Konstantin Tokarev <annulen@yandex.ru>
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "RemoveUnusedEnumMember.h"

#include <cctype>
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Remove unused enum member declarations. \n";

static RegisterTransformation<RemoveUnusedEnumMember>
         Trans("remove-unused-enum-member", DescriptionMsg);

class RemoveUnusedEnumMemberAnalysisVisitor : public
  RecursiveASTVisitor<RemoveUnusedEnumMemberAnalysisVisitor> {
public:

  explicit RemoveUnusedEnumMemberAnalysisVisitor(RemoveUnusedEnumMember *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitEnumDecl(EnumDecl *ED);

private:

  RemoveUnusedEnumMember *ConsumerInstance;
};

bool RemoveUnusedEnumMemberAnalysisVisitor::VisitEnumDecl(EnumDecl *ED)
{
  // Skip enums which are not in the main file
  // Rewriting outside of the main file is currently not supported
  if(!ConsumerInstance->SrcManager->isInMainFile(ED->getLocStart()))
  {
    return true;
  }

  if (ED != ED->getCanonicalDecl())
    return true;

  EnumDecl::enumerator_iterator Previous = ED->enumerator_begin();
  for (EnumDecl::enumerator_iterator I = ED->enumerator_begin(), E = ED->enumerator_end();
      I != E; ++I) {
    if (!(*I)->isReferenced()) {
      ConsumerInstance->ValidInstanceNum++;
      if (ConsumerInstance->ValidInstanceNum ==
          ConsumerInstance->TransformationCounter) {
        ConsumerInstance->TheEnumIterator = I;
        ConsumerInstance->TheEnumDecl = ED;
        ConsumerInstance->TheEnumIteratorPrevious = Previous;
      }
    }
    Previous = I;
  }
  return true;
}

void RemoveUnusedEnumMember::Initialize(ASTContext &context)
{
  Transformation::Initialize(context);
  AnalysisVisitor = new RemoveUnusedEnumMemberAnalysisVisitor(this);
}

void RemoveUnusedEnumMember::HandleTranslationUnit(ASTContext &Ctx)
{
  AnalysisVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  TransAssert(TheEnumDecl && "NULL TheEnumDecl!");

  removeEnumConstantDecl();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void RemoveUnusedEnumMember::removeEnumConstantDecl()
{
  EnumDecl::enumerator_iterator Next = TheEnumIterator;
  ++Next;

  EnumDecl::enumerator_iterator Previous = TheEnumIteratorPrevious;

  if (TheEnumIterator == TheEnumDecl->enumerator_begin() && Next == TheEnumDecl->enumerator_end()) {
    // There is no "," here
    TheRewriter.RemoveText((*TheEnumIterator)->getSourceRange());
  } else if (Next == TheEnumDecl->enumerator_end()) {
    // Remove previous ","
    TheRewriter.RemoveText(SourceRange((*Previous)->getLocEnd().getLocWithOffset(1), (*TheEnumIterator)->getLocEnd()));
  }
  else {
    // Remove next ","
    TheRewriter.RemoveText(SourceRange((*TheEnumIterator)->getLocStart(), (*Next)->getLocStart().getLocWithOffset(-1)));
  }
}

RemoveUnusedEnumMember::~RemoveUnusedEnumMember()
{
  delete AnalysisVisitor;
}

