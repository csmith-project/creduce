//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2015, 2017 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "ReplaceArrayIndexVar.h"

#include <sstream>
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Looking for the pattern like below: \n\
  for (...;...; i++) { \n\
    a[i]; \n\
  } \n\
if  is less than 15, then replace a[i] with  a[0] ... a[num-1]";

static RegisterTransformation<ReplaceArrayIndexVar>
         Trans("replace-array-index-var", DescriptionMsg);

typedef llvm::SmallPtrSet<const clang::ArraySubscriptExpr *, 10>
  ArraySubscriptExprSet;

typedef llvm::DenseMap<const clang::VarDecl *, ArraySubscriptExprSet *>
  VarDeclToASESetMap;

typedef llvm::DenseMap<const clang::VarDecl *, unsigned>
  ArrayVarsToSizeMap;

class ReplaceArrayIndexVarCollectionVisitor : public 
  RecursiveASTVisitor<ReplaceArrayIndexVarCollectionVisitor> {

public:

  explicit ReplaceArrayIndexVarCollectionVisitor(ReplaceArrayIndexVar *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitVarDecl(VarDecl *VD);

  bool VisitForStmt(ForStmt *FS);

private:

  ReplaceArrayIndexVar *ConsumerInstance;
};

class ArraySubscriptExprCollectionVisitor : public
  RecursiveASTVisitor<ArraySubscriptExprCollectionVisitor> {

public:

  ArraySubscriptExprCollectionVisitor(const VarDecl *VD,
                                      ArrayVarsToSizeMap &Vars,
                                      VarDeclToASESetMap &Map)
    : IdxVD(VD),
      CstArrayVars(Vars),
      ASEMap(Map)
  { }

  ~ArraySubscriptExprCollectionVisitor();

  bool VisitArraySubscriptExpr(ArraySubscriptExpr *ASE);

private:

  const VarDecl *getVarDeclFromExpr( const Expr *E);

  const VarDecl *IdxVD;

  ArrayVarsToSizeMap &CstArrayVars;

  VarDeclToASESetMap &ASEMap;
};

bool ReplaceArrayIndexVarCollectionVisitor::VisitVarDecl(VarDecl *VD)
{
  const VarDecl *CanonicalVD = VD->getCanonicalDecl();
  if (ConsumerInstance->CstArrayVars[CanonicalVD])
    return true;

  const Type *Ty = CanonicalVD->getType().getTypePtr();
  const ConstantArrayType *CstArrayTy = dyn_cast<ConstantArrayType>(Ty);
  if (!CstArrayTy)
    return true;

  // skip multi-dimension arrays
  const Type *ElemTy = CstArrayTy->getElementType().getTypePtr();
  if (dyn_cast<ArrayType>(ElemTy))
    return true;

  llvm::APInt APSz = CstArrayTy->getSize();
  unsigned Sz = (unsigned int)(*APSz.getRawData());
  if (Sz <= ConsumerInstance->MaxSize)
    ConsumerInstance->CstArrayVars[CanonicalVD] = Sz;

  return true;
}

bool ReplaceArrayIndexVarCollectionVisitor::VisitForStmt(ForStmt *FS)
{
  if (ConsumerInstance->isInIncludedFile(FS))
    return true;

  const Expr *Inc = FS->getInc();
  if (!Inc)
    return true;

  Stmt *Body = FS->getBody();
  if (!Body || dyn_cast<NullStmt>(Body))
    return true;

  const DeclRefExpr *DRE;
  if (const BinaryOperator *BO = 
      dyn_cast<BinaryOperator>(Inc->IgnoreParenCasts())) {
    if (!BO || !BO->isAssignmentOp())
      return true;

    const Expr *Lhs = BO->getLHS()->IgnoreParenCasts();
    DRE = dyn_cast<DeclRefExpr>(Lhs);
  }
  else if (const UnaryOperator *UO = 
           dyn_cast<UnaryOperator>(Inc->IgnoreParenCasts())) {
    if (!UO || !UO->isIncrementDecrementOp()) 
      return true;

    const Expr *SubE = UO->getSubExpr()->IgnoreParenCasts();
    DRE = dyn_cast<DeclRefExpr>(SubE);
  }
  else {
    return true;
  }

  if (!DRE)
    return true;

  const ValueDecl *OrigDecl = DRE->getDecl();
  const VarDecl *VD = dyn_cast<VarDecl>(OrigDecl);
  if (!VD)
    return true;

  VarDeclToASESetMap ASEMap;
  ArraySubscriptExprCollectionVisitor 
    ASEVisitor(VD->getCanonicalDecl(), ConsumerInstance->CstArrayVars, ASEMap);

  ASEVisitor.TraverseStmt(Body);

  for (VarDeclToASESetMap::iterator I = ASEMap.begin(), 
       E = ASEMap.end(); I != E; ++I) {
    const VarDecl *CurrVD = (*I).first;
    ArraySubscriptExprSet *ESet = (*I).second;
    if (!ESet)
      continue;

    TransAssert(ConsumerInstance->CstArrayVars.count(CurrVD) &&
                "Cannot find CurrVD!");
    unsigned Sz = ConsumerInstance->CstArrayVars[CurrVD];
    for (unsigned Idx = 0; Idx < Sz; Idx++) {
      ConsumerInstance->ValidInstanceNum++;
      if (!ConsumerInstance->TheASESet && (ConsumerInstance->ValidInstanceNum ==
             ConsumerInstance->TransformationCounter)) {
        ConsumerInstance->TheASESet = new ArraySubscriptExprSet(*ESet);
        ConsumerInstance->CurrIdx = Idx;
      }
    }
  }
  return true;
}

const VarDecl *ArraySubscriptExprCollectionVisitor::getVarDeclFromExpr(
        const Expr *E)
{
  TransAssert(E && "NULL Expr!");
  const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E->IgnoreParenCasts());
  if (!DRE)
    return NULL;
  const ValueDecl *OrigDecl = DRE->getDecl();
  const VarDecl *VD = dyn_cast<VarDecl>(OrigDecl);
  if (!VD)
    return NULL;
  const VarDecl *CanonicalVD = VD->getCanonicalDecl();
  return CanonicalVD;
}

