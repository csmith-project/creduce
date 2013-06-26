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

#include "TemplateNonTypeArgToInt.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"
#include "CommonTemplateArgumentVisitor.h"

using namespace clang;
using namespace llvm;
using namespace clang_delta_common_visitor;

static const char *DescriptionMsg = 
"This pass tries to replace a template non-type argument \
wth an integer (if its type is compatible) \n";

static RegisterTransformation<TemplateNonTypeArgToInt>
         Trans("template-non-type-arg-to-int", DescriptionMsg);

class TemplateNonTypeArgToIntASTVisitor : public 
  CommonTemplateArgumentVisitor<TemplateNonTypeArgToIntASTVisitor,
                                TemplateNonTypeArgToInt> {

public:
  explicit TemplateNonTypeArgToIntASTVisitor(
             TemplateNonTypeArgToInt *Instance)
    : CommonTemplateArgumentVisitor<TemplateNonTypeArgToIntASTVisitor,
                                    TemplateNonTypeArgToInt>(Instance)
  { }
};

void TemplateNonTypeArgToInt::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new TemplateNonTypeArgToIntASTVisitor(this);
}

void TemplateNonTypeArgToInt::HandleTranslationUnit(ASTContext &Ctx)
{
  if (TransformationManager::isCLangOpt()) {
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

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);
  TransAssert(TheExpr && "NULL TheExpr");
  RewriteHelper->replaceExpr(TheExpr, IntString);

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

bool TemplateNonTypeArgToInt::isValidTemplateArgument(
       const TemplateArgument &Arg)
{
  TemplateArgument::ArgKind K = Arg.getKind();
  switch (K) {
  case TemplateArgument::Declaration: {
    const ValueDecl *VD = Arg.getAsDecl();
    return VD->getType().getTypePtr()->isIntegerType();
  }

  case TemplateArgument::Expression: {
    const Expr *E = Arg.getAsExpr();
    return E->getType().getTypePtr()->isIntegerType();
  }

  default:
    return false;
  }
  TransAssert(0 && "Unreachable code!");
  return false;
}

void TemplateNonTypeArgToInt::handleOneTemplateArgumentLoc(
       const TemplateArgumentLoc &ArgLoc)
{
  if (ArgLoc.getLocation().isInvalid())
    return;
  const TemplateArgument &Arg = ArgLoc.getArgument();

  if (!isValidTemplateArgument(Arg))
    return;

  ValidInstanceNum++;
  if (ValidInstanceNum == TransformationCounter) {
    TheExpr = ArgLoc.getLocInfo().getAsExpr();
    llvm::APSInt Result;
    if (TheExpr->EvaluateAsInt(Result, *Context)) {
      IntString = Result.toString(10);
    }
  }
}

void TemplateNonTypeArgToInt::handleTemplateArgumentLocs(
       const TemplateDecl *D, const TemplateArgumentLoc *TAL, unsigned NumArgs)
{
  TransAssert(D && "NULL TemplateDecl!");
  if (!TAL)
    return;
  for (unsigned I = 0; I < NumArgs; ++I) {
    handleOneTemplateArgumentLoc(TAL[I]);
  }
}

void TemplateNonTypeArgToInt::handleTemplateSpecializationTypeLoc(
       const TemplateSpecializationTypeLoc &TLoc)
{
  for (unsigned I = 0; I < TLoc.getNumArgs(); ++I) {
    handleOneTemplateArgumentLoc(TLoc.getArgLoc(I));
  }
}

TemplateNonTypeArgToInt::~TemplateNonTypeArgToInt()
{
  delete CollectionVisitor;
}

