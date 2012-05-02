//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ParamToGlobal.h"

#include "clang/AST/RecursiveASTVisitor.h"
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
  RecursiveASTVisitor<ParamToGlobalRewriteVisitor> {

public:

  explicit ParamToGlobalRewriteVisitor(ParamToGlobal *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitCallExpr(CallExpr *E);

  bool VisitFunctionDecl(FunctionDecl *FD);

  bool VisitCXXConstructorDecl(CXXConstructorDecl *CD);

  bool VisitDeclRefExpr(DeclRefExpr *ParmRefExpr);

  void rewriteAllExprs(void);

private:

  ParamToGlobal *ConsumerInstance;

  SmallVector<CallExpr *, 10> AllCallExprs;

  SmallVector<const CXXConstructExpr *, 5> AllConstructExprs;

  bool rewriteFuncDecl(FunctionDecl *FP);

  bool rewriteParam(const ParmVarDecl *PV, 
                    unsigned int NumParams);

  bool rewriteOneCallExpr(CallExpr *CallE);

  bool rewriteOneConstructExpr(const CXXConstructExpr *CE);

  bool makeParamAsGlobalVar(FunctionDecl *FD,
                            const ParmVarDecl *PV);

  std::string getNewName(FunctionDecl *FP,
                         const ParmVarDecl *PV);
};

bool ParamToGlobalASTVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  if (ConsumerInstance->isValidFuncDecl(FD->getCanonicalDecl())) {
    ConsumerInstance->ValidFuncDecls.push_back(FD->getCanonicalDecl());
  }
  return true;
}

bool ParamToGlobalRewriteVisitor::rewriteParam(const ParmVarDecl *PV, 
                                               unsigned int NumParams)
{
  return 
    ConsumerInstance->RewriteHelper->removeParamFromFuncDecl(PV, 
                                          NumParams,
                                          ConsumerInstance->TheParamPos);
}

std::string ParamToGlobalRewriteVisitor::getNewName(FunctionDecl *FP,
                                                    const ParmVarDecl *PV)
{
  std::string NewName;
  NewName = FP->getNameInfo().getAsString();
  NewName += "_";
  NewName += PV->getNameAsString();

  // also backup the new name
  ConsumerInstance->TheNewDeclName = NewName;
  return NewName;
}

bool ParamToGlobalRewriteVisitor::makeParamAsGlobalVar(FunctionDecl *FD,
                                                       const ParmVarDecl *PV)
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

  return ConsumerInstance->RewriteHelper->insertStringBeforeFunc(FD, 
                                                       GlobalVarStr);
}

bool ParamToGlobalRewriteVisitor::rewriteFuncDecl(FunctionDecl *FD) 
{
  const ParmVarDecl *PV = 
    FD->getParamDecl(ConsumerInstance->TheParamPos);  

  TransAssert(PV && "Unmatched ParamPos!");
  if (!rewriteParam(PV, FD->getNumParams()))
    return false;

  if (FD->isThisDeclarationADefinition()) {
    ConsumerInstance->TheParmVarDecl = PV;
    if (!makeParamAsGlobalVar(FD, PV))
      return false;
  }
  return true;
}

bool ParamToGlobalRewriteVisitor::VisitCXXConstructorDecl(
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

bool ParamToGlobalRewriteVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  FunctionDecl *CanonicalFD = FD->getCanonicalDecl();

  if (CanonicalFD == ConsumerInstance->TheFuncDecl)
    return rewriteFuncDecl(FD);

  return true;
}

bool ParamToGlobalRewriteVisitor::rewriteOneCallExpr(CallExpr *CallE)
{
  return 
    ConsumerInstance->RewriteHelper->removeArgFromCallExpr(CallE, 
                                        ConsumerInstance->TheParamPos);
}

bool ParamToGlobalRewriteVisitor::rewriteOneConstructExpr(
       const CXXConstructExpr *CE)
{
  return 
    ConsumerInstance->RewriteHelper->removeArgFromCXXConstructExpr(CE,
                                        ConsumerInstance->TheParamPos);
}

void ParamToGlobalRewriteVisitor::rewriteAllExprs(void)
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

bool ParamToGlobalRewriteVisitor::VisitCallExpr(CallExpr *CallE) 
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

bool ParamToGlobal::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    CollectionVisitor->TraverseDecl(*I);
  }
  return true;
}
 
void ParamToGlobal::HandleTranslationUnit(ASTContext &Ctx)
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

bool ParamToGlobal::isValidFuncDecl(FunctionDecl *FD) 
{
  bool IsValid = false;
  int ParamPos = 0;

  TransAssert(isa<FunctionDecl>(FD) && "Must be a FunctionDecl");

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
  if (CollectionVisitor)
    delete CollectionVisitor;

  if (RewriteVisitor)
    delete RewriteVisitor;
}

