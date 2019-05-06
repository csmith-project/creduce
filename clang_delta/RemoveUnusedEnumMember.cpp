//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2015, 2016, 2019 The University of Utah
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

  for (EnumDecl::enumerator_iterator I = ED->enumerator_begin(),
       E = ED->enumerator_end(); I != E; ++I) {
    if (!(*I)->isReferenced()) {
      ConsumerInstance->ValidInstanceNum++;
      if (ConsumerInstance->ValidInstanceNum ==
          ConsumerInstance->TransformationCounter) {
        ConsumerInstance->TheEnumIterator = I;
        ConsumerInstance->TheEnumDecl = ED;
      }
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

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  TransAssert(TheEnumDecl && "NULL TheEnumDecl!");

  removeEnumConstantDecl();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void RemoveUnusedEnumMember::removeEnumConstantDecl()
{
  SourceLocation StartLoc = (*TheEnumIterator)->getBeginLoc();
  if (StartLoc.isMacroID()) {
    CharSourceRange Range = SrcManager->getExpansionRange(StartLoc);
    StartLoc = Range.getBegin();
  }
  SourceLocation EndLoc = (*TheEnumIterator)->getEndLoc();
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

