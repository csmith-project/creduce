//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2015, 2016, 2017, 2018 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "RenameFun.h"

#include <sstream>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Another pass to increase readability of reduced code. \
It renames function names to fn1, fn2, ...\n";

static RegisterTransformation<RenameFun>
         Trans("rename-fun", DescriptionMsg);

class RNFunCollectionVisitor : public 
  RecursiveASTVisitor<RNFunCollectionVisitor> {

public:

  explicit RNFunCollectionVisitor(RenameFun *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitFunctionDecl(FunctionDecl *FD);

  bool VisitCallExpr(CallExpr *CE);

private:

  RenameFun *ConsumerInstance;

};

class RenameFunVisitor : public RecursiveASTVisitor<RenameFunVisitor> {
public:

  explicit RenameFunVisitor(RenameFun *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitFunctionDecl(FunctionDecl *FD);

  bool VisitDeclRefExpr(DeclRefExpr *DRE);

private:

  RenameFun *ConsumerInstance;

};

bool RNFunCollectionVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  // renaming CXXMethodDecl is handled in a seperate pass
  if (dyn_cast<CXXMethodDecl>(FD)) {
    return true;
  }

  const FunctionDecl *CanonicalFD = FD->getCanonicalDecl();
  if (ConsumerInstance->isInIncludedFile(FD) ||
      ConsumerInstance->isInIncludedFile(CanonicalFD))
    return true;

  ConsumerInstance->addFun(CanonicalFD);
  if (!ConsumerInstance->hasValidPostfix(FD->getNameAsString()))
    ConsumerInstance->HasValidFuns = true;
  return true;
}

bool RNFunCollectionVisitor::VisitCallExpr(CallExpr *CE)
{
  if (ConsumerInstance->isInIncludedFile(CE))
    return true;
  FunctionDecl *FD = CE->getDirectCallee();
  // It could happen, e.g., CE could refer to a DependentScopeDeclRefExpr
  if (!FD || dyn_cast<CXXMethodDecl>(FD))
    return true;
  if (ConsumerInstance->isInIncludedFile(FD))
    return true;

  const FunctionDecl *CanonicalFD = FD->getCanonicalDecl();

  // This case is handled by VisitFunctionDecl
  if (CanonicalFD->isDefined())
    return true;

  // It's possible we don't have function definition
  ConsumerInstance->addFun(CanonicalFD);
  if (!ConsumerInstance->hasValidPostfix(FD->getNameAsString()))
    ConsumerInstance->HasValidFuns = true;
  return true;
}

bool RenameFunVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  FunctionDecl *CanonicalDecl = FD->getCanonicalDecl();
  if (ConsumerInstance->isInIncludedFile(FD) ||
      ConsumerInstance->isInIncludedFile(CanonicalDecl) ||
      dyn_cast<CXXMethodDecl>(FD)) {
    return true;
  }

  llvm::DenseMap<const FunctionDecl *, std::string>::iterator I = 
    ConsumerInstance->FunToNameMap.find(CanonicalDecl);

  TransAssert((I != ConsumerInstance->FunToNameMap.end()) &&
              "Cannot find FunctionDecl!");

  return ConsumerInstance->RewriteHelper->
           replaceFunctionDeclName(FD, (*I).second);
}

bool RenameFunVisitor::VisitDeclRefExpr(DeclRefExpr *DRE)
{
  if (ConsumerInstance->isInIncludedFile(DRE))
    return true;

  ValueDecl *OrigDecl = DRE->getDecl();
  FunctionDecl *FD = dyn_cast<FunctionDecl>(OrigDecl);
  if (!FD || dyn_cast<CXXMethodDecl>(FD) ||
      ConsumerInstance->isInIncludedFile(FD))
    return true;

  FunctionDecl *CanonicalDecl = FD->getCanonicalDecl();
  llvm::DenseMap<const FunctionDecl *, std::string>::iterator I = 
    ConsumerInstance->FunToNameMap.find(CanonicalDecl);

  TransAssert((I != ConsumerInstance->FunToNameMap.end()) &&
              "Cannot find FunctionDecl!");
  ConsumerInstance->TheRewriter.ReplaceText(DRE->getLocStart(), 
    FD->getNameAsString().size(), (*I).second);
  return true;
}

void RenameFun::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  FunCollectionVisitor = new RNFunCollectionVisitor(this);
  RenameVisitor = new RenameFunVisitor(this);
  ValidInstanceNum = 1;
}

bool RenameFun::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    FunCollectionVisitor->TraverseDecl(*I);
  }
  return true;
}

bool RenameFun::isConsecutiveNumbersFromOne(void)
{
  size_t Sz = AllValidNumbers.size();
  if (!Sz)
    return true;

  bool hasOne = AllValidNumbers.count(1);
  if (!hasOne)
    return false;

  if (Sz == 1) {
    return hasOne;
  }

  std::set<unsigned int>::iterator I = 
    max_element(AllValidNumbers.begin(), AllValidNumbers.end());
  return ((*I) == Sz);
}

bool RenameFun::hasValidFuns(void)
{
  return (HasValidFuns || !isConsecutiveNumbersFromOne());
}

void RenameFun::HandleTranslationUnit(ASTContext &Ctx)
{
  if (QueryInstanceOnly) {
    if (hasValidFuns())
      ValidInstanceNum = 1;
    else
      ValidInstanceNum = 0;
    return;
  }

  if (!hasValidFuns()) {
    TransError = TransNoValidFunsError;
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

bool RenameFun::isSpecialFun(const std::string &Name)
{
  if ((Name.compare("main") == 0) ||
      (Name.compare("printf") == 0))
    return true;
  else
    return false;
}

bool RenameFun::hasValidPostfix(const std::string &Name)
{
  unsigned int Value;

  // Don't rename special functions
  if (isSpecialFun(Name))
    return true;

  if (Name.size() <= 2)
    return false;

  std::string Prefix = Name.substr(0, 2);
  if (Prefix != FunNamePrefix)
    return false;

  std::string RestStr = Name.substr(2);
  std::stringstream TmpSS(RestStr);
  if (!(TmpSS >> Value))
    return false;

  AllValidNumbers.insert(Value);
  return true;
}

void RenameFun::addFun(const FunctionDecl *FD)
{
  std::string Name = FD->getNameAsString();
  // Skip special functions
  if (isSpecialFun(Name) || FD->hasAttr<OpenCLKernelAttr>())
    FunToNameMap[FD] = Name;

  if (FunToNameMap.find(FD) != FunToNameMap.end())
    return;

  std::stringstream SS;

  FunNamePostfix++;
  SS << FunNamePrefix << FunNamePostfix;

  TransAssert((FunToNameMap.find(FD) == FunToNameMap.end()) &&
              "Duplicated Fun name!");

  FunToNameMap[FD] = SS.str();
}

RenameFun::~RenameFun(void)
{
  delete FunCollectionVisitor;
  delete RenameVisitor;
}

