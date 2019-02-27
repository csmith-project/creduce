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

#include "SimplifyCallExpr.h"

#include <sstream>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Simplify a call expression to a comma expression. \
Replace function arguments with: \n\
  * 0 for integer and pointer arguments \n\
  * a global temp variable for structs/unions \
and also append a representative return value which is the \
the last inner expression of the comma expression. \n\
For example, assume we have a function foo: \n\
  int foo(int x, int *y, struct z) \
Then this transformation will transform \n\
  foo(i, p, s); \n\
to \n\
  (0, 0, tmp_var, 0);\n";

static RegisterTransformation<SimplifyCallExpr>
         Trans("simplify-callexpr", DescriptionMsg);

class SimplifyCallExprVisitor : public 
  RecursiveASTVisitor<SimplifyCallExprVisitor> {

public:

  explicit SimplifyCallExprVisitor(SimplifyCallExpr *Instance)
    : ConsumerInstance(Instance),
      CurrentFD(NULL)
  { }

  bool VisitCallExpr(CallExpr *CE);

  bool VisitFunctionDecl(FunctionDecl *FD);

private:

  SimplifyCallExpr *ConsumerInstance;

  const FunctionDecl *CurrentFD;
};

bool SimplifyCallExprVisitor::VisitCallExpr(CallExpr *CE)
{
  if (ConsumerInstance->isInIncludedFile(CE))
    return true;

  ConsumerInstance->ValidInstanceNum++;
  if (ConsumerInstance->TransformationCounter != 
      ConsumerInstance->ValidInstanceNum)
    return true;
  
  ConsumerInstance->TheCallExpr = CE;
  ConsumerInstance->CurrentFD = CurrentFD;
  return true;
}

bool SimplifyCallExprVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  CurrentFD = FD;
  return true;
}

void SimplifyCallExpr::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new SimplifyCallExprVisitor(this);
  NameQueryWrap = 
    new TransNameQueryWrap(RewriteHelper->getTmpVarNamePrefix());
}

void SimplifyCallExpr::HandleTranslationUnit(ASTContext &Ctx)
{
  CollectionVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  TransAssert(TheCallExpr && "NULL TheCallExpr!");
  TransAssert(CurrentFD && "NULL CurrentFD");

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  NameQueryWrap->TraverseDecl(Ctx.getTranslationUnitDecl());
  NamePostfix = NameQueryWrap->getMaxNamePostfix() + 1;

  replaceCallExpr();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void SimplifyCallExpr::handleOneArgStr(const Expr *Arg, std::string &Str)
{
  Str = "0";
  const Type *ArgT = Arg->getType().getTypePtr();
  if (!ArgT->isUnionType() && !ArgT->isStructureType())
    return;

  RewriteHelper->getTmpTransName(NamePostfix, Str);
  NamePostfix++;

  std::string TmpVarStr = Str;
  Arg->getType().getAsStringInternal(TmpVarStr, Context->getPrintingPolicy());
  TmpVarStr += ";\n";
  RewriteHelper->insertStringBeforeFunc(CurrentFD, TmpVarStr);
}

void SimplifyCallExpr::replaceCallExpr(void)
{
  std::string CommaStr("");
  unsigned int NumArg = TheCallExpr->getNumArgs();
  if (NumArg == 0) {
    RewriteHelper->replaceExpr(TheCallExpr, CommaStr);
    return;
  }

  const Expr *Arg = TheCallExpr->getArg(0);
  std::string ArgStr;
  handleOneArgStr(Arg, ArgStr);
  CommaStr += ("(" + ArgStr);

  for (unsigned int I = 1; I < NumArg; ++I) {
    Arg = TheCallExpr->getArg(I);
    handleOneArgStr(Arg, ArgStr);
    CommaStr += ("," + ArgStr);
  }

  QualType RVQualType = TheCallExpr->getType();
  const Type *RVType = RVQualType.getTypePtr();
  if (RVType->isVoidType()) {
    // Nothing to do
  }
  else if (RVType->isUnionType() || RVType->isStructureType()) {
    std::string RVStr("");
    RewriteHelper->getTmpTransName(NamePostfix, RVStr);
    NamePostfix++;

    CommaStr += ("," + RVStr);
    RVQualType.getAsStringInternal(RVStr, Context->getPrintingPolicy());
    RVStr += ";\n";
    RewriteHelper->insertStringBeforeFunc(CurrentFD, RVStr);
  }
  else {
    CommaStr += ",0";
  }

  CommaStr += ")";
  RewriteHelper->replaceExpr(TheCallExpr, CommaStr);
}

SimplifyCallExpr::~SimplifyCallExpr(void)
{
  delete CollectionVisitor;
}

