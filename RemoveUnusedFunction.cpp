//===----------------------------------------------------------------------===//
// 
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "RemoveUnusedFunction.h"

#include <sstream>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"

#include "RewriteUtils.h"
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
  Context = &context;
  SrcManager = &Context->getSourceManager();
  AnalysisVisitor = new RUFAnalysisVisitor(this);
  TheRewriter.setSourceMgr(Context->getSourceManager(), 
                           Context->getLangOptions());
}

void RemoveUnusedFunction::HandleTopLevelDecl(DeclGroupRef D) 
{
  // Nothing to do 
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