bool ArraySubscriptExprCollectionVisitor::VisitArraySubscriptExpr(
       ArraySubscriptExpr *ASE)
{
  const VarDecl *BaseVD = getVarDeclFromExpr(ASE->getBase());
  if (!BaseVD || !CstArrayVars[BaseVD])
    return true;

  const VarDecl *VD = getVarDeclFromExpr(ASE->getIdx());
  if (!VD || (VD != IdxVD))
    return true;
  
  ArraySubscriptExprSet *ESet = ASEMap[BaseVD];
  if (!ESet) {
    ESet = new ArraySubscriptExprSet();
    ASEMap[BaseVD] = ESet;
  }
  ESet->insert(ASE);
  return true;
}

ArraySubscriptExprCollectionVisitor::~ArraySubscriptExprCollectionVisitor(void)
{
  for (VarDeclToASESetMap::iterator I = ASEMap.begin(),
       E = ASEMap.end(); I != E; ++I) {
    ArraySubscriptExprSet *Set = (*I).second;
    if (Set)
      delete Set;
  }
}

void ReplaceArrayIndexVar::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new ReplaceArrayIndexVarCollectionVisitor(this);
}

void ReplaceArrayIndexVar::HandleTranslationUnit(ASTContext &Ctx)
{
  TransAssert(CollectionVisitor && "NULL CollectionVisitor!");
  CollectionVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);
  doRewrite();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void ReplaceArrayIndexVar::doRewrite(void)
{
  for (ArraySubscriptExprSet::iterator I = TheASESet->begin(),
       E = TheASESet->end(); I != E; ++I) {
    const ArraySubscriptExpr *ASE = (*I);
    const Expr *Idx = ASE->getIdx();
    TransAssert(Idx && "Bad Idx!");

    std::stringstream TmpSS;
    TmpSS << CurrIdx;
    RewriteHelper->replaceExpr(Idx, TmpSS.str());
  }
}

ReplaceArrayIndexVar::~ReplaceArrayIndexVar(void)
{
  delete CollectionVisitor;
  delete TheASESet;
}

