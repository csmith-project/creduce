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

#include "RemoveUnusedVar.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Remove unused local/global variable declarations. \n";

static RegisterTransformation<RemoveUnusedVar>
         Trans("remove-unused-var", DescriptionMsg);

class RemoveUnusedVarAnalysisVisitor : public 
  RecursiveASTVisitor<RemoveUnusedVarAnalysisVisitor> {
public:

  explicit RemoveUnusedVarAnalysisVisitor(RemoveUnusedVar *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitVarDecl(VarDecl *VD);

  bool VisitDeclStmt(DeclStmt *DS);

private:

  RemoveUnusedVar *ConsumerInstance;
};

bool RemoveUnusedVarAnalysisVisitor::VisitVarDecl(VarDecl *VD)
{
  if (VD->isReferenced() || dyn_cast<ParmVarDecl>(VD))
    return true;

  ConsumerInstance->ValidInstanceNum++;
  if (ConsumerInstance->ValidInstanceNum == 
      ConsumerInstance->TransformationCounter) {
    ConsumerInstance->TheVarDecl = VD;
  }
  return true;
}

bool RemoveUnusedVarAnalysisVisitor::VisitDeclStmt(DeclStmt *DS)
{   
  for (DeclStmt::decl_iterator I = DS->decl_begin(),
       E = DS->decl_end(); I != E; ++I) {
    VarDecl *CurrDecl = dyn_cast<VarDecl>(*I);
    if (CurrDecl) {
      DeclGroupRef DGR = DS->getDeclGroup();
      ConsumerInstance->VarToDeclGroup[CurrDecl] = DGR;
    }
  }
  return true;
}

void RemoveUnusedVar::Initialize(ASTContext &context)
{
  Transformation::Initialize(context);
  AnalysisVisitor = new RemoveUnusedVarAnalysisVisitor(this);
}

bool RemoveUnusedVar::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    VarDecl *VD = dyn_cast<VarDecl>(*I);
    if (VD)
      VarToDeclGroup[VD] = D;
  }
  return true;
}
 
void RemoveUnusedVar::HandleTranslationUnit(ASTContext &Ctx)
{
  AnalysisVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  TransAssert(TheVarDecl && "NULL TheFunctionDecl!");

  removeVarDecl();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void RemoveUnusedVar::removeVarDecl(void)
{
  llvm::DenseMap<const VarDecl *, DeclGroupRef>::iterator DI = 
    VarToDeclGroup.find(TheVarDecl);
  TransAssert((DI != VarToDeclGroup.end()) &&
              "Cannot find VarDeclGroup!");

  RewriteHelper->removeVarDecl(TheVarDecl, (*DI).second);
}

RemoveUnusedVar::~RemoveUnusedVar(void)
{
  if (AnalysisVisitor)
    delete AnalysisVisitor;
}

