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

#include "ParamToLocal.h"

#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Remove an integaral/enumeration parameter from the declaration \
of a function. Define the removed parameter as a local variable \
of the same function. Initialze the newly local variable to be \
0. Also, make corresponding changes on all of the call sites of \
the modified function.\n";
 
static RegisterTransformation<ParamToLocal> 
         Trans("param-to-local", DescriptionMsg);

class ParamToLocalASTVisitor : public 
  RecursiveASTVisitor<ParamToLocalASTVisitor> {

public:
  explicit ParamToLocalASTVisitor(ParamToLocal *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitFunctionDecl(FunctionDecl *FD);

private:
  ParamToLocal *ConsumerInstance;
};

class ParamToLocalRewriteVisitor : public 
  CommonParameterRewriteVisitor<ParamToLocalRewriteVisitor, ParamToLocal> {

public:
  explicit ParamToLocalRewriteVisitor(ParamToLocal *Instance)
    : CommonParameterRewriteVisitor<ParamToLocalRewriteVisitor, 
                                    ParamToLocal>(Instance)
  { }
};

bool ParamToLocalASTVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  if (ConsumerInstance->isValidFuncDecl(FD->getCanonicalDecl())) {
    ConsumerInstance->ValidFuncDecls.push_back(FD->getCanonicalDecl());
  }
  return true;
}

void ParamToLocal::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new ParamToLocalASTVisitor(this);
  RewriteVisitor = new ParamToLocalRewriteVisitor(this);
}

void ParamToLocal::HandleTranslationUnit(ASTContext &Ctx)
{
  CollectionVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  TransAssert(RewriteVisitor && "NULL RewriteVisitor!");
  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);
  TransAssert(TheFuncDecl && "NULL TheFuncDecl!");
  TransAssert((TheParamPos >= 0) && "Invalid parameter position!");

  RewriteVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
  RewriteVisitor->rewriteAllExprs();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

// For CXX, this function could generate bad code, e.g.,
// class A {
//   void foo(void) {...}
//   void foo(int x) {...}
// };
//
// will be transformed to 
// class A {
//   void foo(void) {...}
//   void foo(void) {int x ...}
// };
bool ParamToLocal::rewriteFuncDecl(FunctionDecl *FD) 
{
  const ParmVarDecl *PV = FD->getParamDecl(TheParamPos);  

  TransAssert(PV && "Unmatched ParamPos!");
  RewriteHelper->removeParamFromFuncDecl(PV, 
                                         FD->getNumParams(),
                                         TheParamPos);

  if (FD->isThisDeclarationADefinition()) {
    if (!transformParamVar(FD, PV))
      return false;
  }
  return true;
}

// ISSUE: we could have another type of bad transformation, e.g.,
// class A { ... };
// class B : public A {
//   B(int x) : A(x) {...}
// };
// ==>
// class A { ... };
// class B : public A {
//   B(void) : A(x) {int x ...}
// };
// hence x is undeclared for A(x)
bool ParamToLocal::transformParamVar(FunctionDecl *FD,
                                     const ParmVarDecl *PV)
{
  std::string PName = PV->getNameAsString();
  // Safe to omit an un-named parameter
  if (PName.empty())
    return true;

  std::string LocalVarStr;

  LocalVarStr += PV->getType().getAsString();
  LocalVarStr += " ";
  LocalVarStr += PV->getNameAsString();

  QualType PVType = PV->getOriginalType();
  const Type *T = PVType.getTypePtr();
  if ( const Expr *DefaultArgE = PV->getDefaultArg() ) {
    std::string ArgStr;
    RewriteHelper->getExprString(DefaultArgE, ArgStr);
    LocalVarStr += " = ";
    LocalVarStr += ArgStr;
  }
  else if (T->isPointerType() || T->isIntegralType(*Context)) {
    LocalVarStr += " = 0";
  }
  LocalVarStr += ";";

  return RewriteHelper->addLocalVarToFunc(LocalVarStr, FD);
}

bool ParamToLocal::isValidFuncDecl(FunctionDecl *FD) 
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

ParamToLocal::~ParamToLocal(void)
{
  delete CollectionVisitor;
  delete RewriteVisitor;
}

