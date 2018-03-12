//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2014, 2015, 2016, 2017 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "RenameParam.h"

#include <sstream>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Another pass to increase readability of reduced code. \
It renames function parameters to p1, p2, ...\n";

static RegisterTransformation<RenameParam>
         Trans("rename-param", DescriptionMsg);

class ExistingVarCollectionVisitor : public 
  RecursiveASTVisitor<ExistingVarCollectionVisitor> {
public:

  explicit ExistingVarCollectionVisitor(RenameParam *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitVarDecl(VarDecl *VD);

private:

  RenameParam *ConsumerInstance;
};

class RenameParamVisitor : public RecursiveASTVisitor<RenameParamVisitor> {
public:

  explicit RenameParamVisitor(RenameParam *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitFunctionDecl(FunctionDecl *FD);

  bool VisitDeclRefExpr(DeclRefExpr *DRE);

private:

  RenameParam *ConsumerInstance;

  llvm::DenseMap<ParmVarDecl *, std::string> ParamNameMap;

};

bool ExistingVarCollectionVisitor::VisitVarDecl(VarDecl *VD)
{
  if (ConsumerInstance->isInIncludedFile(VD))
    return true;
 
  ParmVarDecl *PD = dyn_cast<ParmVarDecl>(VD);
  if (PD) {
    ConsumerInstance->validateParam(PD);
    return true;
  }

  VarDecl *CanonicalVD = VD->getCanonicalDecl();

  if (CanonicalVD->isLocalVarDecl()) {
    ConsumerInstance->addLocalVar(VD);
  }
  else {
    ConsumerInstance->addGlobalVar(VD);
  }
  return true;
}

bool RenameParamVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  if (ConsumerInstance->isInIncludedFile(FD) || (FD->param_size() == 0))
    return true;

  FunctionDecl *CanonicalFD = FD->getCanonicalDecl();
  unsigned int CurrPostfix = 0;
  ParamNameMap.clear();

  for(FunctionDecl::param_iterator I = FD->param_begin(),
      E = FD->param_end(); I != E; ++I) {

    CurrPostfix++;
    ParmVarDecl *PD = (*I);
    if (PD->getNameAsString().empty())
      continue;

    CurrPostfix = ConsumerInstance->validatePostfix(CanonicalFD, CurrPostfix);
    std::stringstream TmpSS;
    TmpSS << ConsumerInstance->ParamNamePrefix << CurrPostfix;

    ConsumerInstance->RewriteHelper->replaceVarDeclName(PD, TmpSS.str());

    if (FD->isThisDeclarationADefinition()) {
      ParamNameMap[*I] = TmpSS.str();
    }
  }
  return true;
}

bool RenameParamVisitor::VisitDeclRefExpr(DeclRefExpr *DRE)
{
  if (ConsumerInstance->isInIncludedFile(DRE))
    return true;
  ValueDecl *OrigDecl = DRE->getDecl();
  ParmVarDecl *PD = dyn_cast<ParmVarDecl>(OrigDecl);
  
  if (!PD || ConsumerInstance->isInIncludedFile(PD))
    return true;

  llvm::DenseMap<ParmVarDecl *, std::string>::iterator I =
    ParamNameMap.find(PD);
  TransAssert((I != ParamNameMap.end()) && "Bad Param!");
  
  return ConsumerInstance->RewriteHelper->replaceExpr(DRE, (*I).second);
}

void RenameParam::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  VarCollectionVisitor = new ExistingVarCollectionVisitor(this);
  RenameVisitor = new RenameParamVisitor(this);
  ValidInstanceNum = 1;
}

void RenameParam::HandleTranslationUnit(ASTContext &Ctx)
{
  VarCollectionVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
  if (QueryInstanceOnly) {
    if (!HasValidParams)
      ValidInstanceNum = 0;
    return;
  }

  if (!HasValidParams) {
    TransError = TransNoValidParamsError;
    return;
  }
  else if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  TransAssert(RenameVisitor && "NULL RenameVisitor!");
  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  RenameVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

bool RenameParam::getPostfixValue(const std::string &Name, 
                                  unsigned int &Value)
{
  // It's an unamed parameter, we skip it
  if (Name.size() == 0)
    return true;

  if (Name.size() == 1)
    return false;

  std::string Prefix = Name.substr(0, 1);
  if (Prefix != ParamNamePrefix)
    return false;

  std::string RestStr = Name.substr(1);
  std::stringstream TmpSS(RestStr);
  if (!(TmpSS >> Value))
    return false;

  return true;
}

void RenameParam::validateParam(ParmVarDecl *PD)
{
  unsigned int Value;
  if (PD->isReferenced() && !getPostfixValue(PD->getNameAsString(), Value))
    HasValidParams = true;
}

void RenameParam::addGlobalVar(VarDecl *VD)
{
  unsigned int PostValue;
  if (!getPostfixValue(VD->getNameAsString(), PostValue))
    return;

  ExistingGlobalVars.insert(PostValue);
}

void RenameParam::addLocalVar(VarDecl *VD)
{
  unsigned int PostValue;
  if (!getPostfixValue(VD->getNameAsString(), PostValue))
    return;

  DeclContext *Ctx = VD->getDeclContext();
  FunctionDecl *FD = dyn_cast<FunctionDecl>(Ctx); 
  TransAssert(FD && "Bad function declaration!");
  FunctionDecl *CanonicalFD = FD->getCanonicalDecl();

  ExistingNumberSet *CurrSet;
  llvm::DenseMap<FunctionDecl *, ExistingNumberSet *>::iterator I =
    FunExistingVarsMap.find(CanonicalFD);

  if (I == FunExistingVarsMap.end()) {
    CurrSet = new ExistingNumberSet();
    FunExistingVarsMap[CanonicalFD] = CurrSet;
  }
  else {
    CurrSet = (*I).second;
  }

  CurrSet->insert(PostValue);
}

bool RenameParam::isValidPostfix(ExistingNumberSet *LocalSet, 
                                 unsigned int Postfix)
{
  if (ExistingGlobalVars.count(Postfix))
    return false;

  if (!LocalSet)
    return true;

  return !LocalSet->count(Postfix);
}

unsigned int RenameParam::validatePostfix(FunctionDecl *FD, 
                                          unsigned int CurrPostfix)
{
  int MaxIteration = 0;
  ExistingNumberSet *LocalNumberSet = NULL;

  llvm::DenseMap<FunctionDecl *, ExistingNumberSet *>::iterator I =
    FunExistingVarsMap.find(FD);

  if (I != FunExistingVarsMap.end()) {
    LocalNumberSet = (*I).second;
    MaxIteration += static_cast<int>(LocalNumberSet->size());
  }
  MaxIteration += static_cast<int>(ExistingGlobalVars.size());

  while (!isValidPostfix(LocalNumberSet, CurrPostfix)) {
    CurrPostfix++;
    MaxIteration--;
    TransAssert((MaxIteration >= 0) && "Bad Postfix!");
  }

  return CurrPostfix;
}

RenameParam::~RenameParam(void)
{
  if (VarCollectionVisitor)
    delete VarCollectionVisitor;
  if (RenameVisitor)
    delete RenameVisitor;

  for (llvm::DenseMap<FunctionDecl *, ExistingNumberSet *>::iterator 
        I = FunExistingVarsMap.begin(), E = FunExistingVarsMap.end();
        I != E; ++I) {
    delete (*I).second;
  }
}
