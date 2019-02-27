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

#include "CombineGlobalVarDecl.h"

#include <algorithm>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Combine global variable declarations with the same type. \
Each iteration only does one combination, i.e., \
for the code below: \n\
  int a, b; \n\
  int c; \n\
  int d; \n\
We will need to invoke this transformation twice \
to achieve a complete combination. \
This pass only combines declarations with exactly \
the same type, e.g., it won't combine int *x and int y,\
although it's valid we can have int *x, y in a DeclGroup. \
Note that this transformation always tries to combine \
the rest of declarations with the very first one, \
so it is an unsound transformation and could result \
in compilation failures. \n";

static RegisterTransformation<CombineGlobalVarDecl>
         Trans("combine-global-var", DescriptionMsg);

void CombineGlobalVarDecl::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
}

bool CombineGlobalVarDecl::HandleTopLevelDecl(DeclGroupRef DGR) 
{
  DeclGroupRef::iterator DI = DGR.begin();
  VarDecl *VD = dyn_cast<VarDecl>(*DI);
  if (!VD || isInIncludedFile(VD))
    return true;
  SourceRange Range = VD->getSourceRange();
  if (Range.getBegin().isInvalid() || Range.getEnd().isInvalid())
    return true;

  const Type *T = VD->getType().getTypePtr();
  const Type *CanonicalT = Context->getCanonicalType(T);
  
  DeclGroupVector *DV;
  TypeToDeclMap::iterator TI = AllDeclGroups.find(CanonicalT);
  if (TI == AllDeclGroups.end()) {
    DV = new DeclGroupVector();
    AllDeclGroups[CanonicalT] = DV;
  }
  else {
    ValidInstanceNum++;
    DV = (*TI).second;

    if (ValidInstanceNum == TransformationCounter) {
      if (DV->size() >= 1) {
        void* DP1 = *(DV->begin());
        TheDeclGroupRefs.push_back(DP1);
        TheDeclGroupRefs.push_back(DGR.getAsOpaquePtr());
      }
    }
  }

  // Note that it's unnecessary to keep all encountered
  // DeclGroupRefs. We could choose a light way similar
  // to what we implemented in CombineLocalVarDecl.
  // I kept the code here because I feel we probably 
  // need more combinations, i.e., not only combine the
  // first DeclGroup with others, but we could combine
  // the second one and the third one. 
  DV->push_back(DGR.getAsOpaquePtr());
  return true;
}
 
void CombineGlobalVarDecl::HandleTranslationUnit(ASTContext &Ctx)
{
  if (QueryInstanceOnly) {
    return;
  }

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  doCombination();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void CombineGlobalVarDecl::doCombination(void)
{
  TransAssert(TheDeclGroupRefs.size() == 2);
  void *P2 = TheDeclGroupRefs.pop_back_val();
  void *P1 = TheDeclGroupRefs.pop_back_val();

  DeclGroupRef FirstDGR = DeclGroupRef::getFromOpaquePtr(P1);
  DeclGroupRef SecondDGR = DeclGroupRef::getFromOpaquePtr(P2);

  SourceLocation EndLoc = 
    RewriteHelper->getDeclGroupRefEndLoc(FirstDGR);

  std::string DStr;
  RewriteHelper->getDeclGroupStrAndRemove(SecondDGR, DStr);
  TheRewriter.InsertText(EndLoc, ", " + DStr, /*InsertAfter=*/false);
}

CombineGlobalVarDecl::~CombineGlobalVarDecl(void)
{
  for(TypeToDeclMap::iterator I = AllDeclGroups.begin(), 
      E = AllDeclGroups.end(); I != E; ++I) {
    delete (*I).second;
  }
}

