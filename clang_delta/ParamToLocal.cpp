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

class PToLASTVisitor : public RecursiveASTVisitor<PToLASTVisitor> {
public:
  typedef RecursiveASTVisitor<PToLASTVisitor> Inherited;

  explicit PToLASTVisitor(ParamToLocal *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitCallExpr(CallExpr *E);

  bool VisitFunctionDecl(FunctionDecl *FD);

  void rewriteAllCallExprs(void);

private:

  ParamToLocal *ConsumerInstance;

  SmallVector<CallExpr *, 10> AllCallExprs;

  bool rewriteFuncDecl(FunctionDecl *FP);

  bool rewriteParam(const ParmVarDecl *PV, 
                    unsigned int NumParams);

  bool rewriteOneCallExpr(CallExpr *CallE);

  bool makeParamAsLocalVar(FunctionDecl *FP,
                           const ParmVarDecl *PV);

};

void ParamToLocal::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  TransformationASTVisitor = new PToLASTVisitor(this);
}

bool ParamToLocal::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    FunctionDecl *FD = dyn_cast<FunctionDecl>(*I);
    if (FD && isValidFuncDecl(FD->getCanonicalDecl())) {
      ValidFuncDecls.push_back(FD->getCanonicalDecl());
    }
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

  TransAssert(TransformationASTVisitor && "NULL TransformationASTVisitor!");
  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);
  TransAssert(TheFuncDecl && "NULL TheFuncDecl!");
  TransAssert((TheParamPos >= 0) && "Invalid parameter position!");

  TransformationASTVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
  TransformationASTVisitor->rewriteAllCallExprs();

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
  if (TransformationASTVisitor)
    delete TransformationASTVisitor;
}

bool PToLASTVisitor::rewriteParam(const ParmVarDecl *PV, 
                                 unsigned int NumParams)
{
  return 
    ConsumerInstance->RewriteHelper->removeParamFromFuncDecl(PV, 
                                          NumParams,
                                          ConsumerInstance->TheParamPos);
}

bool PToLASTVisitor::makeParamAsLocalVar(FunctionDecl *FD,
                                        const ParmVarDecl *PV)
{
  std::string LocalVarStr;

  LocalVarStr += PV->getType().getAsString();
  LocalVarStr += " ";
  LocalVarStr += PV->getNameAsString();

  QualType PVType = PV->getOriginalType();
  const Type *T = PVType.getTypePtr();
  if (T->isIntegralOrEnumerationType() || T->isPointerType()) {
    LocalVarStr += " = 0";
  }
  LocalVarStr += ";";

  return ConsumerInstance->RewriteHelper->addLocalVarToFunc(LocalVarStr, FD);
}

bool PToLASTVisitor::rewriteFuncDecl(FunctionDecl *FD) 
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

bool PToLASTVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  FunctionDecl *CanonicalFD = FD->getCanonicalDecl();

  if (CanonicalFD == ConsumerInstance->TheFuncDecl)
    return rewriteFuncDecl(FD);

  return true;
}

bool PToLASTVisitor::rewriteOneCallExpr(CallExpr *CallE)
{
  return 
    ConsumerInstance->RewriteHelper->removeArgFromCallExpr(CallE, 
                                        ConsumerInstance->TheParamPos);
}

void PToLASTVisitor::rewriteAllCallExprs(void)
{
  while (!AllCallExprs.empty()) {
    CallExpr *CallE = AllCallExprs.pop_back_val();
    rewriteOneCallExpr(CallE);
  }
}

bool PToLASTVisitor::VisitCallExpr(CallExpr *CallE) 
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

