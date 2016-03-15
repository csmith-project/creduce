//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2015 The University of Utah
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

#include "RemoveEnumMemberValue.h"

#include <cctype>
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Remove enum member value. \n";

static RegisterTransformation<RemoveEnumMemberValue>
         Trans("remove-enum-member-value", DescriptionMsg);

class RemoveEnumMemberValueAnalysisVisitor : public
  RecursiveASTVisitor<RemoveEnumMemberValueAnalysisVisitor> {
public:

  explicit RemoveEnumMemberValueAnalysisVisitor(RemoveEnumMemberValue *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitEnumConstantDecl(EnumConstantDecl *ECD);

private:

  RemoveEnumMemberValue *ConsumerInstance;
};

bool RemoveEnumMemberValueAnalysisVisitor::VisitEnumConstantDecl(
       EnumConstantDecl *ECD)
{
  if (ConsumerInstance->isInIncludedFile(ECD) || !ECD->getInitExpr())
    return true;

  ConsumerInstance->ValidInstanceNum++;
  if (ConsumerInstance->ValidInstanceNum ==
      ConsumerInstance->TransformationCounter) {
    ConsumerInstance->TheEnumConstantDecl = ECD;
  }
  return true;
}

void RemoveEnumMemberValue::Initialize(ASTContext &context)
{
  Transformation::Initialize(context);
  AnalysisVisitor = new RemoveEnumMemberValueAnalysisVisitor(this);
}

void RemoveEnumMemberValue::HandleTranslationUnit(ASTContext &Ctx)
{
  AnalysisVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  TransAssert(TheEnumConstantDecl && "NULL TheEnumConstantDecl!");

  removeEnumValue();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

static int getOffset(const char *Buf, char Symbol)
{
  int Offset = 0;
  while (*Buf != Symbol) {
    Buf--;
    if (*Buf == '\0')
      break;
    Offset--;
  }
  return Offset;
}

void RemoveEnumMemberValue::removeEnumValue()
{
  SourceManager &SrcManager = TheRewriter.getSourceMgr();
  SourceRange Range = TheEnumConstantDecl->getInitExpr()->getSourceRange();
  const char *Buf = SrcManager.getCharacterData(Range.getBegin());
  int offset = getOffset(Buf, '=');
  Range.setBegin(Range.getBegin().getLocWithOffset(offset));
  TheRewriter.RemoveText(Range);
}

RemoveEnumMemberValue::~RemoveEnumMemberValue()
{
  delete AnalysisVisitor;
}

