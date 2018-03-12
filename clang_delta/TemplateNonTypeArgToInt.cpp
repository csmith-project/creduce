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

#include "TemplateNonTypeArgToInt.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"
#include "CommonTemplateArgumentVisitor.h"

using namespace clang;
using namespace clang_delta_common_visitor;

static const char *DescriptionMsg = 
"This pass tries to replace a template non-type argument \
wth an integer (if its type is compatible) \n";

static RegisterTransformation<TemplateNonTypeArgToInt>
         Trans("template-non-type-arg-to-int", DescriptionMsg);

class TemplateNonTypeArgToIntArgCollector : public 
  RecursiveASTVisitor<TemplateNonTypeArgToIntArgCollector> {

public:
  explicit TemplateNonTypeArgToIntArgCollector(
             TemplateNonTypeArgToInt *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitClassTemplateDecl(ClassTemplateDecl *D);

  bool VisitFunctionTemplateDecl(FunctionTemplateDecl *D);

private:
  TemplateNonTypeArgToInt *ConsumerInstance;

};

bool TemplateNonTypeArgToIntArgCollector::VisitClassTemplateDecl(
       ClassTemplateDecl *D)
{
  if (D->isThisDeclarationADefinition())
    ConsumerInstance->handleOneTemplateDecl(D);
  return true;
}

bool TemplateNonTypeArgToIntArgCollector::VisitFunctionTemplateDecl(
       FunctionTemplateDecl *D)
{
  if (D->isThisDeclarationADefinition())
    ConsumerInstance->handleOneTemplateDecl(D);
  return true;
}

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
  ArgCollector = new TemplateNonTypeArgToIntArgCollector(this);
}

void TemplateNonTypeArgToInt::HandleTranslationUnit(ASTContext &Ctx)
{
  if (TransformationManager::isCLangOpt() ||
      TransformationManager::isOpenCLLangOpt()) {
    ValidInstanceNum = 0;
  }
  else {
    ArgCollector->TraverseDecl(Ctx.getTranslationUnitDecl());
    CollectionVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
  }

  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);
  if (TheExpr) {
    RewriteHelper->replaceExpr(TheExpr, IntString);
  }
  else if (TheValueDecl) {
    RewriteHelper->replaceValueDecl(TheValueDecl,
                                    "int " + TheValueDecl->getNameAsString());
  }
  else {
    TransAssert(0 && "No valid targets!");
  }

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
    return true;
  }

  case TemplateArgument::Expression: {
    const Expr *E = Arg.getAsExpr()->IgnoreParenCasts();
    if (dyn_cast<IntegerLiteral>(E) || dyn_cast<CXXBoolLiteralExpr>(E))
      return false;
    if (const UnaryOperator *UO = dyn_cast<UnaryOperator>(E)) {
      UnaryOperator::Opcode Op = UO->getOpcode();
      if((Op == UO_Minus) || (Op == UO_Plus))
        return false;
    }

    return true;
  }

  default:
    TransAssert(0 && "Unreachable code!");
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
    if (!TheExpr->isValueDependent() &&
        TheExpr->EvaluateAsInt(Result, *Context)) {
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
  TemplateParameterIdxSet *ValidIdx = 
    DeclToParamIdx[dyn_cast<TemplateDecl>(D->getCanonicalDecl())];
  if (!ValidIdx)
    return;
  for (unsigned I = 0; I < NumArgs; ++I) {
    if (ValidIdx->count(I))
      handleOneTemplateArgumentLoc(TAL[I]);
  }
}

void TemplateNonTypeArgToInt::handleTemplateSpecializationTypeLoc(
       const TemplateSpecializationTypeLoc &TLoc)
{
  const Type *Ty = TLoc.getTypePtr();
  const TemplateSpecializationType *TST = 
    Ty->getAs<TemplateSpecializationType>();
  TemplateName TplName = TST->getTemplateName();
  const TemplateDecl *TplD = TplName.getAsTemplateDecl();

  TemplateParameterIdxSet *ValidIdx = 
    DeclToParamIdx[dyn_cast<TemplateDecl>(TplD->getCanonicalDecl())];
  if (!ValidIdx)
    return;
  for (unsigned I = 0; I < TLoc.getNumArgs(); ++I) {
    if (ValidIdx->count(I))
      handleOneTemplateArgumentLoc(TLoc.getArgLoc(I));
  }
}

bool TemplateNonTypeArgToInt::isValidParameter(const NamedDecl *ND)
{
  const NonTypeTemplateParmDecl *NonTypeD = 
          dyn_cast<NonTypeTemplateParmDecl>(ND);
  if (!NonTypeD)
    return false;
  // To avoid something like replacing int with int.
  if (NonTypeD->getType().getAsString() == "int")
    return false;
  const Type *Ty = NonTypeD->getType().getTypePtr();
  return Ty->isIntegerType();
}
      
void TemplateNonTypeArgToInt::handleOneTemplateDecl(const TemplateDecl *D)
{
  if (isInIncludedFile(D))
    return;
  TemplateParameterIdxSet *ValidParamIdx = new TemplateParameterIdxSet();
  TemplateParameterList *TPList = D->getTemplateParameters();
  unsigned Idx = 0;
  for (TemplateParameterList::const_iterator I = TPList->begin(),
       E = TPList->end(); I != E; ++I) {
    const NamedDecl *ParamND = (*I);
    if (isValidParameter(ParamND)) {
      ValidParamIdx->insert(Idx);
      if (const ValueDecl* ValD = dyn_cast<ValueDecl>(ParamND)) {
        ++ValidInstanceNum;
        if (ValidInstanceNum == TransformationCounter)
          TheValueDecl = ValD;
      }
    }
    Idx++;
  }

  TransAssert(!DeclToParamIdx[D] && "Duplicate TemplateDecl!");
  DeclToParamIdx[dyn_cast<TemplateDecl>(D->getCanonicalDecl())] = ValidParamIdx;
}

TemplateNonTypeArgToInt::~TemplateNonTypeArgToInt()
{
  delete CollectionVisitor;
  delete ArgCollector;
}

