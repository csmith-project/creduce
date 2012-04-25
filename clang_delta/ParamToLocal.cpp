//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "ParamToLocal.h"

#include "clang/AST/RecursiveASTVisitor.h"
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
  RecursiveASTVisitor<ParamToLocalRewriteVisitor> {

public:
  explicit ParamToLocalRewriteVisitor(ParamToLocal *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitCallExpr(CallExpr *E);

  bool VisitCXXConstructorDecl(CXXConstructorDecl *CD);

  bool VisitFunctionDecl(FunctionDecl *FD);

  void rewriteAllExprs(void);

private:

  ParamToLocal *ConsumerInstance;

  SmallVector<CallExpr *, 10> AllCallExprs;

  SmallVector<const CXXConstructExpr *, 5> AllConstructExprs;

  bool rewriteFuncDecl(FunctionDecl *FP);

  bool rewriteParam(const ParmVarDecl *PV, 
                    unsigned int NumParams);

  bool rewriteOneCallExpr(CallExpr *CallE);

  bool rewriteOneConstructExpr(const CXXConstructExpr *CE);

  bool makeParamAsLocalVar(FunctionDecl *FP,
                           const ParmVarDecl *PV);

};

bool ParamToLocalASTVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  if (ConsumerInstance->isValidFuncDecl(FD->getCanonicalDecl())) {
    ConsumerInstance->ValidFuncDecls.push_back(FD->getCanonicalDecl());
  }
  return true;
}

bool ParamToLocalRewriteVisitor::rewriteParam(const ParmVarDecl *PV, 
                                              unsigned int NumParams)
{
  return 
    ConsumerInstance->RewriteHelper->removeParamFromFuncDecl(PV, 
                                          NumParams,
                                          ConsumerInstance->TheParamPos);
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
bool ParamToLocalRewriteVisitor::makeParamAsLocalVar(FunctionDecl *FD,
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
    ConsumerInstance->RewriteHelper->getExprString(DefaultArgE, ArgStr);
    LocalVarStr += " = ";
    LocalVarStr += ArgStr;
  }
  else if (T->isPointerType() || 
           T->isIntegralType(*(ConsumerInstance->Context))) {
    LocalVarStr += " = 0";
  }
  LocalVarStr += ";";

  return ConsumerInstance->RewriteHelper->addLocalVarToFunc(LocalVarStr, FD);
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
bool ParamToLocalRewriteVisitor::rewriteFuncDecl(FunctionDecl *FD) 
{
  const ParmVarDecl *PV = 
    FD->getParamDecl(ConsumerInstance->TheParamPos);  

  TransAssert(PV && "Unmatched ParamPos!");
  if (!rewriteParam(PV, FD->getNumParams()))
    return false;

  if (FD->isThisDeclarationADefinition()) {
    if (!makeParamAsLocalVar(FD, PV))
      return false;
  }
  return true;
}

bool ParamToLocalRewriteVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  FunctionDecl *CanonicalFD = FD->getCanonicalDecl();

  if (CanonicalFD == ConsumerInstance->TheFuncDecl)
    return rewriteFuncDecl(FD);

  return true;
}

bool ParamToLocalRewriteVisitor::rewriteOneCallExpr(CallExpr *CallE)
{
  return 
    ConsumerInstance->RewriteHelper->removeArgFromCallExpr(CallE, 
                                        ConsumerInstance->TheParamPos);
}

bool ParamToLocalRewriteVisitor::rewriteOneConstructExpr(
       const CXXConstructExpr *CE)
{
  return 
    ConsumerInstance->RewriteHelper->removeArgFromCXXConstructExpr(CE,
                                        ConsumerInstance->TheParamPos);
}

void ParamToLocalRewriteVisitor::rewriteAllExprs(void)
{
  while (!AllCallExprs.empty()) {
    CallExpr *CallE = AllCallExprs.pop_back_val();
    rewriteOneCallExpr(CallE);
  }

  while (!AllConstructExprs.empty()) {
    const CXXConstructExpr *CE = AllConstructExprs.pop_back_val();
    rewriteOneConstructExpr(CE);
  }
}

bool ParamToLocalRewriteVisitor::VisitCXXConstructorDecl(
       CXXConstructorDecl *CD)
{
  for (CXXConstructorDecl::init_iterator I = CD->init_begin(),
       E = CD->init_end(); I != E; ++I) {
    const Expr *InitE = (*I)->getInit();
    if (!InitE)
      continue;
    const CXXConstructExpr *CE = dyn_cast<CXXConstructExpr>(InitE);
    if (!CE)
      continue;

    const CXXConstructorDecl *CtorD = CE->getConstructor();
    if (CtorD->getCanonicalDecl() == ConsumerInstance->TheFuncDecl)
      AllConstructExprs.push_back(CE);
  }

  return true;
}

bool ParamToLocalRewriteVisitor::VisitCallExpr(CallExpr *CallE) 
{
  FunctionDecl *CalleeDecl = CallE->getDirectCallee();
  if (!CalleeDecl)
    return true;

  if (CalleeDecl->getCanonicalDecl() != ConsumerInstance->TheFuncDecl)
    return true;

  // We now have a correct CallExpr
  // Here we only collect these valid CallExprs, and 
  // will rewrite them later in a reverse order. 
  // The reason is that if we have code like below:
  //    foo(foo(1));
  // we want to rewrite the nested foo(1) first.
  // If we rewrite the outside foo first, we will
  // end up with bad transformation when we try to 
  // rewrite foo(1), which has been removed. 
  AllCallExprs.push_back(CallE);

  return true;
}

void ParamToLocal::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new ParamToLocalASTVisitor(this);
  RewriteVisitor = new ParamToLocalRewriteVisitor(this);
}

bool ParamToLocal::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    CollectionVisitor->TraverseDecl(*I);
  }
  return true;
}
 
void ParamToLocal::HandleTranslationUnit(ASTContext &Ctx)
{
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

bool ParamToLocal::isValidFuncDecl(FunctionDecl *FD) 
{
  bool IsValid = false;
  int ParamPos = 0;

  TransAssert(isa<FunctionDecl>(FD) && "Must be a FunctionDecl");

  // Skip the case like foo(int, ...), because we cannot remove
  // the "int" there
  if (FD->isVariadic() && (FD->getNumParams() == 1)) {
    return false;
  }

  // Avoid duplications
  if (std::find(ValidFuncDecls.begin(), 
                ValidFuncDecls.end(), FD) != 
      ValidFuncDecls.end())
    return false;

  for (FunctionDecl::param_const_iterator PI = FD->param_begin(),
       PE = FD->param_end(); PI != PE; ++PI) {
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
  if (CollectionVisitor)
    delete CollectionVisitor;

  if (RewriteVisitor)
    delete RewriteVisitor;
}

