//===----------------------------------------------------------------------===//
//
// Copyright (c) 2016, 2017 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "ReplaceArrayAccessWithIndex.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

#include <iostream>

using namespace clang;


static const char *Description =
  "Replace array accesses with the index expression.";

static RegisterTransformation<ReplaceArrayAccessWithIndex>
Trans("replace-array-access-with-index", Description);

class ReplaceArrayAccessWithIndex::IndexCollector
  : public RecursiveASTVisitor<ReplaceArrayAccessWithIndex::IndexCollector>
{
public:
  explicit IndexCollector(ReplaceArrayAccessWithIndex *instance);
  bool VisitArraySubscriptExpr(ArraySubscriptExpr *ASE);

private:
  const VarDecl *getVarDeclFromExpr(const Expr *E);
  ReplaceArrayAccessWithIndex *ConsumerInstance;
};

ReplaceArrayAccessWithIndex::IndexCollector::IndexCollector(
  ReplaceArrayAccessWithIndex *instance)
  : ConsumerInstance(instance)
{
  // No further initialization needed.
}

bool ReplaceArrayAccessWithIndex::IndexCollector::VisitArraySubscriptExpr(
  ArraySubscriptExpr *ASE)
{
  // Skip expressions in included files.
  if (ConsumerInstance->isInIncludedFile(ASE))
    return true;

  const VarDecl *BaseVD = getVarDeclFromExpr(ASE->getBase());

  if (!BaseVD)
    return true;

  ArrayType const *ArrayTy = dyn_cast<ArrayType>(BaseVD->getType().getTypePtr());
  // Only apply the transformation to one-dimensional arrays of scalars.
  if (!ArrayTy || !ArrayTy->getElementType().getTypePtr()->isScalarType())
    return true;

  ConsumerInstance->ASEs.push_back(ASE);
  ConsumerInstance->ValidInstanceNum++;

  return true;
}

const VarDecl *ReplaceArrayAccessWithIndex::IndexCollector::getVarDeclFromExpr(
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



ReplaceArrayAccessWithIndex::~ReplaceArrayAccessWithIndex(void)
{
  delete Collector;
}


void ReplaceArrayAccessWithIndex::Initialize(clang::ASTContext &context)
{
  Transformation::Initialize(context);
  Collector = new IndexCollector(this);
}

void ReplaceArrayAccessWithIndex::HandleTranslationUnit(clang::ASTContext &Ctx)
{
  TransAssert(Collector && "NULL Collector");
  Collector->TraverseDecl(Ctx.getTranslationUnitDecl());

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

void ReplaceArrayAccessWithIndex::doRewrite(void)
{
  ArraySubscriptExpr const *ASE = ASEs[TransformationCounter - 1];
  Expr const *Idx = ASE->getIdx();

  TransAssert(Idx && "Bad Idx!");

  std::string IdxStr;
  RewriteHelper->getExprString(Idx, IdxStr);

  QualType ASEType = ASE->getType().getCanonicalType();
  QualType IdxType = Idx->getType().getCanonicalType();

  if (ASEType != IdxType) {
    IdxStr = std::string("(") + ASEType.getAsString() + std::string(")")+
      std::string("(") + IdxStr + std::string(")");
  }

  RewriteHelper->replaceExpr(ASE, IdxStr);
}
