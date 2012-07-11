//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "RemoveUnusedFunction.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Remove unused function declarations. \n";

static RegisterTransformation<RemoveUnusedFunction>
         Trans("remove-unused-function", DescriptionMsg);

class RUFAnalysisVisitor : public RecursiveASTVisitor<RUFAnalysisVisitor> {
public:

  explicit RUFAnalysisVisitor(RemoveUnusedFunction *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitFunctionDecl(FunctionDecl *FD);

private:

  RemoveUnusedFunction *ConsumerInstance;
};

bool RUFAnalysisVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  if (FD->isReferenced() || FD->isMain())
    return true;

  ConsumerInstance->ValidInstanceNum++;
  if (ConsumerInstance->ValidInstanceNum == 
      ConsumerInstance->TransformationCounter) {
    ConsumerInstance->TheFunctionDecl = FD;
  }
  return true;
}

void RemoveUnusedFunction::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  AnalysisVisitor = new RUFAnalysisVisitor(this);
}

void RemoveUnusedFunction::HandleTranslationUnit(ASTContext &Ctx)
{
  AnalysisVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  TransAssert(TheFunctionDecl && "NULL TheFunctionDecl!");

  removeFunctionDecl();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void RemoveUnusedFunction::removeFunctionDecl(void)
{
  SourceRange FuncRange = TheFunctionDecl->getSourceRange();
  TheRewriter.RemoveText(FuncRange);
}

RemoveUnusedFunction::~RemoveUnusedFunction(void)
{
  if (AnalysisVisitor)
    delete AnalysisVisitor;
}

