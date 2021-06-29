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

#include "RemoveTryCatch.h"

#include <cctype>
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Remove catch blocks and if not present the try block as well. \n";

static RegisterTransformation<RemoveTryCatch>
         Trans("remove-try-catch", DescriptionMsg);

class RemoveTryCatchAnalysisVisitor : public
  RecursiveASTVisitor<RemoveTryCatchAnalysisVisitor> {
public:

  explicit RemoveTryCatchAnalysisVisitor(RemoveTryCatch *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitCXXTryStmt(CXXTryStmt *CTS);

private:

  RemoveTryCatch *ConsumerInstance;
};

bool RemoveTryCatchAnalysisVisitor::VisitCXXTryStmt(
       CXXTryStmt *CTS)
{
  if (ConsumerInstance->isInIncludedFile(CTS)) {
    return true;
  }

  // Count try block
  ++ConsumerInstance->ValidInstanceNum;

  if (ConsumerInstance->TransformationCounter ==
      ConsumerInstance->ValidInstanceNum) {
    ConsumerInstance->TheTryCatchStmt = CTS;
  }

  int TmpInstanceNum = ConsumerInstance->ValidInstanceNum;

  // Count all catch blocks
  ConsumerInstance->ValidInstanceNum += CTS->getNumHandlers();

  // Early exit if the transformation counter is less than the index of any
  // catch block
  if (ConsumerInstance->TransformationCounter <= TmpInstanceNum) {
    return true;
  }

  // Early exit if the transformation counter is higher than the index of any
  // catch block
  if (ConsumerInstance->TransformationCounter >
      ConsumerInstance->ValidInstanceNum) {
    return true;
  }

  TransAssert(ConsumerInstance->TransformationCounter > TmpInstanceNum);

  int CatchIdx = ConsumerInstance->TransformationCounter - TmpInstanceNum - 1;
  ConsumerInstance->TheTryCatchStmt = CTS->getHandler(CatchIdx);

  // If the last catch block is removed the "try" has to be removed as well
  if (CTS->getNumHandlers() == 1) {
    ConsumerInstance->RewriteTryStmt = CTS;
  }

  return true;
}

void RemoveTryCatch::Initialize(ASTContext &context)
{
  Transformation::Initialize(context);
  AnalysisVisitor = new RemoveTryCatchAnalysisVisitor(this);
}

void RemoveTryCatch::HandleTranslationUnit(ASTContext &Ctx)
{
  AnalysisVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  TransAssert(TheTryCatchStmt && "NULL TheTryCatchStmt!");

  removeStmt();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void RemoveTryCatch::removeStmt()
{
  SourceRange Range = TheTryCatchStmt->getSourceRange();
  TheRewriter.RemoveText(Range);

  if (RewriteTryStmt != nullptr) {
    TheRewriter.RemoveText(RewriteTryStmt->getLocStart(), 3);
  }
}

RemoveTryCatch::~RemoveTryCatch()
{
  delete AnalysisVisitor;
}
