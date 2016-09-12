//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2015, 2016 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "RemoveUnusedVar.h"

#include <cctype>
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
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

  bool VisitCXXCatchStmt(CXXCatchStmt *DS);

private:

  RemoveUnusedVar *ConsumerInstance;
};

bool RemoveUnusedVarAnalysisVisitor::VisitVarDecl(VarDecl *VD)
{
  if (ConsumerInstance->isInIncludedFile(VD))
    return true;

  if (VD->isReferenced() || dyn_cast<ParmVarDecl>(VD) || 
      VD->isStaticDataMember())
    return true;

  SourceRange VarRange = VD->getSourceRange();
  if (VarRange.getEnd().isInvalid())
    return true;

  if (ConsumerInstance->SkippedVars.count(VD->getCanonicalDecl()))
    return true;

  ConsumerInstance->ValidInstanceNum++;
  if (ConsumerInstance->ToCounter > 0) {
    ConsumerInstance->AllValidVarDecls.push_back(VD);
    return true;
  }
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

bool RemoveUnusedVarAnalysisVisitor::VisitCXXCatchStmt(CXXCatchStmt *S)
{
  const VarDecl *VD = S->getExceptionDecl();
  if (VD) {
    ConsumerInstance->SkippedVars.insert(VD->getCanonicalDecl());
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
  if (ToCounter > ValidInstanceNum) {
    TransError = TransToCounterTooBigError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  doRewriting();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void RemoveUnusedVar::doRewriting()
{
  if (ToCounter <= 0) {
    TransAssert(TheVarDecl && "NULL TheVarDecl!");
    removeVarDecl(TheVarDecl);
    return;
  }

  TransAssert((TransformationCounter <=
               static_cast<int>(AllValidVarDecls.size())) &&
              "TransformationCounter is larger than the number of decls!");
  TransAssert((ToCounter <= static_cast<int>(AllValidVarDecls.size())) &&
              "ToCounter is larger than the number of decls!");
  for (int I = ToCounter; I >= TransformationCounter; --I) {
    TransAssert((I >= 1) && "Invalid Index!");
    const VarDecl *VD = AllValidVarDecls[I-1];
    TransAssert(VD && "NULL FunctionDecl!");
    removeVarDecl(VD);
  }
}

void RemoveUnusedVar::removeVarDeclFromLinkageSpecDecl(
       const LinkageSpecDecl *LinkageD, const VarDecl *VD)
{
  const DeclContext *Ctx = LinkageSpecDecl::castToDeclContext(LinkageD);
  unsigned NumDecls = 0;
  for (DeclContext::decl_iterator I = Ctx->decls_begin(), E = Ctx->decls_end();
       I != E; ++I) {
    NumDecls++;
    if (NumDecls > 1)
      break;
  }

  if (NumDecls <= 1) {
    RewriteHelper->removeDecl(LinkageD);
    return;
  }
  else {
    RewriteHelper->removeVarDecl(VD);
  }
}

void RemoveUnusedVar::removeVarDecl(const VarDecl *VD)
{
  const DeclContext *Ctx = VD->getDeclContext();
  if (const LinkageSpecDecl *LinkageDecl = dyn_cast<LinkageSpecDecl>(Ctx)) {
    removeVarDeclFromLinkageSpecDecl(LinkageDecl, VD);
    return;
  }
  else if (dyn_cast<NamespaceDecl>(Ctx)) {
    // if a var is declared inside a namespace, we don't know
    // which declaration group it belongs to. 
    RewriteHelper->removeVarDecl(VD);
    return;
  }

  llvm::DenseMap<const VarDecl *, DeclGroupRef>::iterator DI = 
    VarToDeclGroup.find(VD);
  if (DI == VarToDeclGroup.end()) {
    // We don't know the decl group that the var decl belongs to.
    RewriteHelper->removeVarDecl(VD);
  }
  else {
    RewriteHelper->removeVarDecl(VD, (*DI).second);
  }
}

RemoveUnusedVar::~RemoveUnusedVar(void)
{
  delete AnalysisVisitor;
}

