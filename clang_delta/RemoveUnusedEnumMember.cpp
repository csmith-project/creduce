//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2015, 2016, 2019, 2020 The University of Utah
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
#include "clang/Lex/Lexer.h"
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

  explicit RemoveUnusedEnumMemberAnalysisVisitor(
             RemoveUnusedEnumMember *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitEnumDecl(EnumDecl *ED);

private:

  RemoveUnusedEnumMember *ConsumerInstance;
};

bool RemoveUnusedEnumMemberAnalysisVisitor::VisitEnumDecl(EnumDecl *ED)
{
  if (ConsumerInstance->isInIncludedFile(ED) || ED != ED->getCanonicalDecl())
    return true;

  /* Make it backward compatible where --to-counter is unset. */
  if (ConsumerInstance->ToCounter == -1)
    ConsumerInstance->ToCounter = ConsumerInstance->TransformationCounter;

  for (EnumDecl::enumerator_iterator I = ED->enumerator_begin(),
       E = ED->enumerator_end(); I != E; ++I) {
    if (!(*I)->isReferenced()) {
      ConsumerInstance->ValidInstanceNum++;
      if (ConsumerInstance->ValidInstanceNum >=
          ConsumerInstance->TransformationCounter
          && ConsumerInstance->ValidInstanceNum <=
             ConsumerInstance->ToCounter)
        ConsumerInstance->EnumValues.push_back (I);
    }
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
  if (ToCounter != -1 && ToCounter > ValidInstanceNum) {
    TransError = TransToCounterTooBigError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  for (auto ev : EnumValues) {
    removeEnumConstantDecl(ev);
  }

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void RemoveUnusedEnumMember::removeEnumConstantDecl(clang::EnumDecl::enumerator_iterator it)
{
  SourceLocation StartLoc = it->getBeginLoc();
  if (StartLoc.isMacroID()) {
    CharSourceRange Range = SrcManager->getExpansionRange(StartLoc);
    StartLoc = Range.getBegin();
  }
  SourceLocation EndLoc = it->getEndLoc();
  if (EndLoc.isMacroID()) {
    CharSourceRange Range = SrcManager->getExpansionRange(EndLoc);
    EndLoc = Range.getEnd();
  }
  SourceLocation CommaLoc = Lexer::findLocationAfterToken(
    EndLoc, tok::comma, *SrcManager, Context->getLangOpts(),
    /*SkipTrailingWhitespaceAndNewLine=*/false);
  if (CommaLoc.isValid())
    EndLoc = CommaLoc;
  TheRewriter.RemoveText(SourceRange(StartLoc, EndLoc));
}

RemoveUnusedEnumMember::~RemoveUnusedEnumMember()
{
  delete AnalysisVisitor;
}

