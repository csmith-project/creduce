//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "RemovePointer.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;
using namespace llvm;

static const char *DescriptionMsg =
"Only reduce the level of a pointer var if this pointer is \
not referenced or dereferenced except being used as an operand of \
a comparison operator where another operand is 0. We only change \
the definition of this pointer. \n";

static RegisterTransformation<RemovePointer>
         Trans("remove-pointer", DescriptionMsg);

class RemovePointerCollectionVisitor : public 
  RecursiveASTVisitor<RemovePointerCollectionVisitor> {

public:

  explicit RemovePointerCollectionVisitor(RemovePointer *Instance)
    : ConsumerInstance(Instance),
      CurrDeclRefExpr(NULL)
  { }

  bool VisitVarDecl(VarDecl *VD);

  bool VisitDeclRefExpr(DeclRefExpr *DRE);

  bool VisitBinaryOperator(BinaryOperator *BO);

private:

  RemovePointer *ConsumerInstance;

  const DeclRefExpr *CurrDeclRefExpr;
};

bool RemovePointerCollectionVisitor::VisitVarDecl(VarDecl *VD)
{
  ConsumerInstance->handleOneVarDecl(VD);
  return true;
}

bool RemovePointerCollectionVisitor::VisitDeclRefExpr(DeclRefExpr *DRE)
{
  if (CurrDeclRefExpr != DRE) 
    ConsumerInstance->invalidateOneVarDecl(DRE);

  CurrDeclRefExpr = NULL;
  return true;
}

bool RemovePointerCollectionVisitor::VisitBinaryOperator(BinaryOperator *BO)
{
  if (!BO->isComparisonOp())
    return true;

  const Expr *E = NULL;
  if (dyn_cast<IntegerLiteral>(BO->getLHS()->IgnoreParenCasts())) {
    E = BO->getRHS()->IgnoreParenCasts();
  }
  else if (dyn_cast<IntegerLiteral>(BO->getRHS()->IgnoreParenCasts())) {
    E = BO->getLHS()->IgnoreParenCasts();
  }
  
  if (!E)
    return true;

  const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E);
  if (!DRE)
    return true;

  const ValueDecl *OrigDecl = DRE->getDecl();
  const VarDecl *VD = dyn_cast<VarDecl>(OrigDecl);
  if (!VD || dyn_cast<ParmVarDecl>(VD))
    return true;
  
  if (ConsumerInstance->AllPointerVarDecls.count(VD) && 
      !(ConsumerInstance->AllInvalidPointerVarDecls.count(VD)))
    CurrDeclRefExpr = DRE;
  return true;
}

void RemovePointer::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new RemovePointerCollectionVisitor(this);
}

void RemovePointer::HandleTranslationUnit(ASTContext &Ctx)
{
  TransAssert(CollectionVisitor && "NULL CollectionVisitor!");
  CollectionVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
  doAnalysis();

  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);
  TransAssert(TheVarDecl && "NULL TheVarDecl!");

  doRewriting();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void RemovePointer::doAnalysis(void)
{
  for (VarDeclSet::iterator I = AllPointerVarDecls.begin(),
       E = AllPointerVarDecls.end(); I != E; ++I) {
    const VarDecl *VD = (*I);
    if (AllInvalidPointerVarDecls.count(VD))
      continue;

    ValidInstanceNum++;
    if (TransformationCounter == ValidInstanceNum)
      TheVarDecl = VD;
  }
}

void RemovePointer::doRewriting(void)
{
  const VarDecl *FirstVD = TheVarDecl->getCanonicalDecl();
  for(VarDecl::redecl_iterator RI = FirstVD->redecls_begin(),
      RE = FirstVD->redecls_end(); RI != RE; ++RI) {
    RewriteHelper->removeAStarBefore(*RI); 
  }
}

void RemovePointer::invalidateOneVarDecl(const DeclRefExpr *DRE)
{
  const ValueDecl *OrigDecl = DRE->getDecl();
  const VarDecl *VD = dyn_cast<VarDecl>(OrigDecl);
  if (!VD || dyn_cast<ParmVarDecl>(VD))
    return;

  const Type *Ty = VD->getType().getTypePtr();
  if (!Ty->isPointerType())
    return;
  const VarDecl *CanonicalVD = VD->getCanonicalDecl();
  AllInvalidPointerVarDecls.insert(CanonicalVD);
}

void RemovePointer::handleOneVarDecl(const VarDecl *VD)
{
  if (dyn_cast<ParmVarDecl>(VD))
    return;

  if (VD->getAnyInitializer())
    return;

  const Type *Ty = VD->getType().getTypePtr();
  if (!Ty->isPointerType())
    return;

  AllPointerVarDecls.insert(VD->getCanonicalDecl());
}

RemovePointer::~RemovePointer(void)
{
  delete CollectionVisitor;
}

