//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2014, 2015 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "ParamToGlobal.h"

#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Lift the declaraion of a parameter from a function \
to the global scope. Also rename the lifted parameter \
to avoid possible name conflicts. Note that for C++ code, \
this pass actually lifts a parameter from a member function \
to the class scope rather than the global scope.  \n";

static RegisterTransformation<ParamToGlobal>
         Trans("param-to-global", DescriptionMsg);

class ParamToGlobalASTVisitor : public 
  RecursiveASTVisitor<ParamToGlobalASTVisitor> {

public:
  explicit ParamToGlobalASTVisitor(ParamToGlobal *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitFunctionDecl(FunctionDecl *FD);

private:
  ParamToGlobal *ConsumerInstance;

};

class ParamToGlobalRewriteVisitor : public 
  CommonParameterRewriteVisitor<ParamToGlobalRewriteVisitor, ParamToGlobal> {

public:
  explicit ParamToGlobalRewriteVisitor(ParamToGlobal *Instance)
    : CommonParameterRewriteVisitor<ParamToGlobalRewriteVisitor,
                                    ParamToGlobal>(Instance)
  { }

  bool VisitDeclRefExpr(DeclRefExpr *ParmRefExpr);
};

bool ParamToGlobalASTVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  if (ConsumerInstance->isValidFuncDecl(FD->getCanonicalDecl())) {
    ConsumerInstance->ValidFuncDecls.push_back(FD->getCanonicalDecl());
  }
  return true;
}

std::string ParamToGlobal::getNewName(FunctionDecl *FP, const ParmVarDecl *PV)
{
  std::string NewName;
  NewName = FP->getNameInfo().getAsString();
  NewName += "_";
  NewName += PV->getNameAsString();

  // also backup the new name
  TheNewDeclName = NewName;
  return NewName;
}

bool ParamToGlobalRewriteVisitor::VisitDeclRefExpr(DeclRefExpr *ParmRefExpr)
{
  const ValueDecl *OrigDecl = ParmRefExpr->getDecl();

  if (!ConsumerInstance->TheParmVarDecl)
    return true;

  if (OrigDecl != ConsumerInstance->TheParmVarDecl)
    return true;

  SourceRange ExprRange = ParmRefExpr->getSourceRange();
  return 
    !(ConsumerInstance->TheRewriter.ReplaceText(ExprRange, 
                                      ConsumerInstance->TheNewDeclName));
}

void ParamToGlobal::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new ParamToGlobalASTVisitor(this);
  RewriteVisitor = new ParamToGlobalRewriteVisitor(this);
}

void ParamToGlobal::HandleTranslationUnit(ASTContext &Ctx)
{
  if (TransformationManager::isOpenCLLangOpt()) {
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

  TransAssert(RewriteVisitor && "NULL RewriteVisitor!");
  TransAssert(TheFuncDecl && "NULL TheFuncDecl!");
  TransAssert((TheParamPos >= 0) && "Invalid parameter position!");
  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  RewriteVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
  RewriteVisitor->rewriteAllExprs();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

bool ParamToGlobal::transformParamVar(FunctionDecl *FD, const ParmVarDecl *PV)
{
  std::string PName = PV->getNameAsString();
  // Safe to omit an un-named parameter
  if (PName.empty())
    return true;

  std::string GlobalVarStr;

  GlobalVarStr = PV->getType().getAsString();
  GlobalVarStr += " ";
  GlobalVarStr += getNewName(FD, PV);
  GlobalVarStr += ";\n";

  return RewriteHelper->insertStringBeforeFunc(FD, GlobalVarStr);
}

bool ParamToGlobal::rewriteFuncDecl(clang::FunctionDecl *FD)
{
  const clang::ParmVarDecl *PV = FD->getParamDecl(TheParamPos);

  TransAssert(PV && "Unmatched ParamPos!");
  RewriteHelper->removeParamFromFuncDecl(PV, 
                                         FD->getNumParams(),
                                         TheParamPos);

  if (FD->isThisDeclarationADefinition()) {
    TheParmVarDecl = PV;
    if (!transformParamVar(FD, PV))
      return false;
  }
  return true;
}

bool ParamToGlobal::isValidFuncDecl(FunctionDecl *FD) 
{
  bool IsValid = false;
  int ParamPos = 0;

  TransAssert(isa<FunctionDecl>(FD) && "Must be a FunctionDecl");

  if (isInIncludedFile(FD))
    return false;

  // Skip the case like foo(int, ...), because we cannot remove
  // the "int" there
  if (FD->isVariadic() && (FD->getNumParams() == 1)) {
    return false;
  }

  if (FD->isOverloadedOperator())
    return false;

  // Avoid duplications
  if (std::find(ValidFuncDecls.begin(), 
                ValidFuncDecls.end(), FD) != 
      ValidFuncDecls.end())
    return false;

  for (FunctionDecl::param_const_iterator PI = FD->param_begin(),
       PE = FD->param_end(); PI != PE; ++PI) {
    if ((*PI)->isImplicit() || (*PI)->getSourceRange().isInvalid())
      continue;
    if (!FD->hasBody() && (*PI)->getNameAsString().empty())
      continue;

    ValidInstanceNum++;

    if (ValidInstanceNum == TransformationCounter) {
      TheFuncDecl = FD;
      TheParamPos = ParamPos;
    }

    IsValid = true;
    ParamPos++;
  }
  return IsValid;
}

ParamToGlobal::~ParamToGlobal(void)
{
  delete CollectionVisitor;
  delete RewriteVisitor;
}

