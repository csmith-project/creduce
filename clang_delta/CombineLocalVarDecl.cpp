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

#include "CombineLocalVarDecl.h"

#include <algorithm>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Combine local variable declarations with the same type. \
This transformation records the location of the first \
local variable declaration, and tries to combine other \
declarations in the same DeclContext as the first one. \
For example, for the following cod3: \n\
  int foo(void) { \n\
    int x; \n\
    int y; \n\
    if (...) { \n\
      int z; \n\
    } \n\
  } \n\
We will only combine x and y, and won't touch z, \
because z is not in the same DeclContext as x and y. \
Note that this transformation is unsound for the same \
reason as combine-global-var. \n";

static RegisterTransformation<CombineLocalVarDecl>
         Trans("combine-local-var", DescriptionMsg);

class CombLocalVarCollectionVisitor : 
  public RecursiveASTVisitor<CombLocalVarCollectionVisitor> {

public:

  explicit CombLocalVarCollectionVisitor(CombineLocalVarDecl *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitCompoundStmt(CompoundStmt *DS);

private:

  const Type *getTypeFromDeclStmt(DeclStmt *DS);

  CombineLocalVarDecl *ConsumerInstance;

};

const Type *CombLocalVarCollectionVisitor::getTypeFromDeclStmt(DeclStmt *DS)
{
  Decl *D;
  if (DS->isSingleDecl()) {
    D = DS->getSingleDecl();
  }
  else {
    DeclGroupRef DGR = DS->getDeclGroup();
    DeclGroupRef::iterator I = DGR.begin();
    D = (*I);
  }

  VarDecl *VD = dyn_cast<VarDecl>(D);
  if (!VD)
    return NULL;
  
  return VD->getType().getTypePtr();
}

bool CombLocalVarCollectionVisitor::VisitCompoundStmt(CompoundStmt *CS)
{
  if (ConsumerInstance->isInIncludedFile(CS))
    return true;

  ConsumerInstance->DeclStmts.clear();

  for (CompoundStmt::body_iterator I = CS->body_begin(), 
       E = CS->body_end(); I != E; ++I) {

    DeclStmt *DS = dyn_cast<DeclStmt>(*I);
    if (!DS)
      continue;
    const Type *T = getTypeFromDeclStmt(DS);
    if (!T)
      continue;

    const Type *CanonicalT = 
      ConsumerInstance->Context->getCanonicalType(T);
    llvm::DenseMap<const Type *, DeclStmt *>::iterator TI = 
      ConsumerInstance->DeclStmts.find(CanonicalT);
    if (TI == ConsumerInstance->DeclStmts.end()) {
      ConsumerInstance->DeclStmts[CanonicalT] = DS;
    }
    else {
      ConsumerInstance->ValidInstanceNum++;
      if (ConsumerInstance->ValidInstanceNum != 
          ConsumerInstance->TransformationCounter)
        continue;
      ConsumerInstance->TheDeclStmts.push_back((*TI).second);
      ConsumerInstance->TheDeclStmts.push_back(DS);  
    }
  }

  return true;
}

void CombineLocalVarDecl::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new CombLocalVarCollectionVisitor(this);
}

bool CombineLocalVarDecl::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    CollectionVisitor->TraverseDecl(*I);
  }
  return true;
}
 
void CombineLocalVarDecl::HandleTranslationUnit(ASTContext &Ctx)
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

void CombineLocalVarDecl::doCombination(void)
{
  TransAssert(TheDeclStmts.size() == 2);
  DeclStmt *DS2 = TheDeclStmts.pop_back_val();
  DeclStmt *DS1 = TheDeclStmts.pop_back_val();

  SourceLocation EndLoc = RewriteHelper->getDeclStmtEndLoc(DS1);

  std::string DStr;
  RewriteHelper->getDeclStmtStrAndRemove(DS2, DStr);
  TheRewriter.InsertText(EndLoc, ", " + DStr, /*InsertAfter=*/false);
}

CombineLocalVarDecl::~CombineLocalVarDecl(void)
{
  delete CollectionVisitor;
}

