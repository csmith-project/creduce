//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "RemoveArray.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;
using namespace llvm;

static const char *DescriptionMsg =
"Change an array var to a corresponding non-array one. \
For example, replace \"int a[10];\" with \"int a;\". Conditions \n\
  * Array dimension is 1, and \n\
  * array var is not referenced except via ArraySubscriptExpr. \n\
All relevant ArraySubscriptExpr[s] will be rewritten accordingly. \n";

static RegisterTransformation<RemoveArray>
         Trans("remove-array", DescriptionMsg);

class RemoveArrayCollectionVisitor : public 
  RecursiveASTVisitor<RemoveArrayCollectionVisitor> {

public:

  explicit RemoveArrayCollectionVisitor(RemoveArray *Instance)
    : ConsumerInstance(Instance),
      CurrDeclRefExpr(NULL)
  { }

  bool VisitVarDecl(VarDecl *VD);

  bool VisitArraySubscriptExpr(ArraySubscriptExpr *ASE);

  bool VisitDeclRefExpr(DeclRefExpr *DRE);

private:

  RemoveArray *ConsumerInstance;

  const DeclRefExpr *CurrDeclRefExpr;
};

bool RemoveArrayCollectionVisitor::VisitVarDecl(VarDecl *VD)
{
  ConsumerInstance->handleOneVarDecl(VD);
  return true;
}

bool RemoveArrayCollectionVisitor::VisitArraySubscriptExpr(ArraySubscriptExpr *ASE)
{
  // we only have one-dimension array, so we are safe here.
  const Expr *BaseE = ASE->getBase()->IgnoreParenCasts();
  
  TransAssert(BaseE && "Empty Base expression!");
  const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(BaseE);
  if (!DRE)
    return true;

  CurrDeclRefExpr = DRE;
  ConsumerInstance->addOneArraySubscriptExpr(ASE, DRE);
  return true;
}

bool RemoveArrayCollectionVisitor::VisitDeclRefExpr(DeclRefExpr *DRE)
{
  if (CurrDeclRefExpr != DRE) 
    ConsumerInstance->deleteOneVarDecl(DRE);

  CurrDeclRefExpr = NULL;
  return true;
}

void RemoveArray::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new RemoveArrayCollectionVisitor(this);
}

void RemoveArray::HandleTranslationUnit(ASTContext &Ctx)
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
  TransAssert(TheArrayVarDecl && "NULL TheArrayVarDecl!");

  doRewriting();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void RemoveArray::doAnalysis(void)
{
  for (VarDeclToArraySubscriptExprMap::iterator I = ValidVarToASEMap.begin(),
       E = ValidVarToASEMap.end(); I != E; ++I) {
    if (!((*I).second))
      continue;
    ValidInstanceNum++;
    if (TransformationCounter == ValidInstanceNum) {
      TheArrayVarDecl = (*I).first;
      TheASEVec = (*I).second;
    }
  }
}

void RemoveArray::getBracketLocPair(const VarDecl *VD,
                                    BracketLocPair &LocPair)
{
  SourceLocation StartLoc = VD->getLocation();
  SourceLocation BPStartLoc, BPEndLoc;
  BPStartLoc = RewriteHelper->getLocationUntil(StartLoc, '[');
  BPEndLoc = RewriteHelper->getLocationUntil(BPStartLoc, ']');

  LocPair.first = BPStartLoc;
  LocPair.second = BPEndLoc;
}

void RemoveArray::doRewriting(void)
{
  // rewrite decls first
  const VarDecl *FirstVD = TheArrayVarDecl->getFirstDecl();
  for(VarDecl::redecl_iterator RI = FirstVD->redecls_begin(),
      RE = FirstVD->redecls_end(); RI != RE; ++RI) {

    BracketLocPair LocPair;
    getBracketLocPair((*RI), LocPair);
    TheRewriter.RemoveText(SourceRange(LocPair.first, LocPair.second));
  }

  // Then rewrite ArraySubscriptExprs
  if (!TheASEVec)
    return;

  for (ArraySubscriptExprVector::iterator I = TheASEVec->begin(),
       E = TheASEVec->end(); I != E; ++I) {
    const Expr *IdxE = (*I)->getIdx();
    RewriteHelper->removeArraySubscriptExpr(IdxE); 
  } 
}

void RemoveArray::deleteOneVarDecl(const DeclRefExpr *DRE)
{
  const ValueDecl *OrigDecl = DRE->getDecl();
  const VarDecl *VD = dyn_cast<VarDecl>(OrigDecl);
  if (!VD)
    return;
  const VarDecl *CanonicalVD = VD->getCanonicalDecl();
  ArraySubscriptExprVector *ASEVec = ValidVarToASEMap[CanonicalVD];
  if (ASEVec) {
    delete ASEVec;
    ValidVarToASEMap[CanonicalVD] = NULL;
  }
}

void RemoveArray::addOneArraySubscriptExpr(const ArraySubscriptExpr *ASE, 
                                           const DeclRefExpr *DRE)
{
  const ValueDecl *OrigDecl = DRE->getDecl();
  const VarDecl *VD = dyn_cast<VarDecl>(OrigDecl);
  if (!VD)
    return;

  const VarDecl *CanonicalVD = VD->getCanonicalDecl();
  ArraySubscriptExprVector *ASEVec = ValidVarToASEMap[CanonicalVD];
  if (ASEVec)
    ASEVec->push_back(ASE);
}

void RemoveArray::handleOneVarDecl(const VarDecl *VD)
{
  if (VD->getAnyInitializer())
    return;

  const Type *Ty = VD->getType().getTypePtr();
  const ArrayType *ArrayTy = dyn_cast<ArrayType>(Ty);
  if (!ArrayTy)
    return;
  
  unsigned Dim = getArrayDimension(ArrayTy);
  if (Dim != 1)
    return;

  const VarDecl *CanonicalVD = VD->getCanonicalDecl();
  ArraySubscriptExprVector *ASEVec = ValidVarToASEMap[CanonicalVD];
  if (ASEVec)
    return;

  ASEVec = new ArraySubscriptExprVector();
  ValidVarToASEMap[CanonicalVD] = ASEVec;
}

RemoveArray::~RemoveArray(void)
{
  if (CollectionVisitor)
    delete CollectionVisitor;

  for (VarDeclToArraySubscriptExprMap::iterator I = ValidVarToASEMap.begin(),
       E = ValidVarToASEMap.end(); I != E; ++I) {
    ArraySubscriptExprVector *ASEVec = (*I).second;
    if (ASEVec)
      delete ASEVec;
  }
}
