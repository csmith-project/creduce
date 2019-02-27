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

#include "RenameVar.h"

#include <algorithm>
#include <sstream>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"To increase readability, rename global and local variables \
to a, b, ..., z. The transformation returns with error message \
if there are more than 26 different variables. \n";

static RegisterTransformation<RenameVar>
         Trans("rename-var", DescriptionMsg);

class RNVCollectionVisitor : public RecursiveASTVisitor<RNVCollectionVisitor> {
public:

  explicit RNVCollectionVisitor(RenameVar *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitVarDecl(VarDecl *VD);

private:

  RenameVar *ConsumerInstance;

};

class RenameVarVisitor : public RecursiveASTVisitor<RenameVarVisitor> {
public:

  explicit RenameVarVisitor(RenameVar *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitVarDecl(VarDecl *VD);

  bool VisitDeclRefExpr(DeclRefExpr *DRE);

private:

  RenameVar *ConsumerInstance;

};

bool RNVCollectionVisitor::VisitVarDecl(VarDecl *VD)
{
  if (ConsumerInstance->isInIncludedFile(VD))
    return true;

  ParmVarDecl *PV = dyn_cast<ParmVarDecl>(VD);
  // Skip parameters
  if (PV)
    return true;

  VarDecl *CanonicalVD = VD->getCanonicalDecl();
  ConsumerInstance->addVar(CanonicalVD);
  return true;
}

bool RenameVarVisitor::VisitVarDecl(VarDecl *VD)
{
  VarDecl *CanonicalDecl = VD->getCanonicalDecl();
  llvm::DenseMap<VarDecl *, std::string>::iterator I = 
    ConsumerInstance->VarToNameMap.find(CanonicalDecl);

  if (I == ConsumerInstance->VarToNameMap.end())
    return true;

  return ConsumerInstance->RewriteHelper->replaceVarDeclName(VD, (*I).second);
}

bool RenameVarVisitor::VisitDeclRefExpr(DeclRefExpr *DRE)
{
  ValueDecl *OrigDecl = DRE->getDecl();
  VarDecl *VD = dyn_cast<VarDecl>(OrigDecl);
  if (!VD)
    return true;

  VarDecl *CanonicalDecl = VD->getCanonicalDecl();
  llvm::DenseMap<VarDecl *, std::string>::iterator I = 
    ConsumerInstance->VarToNameMap.find(CanonicalDecl);

  if (I == ConsumerInstance->VarToNameMap.end())
    return true;

  // We can visit the same DRE twice from an InitListExpr, i.e.,
  // through InitListExpr's semantic form and syntactic form.
  if (ConsumerInstance->VisitedDREs.count(DRE))
    return true;
  ConsumerInstance->VisitedDREs.insert(DRE);

  return ConsumerInstance->RewriteHelper->replaceExpr(DRE, (*I).second);
}

void RenameVar::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);

  VarCollectionVisitor = new RNVCollectionVisitor(this);
  RenameVisitor = new RenameVarVisitor(this);

  for (char C = 'z'; C >= 'a'; C--) {
    AvailableNames.push_back(C);
  }
  ValidInstanceNum = 1;
}

bool RenameVar::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    VarCollectionVisitor->TraverseDecl(*I);
  }
  return true;
}
 
void RenameVar::HandleTranslationUnit(ASTContext &Ctx)
{
  unsigned int NumNames = AvailableNames.size();
  unsigned int NumVars = ValidVars.size();

  if (NumVars == 0) {
    ValidInstanceNum = 0;
  }
  else if (NumVars > NumNames) {
    // TEMP: currently not to rename vars in C++ files if there are
    //       more than 26 global or local vars
    if (TransformationManager::isCXXLangOpt() || allValidNames()) {
      ValidInstanceNum = 0;
    }
    else {
      NumNames = NumVars;
    }
  }

  if (QueryInstanceOnly) {
    return;
  }

  if (NumVars > NumNames) {
    TransError = TransMaxVarsError;
    return;
  }
  else if (NumVars == 0) {
    TransError = TransNoValidVarsError;
    return;
  }
  else if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  TransAssert(RenameVisitor && "NULL RenameVisitor!");
  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  collectVars();
  RenameVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void RenameVar::addVar(VarDecl *VD)
{
  std::string Name = VD->getNameAsString();
  if (Name.size() > 1) {
    ValidVars.push_back(VD);
    return;
  }

  char C = Name[0];
  if ((C < 'a') || (C > 'z'))
    return;

  SmallVector<char, 26>::iterator I = 
    std::find(AvailableNames.begin(), AvailableNames.end(), C);

  // It could be the case where C has been erased, e.g.,
  // two variables from different scopes have the same name
  if (I == AvailableNames.end())
    return;    

  AvailableNames.erase(I);
}

bool RenameVar::allValidNames(void)
{
  for (std::vector<VarDecl*>::iterator I = ValidVars.begin(),
       E = ValidVars.end(); I != E; ++I) {
    VarDecl *VD = (*I);
    std::string Name = VD->getNameAsString();
    if (Name.size() == 1) {
      char C = Name[0];
      if ((C < 'a') || (C > 'z'))
        return false;
    }
    else {
      std::stringstream TmpSS(Name.substr(1));
      unsigned int Value;
      if (!(TmpSS >> Value))
        return false;
    }
  }
  return true;
}

void RenameVar::collectVars(void)
{
  unsigned Count = 1;
  for (std::vector<VarDecl*>::iterator I = ValidVars.begin(),
       E = ValidVars.end(); I != E; ++I) {
    VarDecl *VD = (*I);
    if (AvailableNames.size()) {
      char Name = AvailableNames.back();
      AvailableNames.pop_back();
      VarToNameMap[VD] = std::string(1, Name);
    }
    else {
      std::stringstream TmpSS;
      TmpSS << RenamePrefix << Count;
      VarToNameMap[VD] = TmpSS.str();
      Count++;
    }
  }
}

RenameVar::~RenameVar(void)
{
  delete VarCollectionVisitor;
  delete RenameVisitor;
}

