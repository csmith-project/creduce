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
to avoid possible name conflicts. Currently only support \
lifting integaral/enumeration parameters. \n";

static RegisterTransformation<ParamToGlobal>
         Trans("param-to-global", DescriptionMsg);

class PToGASTVisitor : public RecursiveASTVisitor<PToGASTVisitor> {
public:
  typedef RecursiveASTVisitor<PToGASTVisitor> Inherited;

  explicit PToGASTVisitor(ParamToGlobal *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitCallExpr(CallExpr *E);

  bool VisitFunctionDecl(FunctionDecl *FD);

  bool VisitDeclRefExpr(DeclRefExpr *ParmRefExpr);

  void rewriteAllCallExprs(void);

private:

  ParamToGlobal *ConsumerInstance;

  SmallVector<CallExpr *, 10> AllCallExprs;

  bool rewriteFuncDecl(FunctionDecl *FP);

  bool rewriteParam(const ParmVarDecl *PV, 
                    unsigned int NumParams);

  bool rewriteOneCallExpr(CallExpr *CallE);

  bool makeParamAsGlobalVar(FunctionDecl *FD,
                            const ParmVarDecl *PV);

  std::string getNewName(FunctionDecl *FP,
                         const ParmVarDecl *PV);
};

void ParamToGlobal::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  TransformationASTVisitor = new PToGASTVisitor(this);
}

void ParamToGlobal::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    FunctionDecl *FD = dyn_cast<FunctionDecl>(*I);
    if (FD && isValidFuncDecl(FD->getCanonicalDecl())) {
      ValidFuncDecls.push_back(FD->getCanonicalDecl());
    }
  }
}
 
void ParamToGlobal::HandleTranslationUnit(ASTContext &Ctx)
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
  if (TransformationASTVisitor)
    delete TransformationASTVisitor;
}

bool PToGASTVisitor::rewriteParam(const ParmVarDecl *PV, 
                                 unsigned int NumParams)
{
  return 
    ConsumerInstance->RewriteHelper->removeParamFromFuncDecl(PV, 
                                          NumParams,
                                          ConsumerInstance->TheParamPos);
}

std::string PToGASTVisitor::getNewName(FunctionDecl *FP,
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

bool PToGASTVisitor::makeParamAsGlobalVar(FunctionDecl *FD,
                                          const ParmVarDecl *PV)
{
  std::string GlobalVarStr;

  GlobalVarStr = PV->getType().getAsString();
  GlobalVarStr += " ";
  GlobalVarStr += getNewName(FD, PV);
  GlobalVarStr += ";\n";

  return ConsumerInstance->RewriteHelper->insertStringBeforeFunc(FD, 
                                                       GlobalVarStr);
}

bool PToGASTVisitor::rewriteFuncDecl(FunctionDecl *FD) 
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

bool PToGASTVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  FunctionDecl *CanonicalFD = FD->getCanonicalDecl();

  if (CanonicalFD == ConsumerInstance->TheFuncDecl)
    return rewriteFuncDecl(FD);

  return true;
}

bool PToGASTVisitor::rewriteOneCallExpr(CallExpr *CallE)
{
  return 
    ConsumerInstance->RewriteHelper->removeArgFromCallExpr(CallE, 
                                        ConsumerInstance->TheParamPos);
}

void PToGASTVisitor::rewriteAllCallExprs(void)
{
  while (!AllCallExprs.empty()) {
    CallExpr *CallE = AllCallExprs.pop_back_val();
    rewriteOneCallExpr(CallE);
  }
}

bool PToGASTVisitor::VisitCallExpr(CallExpr *CallE) 
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

bool PToGASTVisitor::VisitDeclRefExpr(DeclRefExpr *ParmRefExpr)
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

