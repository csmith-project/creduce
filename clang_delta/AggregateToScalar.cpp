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

#include "AggregateToScalar.h"

#include <sstream>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Replace accesses to an aggregate member with \
a corresponding scalar variable. \
In more detail, the transformation creates \
a scalar variable for an aggregate access, \
assigns the initial value of the aggregate member to \
the scalar, and substitutes all accesses to the same aggregate \
member with the accesses to the corresponding scalar variable. \
(Note that this transformation is unsound).\n";

static RegisterTransformation<AggregateToScalar>
         Trans("aggregate-to-scalar", DescriptionMsg);

class ATSCollectionVisitor : public RecursiveASTVisitor<ATSCollectionVisitor> {

public:
  explicit ATSCollectionVisitor(AggregateToScalar *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitMemberExpr(MemberExpr *ME);

  bool VisitArraySubscriptExpr(ArraySubscriptExpr *ASE);

  bool VisitDeclStmt(DeclStmt *DS);

private:

  AggregateToScalar *ConsumerInstance;

};

bool ATSCollectionVisitor::VisitMemberExpr(MemberExpr *ME)
{
  if (ConsumerInstance->isInIncludedFile(ME))
    return true;

  ValueDecl *OrigDecl = ME->getMemberDecl();
  FieldDecl *FD = dyn_cast<FieldDecl>(OrigDecl);

  if (!FD) {
    // in C++, getMemberDecl returns a CXXMethodDecl.
    if (TransformationManager::isCXXLangOpt())
      return true;
    TransAssert(0 && "Bad FD!\n");
  }

  const Type *T = FD->getType().getTypePtr();
  if (!T->isScalarType())
    return true;

  RecordDecl *RD = FD->getParent();
  TransAssert(RD && "NULL RecordDecl!");
  if (!RD->isStruct() && !RD->isUnion())
    return true;

  ConsumerInstance->addOneExpr(ME);
  return true;
}

bool ATSCollectionVisitor::VisitArraySubscriptExpr(ArraySubscriptExpr *ASE)
{
  if (ConsumerInstance->isInIncludedFile(ASE))
    return true;

  const Type *T = ASE->getType().getTypePtr();
  if (!T->isScalarType())
    return true;

  ConsumerInstance->addOneExpr(ASE);
  return true;
}

bool ATSCollectionVisitor::VisitDeclStmt(DeclStmt *DS)
{
  for (DeclStmt::decl_iterator I = DS->decl_begin(),
       E = DS->decl_end(); I != E; ++I) {
    VarDecl *CurrDecl = dyn_cast<VarDecl>(*I);
    if (CurrDecl)
      ConsumerInstance->VarDeclToDeclStmtMap[CurrDecl] = DS;
  }
  return true;
}

void AggregateToScalar::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  AggregateAccessVisitor = new ATSCollectionVisitor(this);
}

bool AggregateToScalar::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    AggregateAccessVisitor->TraverseDecl(*I);
  }
  return true;
}
 
void AggregateToScalar::HandleTranslationUnit(ASTContext &Ctx)
{
  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  TransAssert(TheVarDecl && "NULL TheVarDecl!");
  TransAssert(TheIdx && "NULL TheIdx!");

  doRewrite();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

bool AggregateToScalar::addTmpVar(const Expr *RefE,
                                  const std::string &VarName,
                                  const std::string *InitStr)
{
  std::string VarStr(VarName);
  QualType QT = RefE->getType();
  QT.getAsStringInternal(VarStr, Context->getPrintingPolicy());

  if (InitStr) {
    VarStr += " = ";
    VarStr += (*InitStr);
  }
  VarStr += ";";
   
  if (TheVarDecl->getStorageClass() == SC_Static)
    VarStr = "static " + VarStr; 

  TransAssert(!dyn_cast<ParmVarDecl>(TheVarDecl) && 
              "We don't handle ParmVarDecl!");
  if (TheVarDecl->isLocalVarDecl()) {
    DeclStmt *TheDeclStmt = VarDeclToDeclStmtMap[TheVarDecl];
    TransAssert(TheDeclStmt && "NULL TheDeclStmt");
    return RewriteHelper->addStringAfterStmt(TheDeclStmt, VarStr);
  }
  else {
    return RewriteHelper->addStringAfterVarDecl(TheVarDecl, VarStr);
  }
}

void AggregateToScalar::createNewVarName(std::string &VarName)
{
  std::stringstream SS;
  SS << TheVarDecl->getNameAsString();
  for (IndexVector::const_reverse_iterator I = TheIdx->rbegin(),
       E = TheIdx->rend(); I != E; ++I) {
    SS << "_" << (*I);
  }

  VarName = SS.str();
}

bool AggregateToScalar::createNewVar(const Expr *RefE, std::string &VarName)
{
  const Type *VarT = TheVarDecl->getType().getTypePtr();
  TransAssert((VarT->isStructureType() || VarT->isUnionType() 
               || VarT->isArrayType() || VarT->isPointerType())
              && "Non-valid var type!");

  createNewVarName(VarName);

  if (VarT->isPointerType())
    return addTmpVar(RefE, VarName, NULL);

  const Expr *IE = TheVarDecl->getAnyInitializer();
  if (!IE)
    return addTmpVar(RefE, VarName, NULL);

  const InitListExpr *ILE = dyn_cast<InitListExpr>(IE);
  if (!ILE) {
    return addTmpVar(RefE, VarName, NULL);
  }

  const Expr *InitE = getInitExprByIndex(*TheIdx, ILE);
  // We might have incomplete initialization list
  if (!InitE)
    return addTmpVar(RefE, VarName, NULL);

  std::string InitStr;
  if (InitE->getLocStart().isInvalid()) {
    const Type *ET = InitE->getType().getTypePtr();
    if (ET->isIntegerType() || ET->isPointerType())
      InitStr = "0"; 
    else
      return addTmpVar(RefE, VarName, NULL);
  }
  else {
    RewriteHelper->getExprString(InitE, InitStr);
  }
  return addTmpVar(RefE, VarName, &InitStr);
}

void AggregateToScalar::doRewrite(void)
{
  ExprSet *TheExprSet = ValidExprs[TheIdx];
  TransAssert(!TheExprSet->empty() && "TheExprSet cannot be empty!");

  ExprSet::iterator I = TheExprSet->begin(), E = TheExprSet->end();
  std::string VarName("");
  createNewVar(*I, VarName);

  for (; I != E; ++I) {
    RewriteHelper->replaceExpr((*I), VarName);
  }
}

void AggregateToScalar::addOneIdx(const Expr *E,
                                  const VarDecl *VD,
                                  IdxVectorSet *IdxSet,
                                  IndexVector *Idx)
{
  IdxSet->insert(Idx);
  ExprSet *ESet = new ExprSet();
  ValidExprs[Idx] = ESet;
  ESet->insert(E);
  ValidInstanceNum++;
  if (ValidInstanceNum == TransformationCounter) {
    TheVarDecl = VD;
    TheIdx = Idx;
  }
}

bool AggregateToScalar::isStructuralEqualVectors(IndexVector *IV1, 
                                                 IndexVector *IV2)
{
  unsigned int Sz = IV1->size();
  if (Sz != IV2->size())
    return false;

  for (unsigned int I = 0; I < Sz; ++I) {
    unsigned int Idx1 = (*IV1)[I];
    unsigned int Idx2 = (*IV2)[I];
    if (Idx1 != Idx2)
      return false;
  }
  return true;
}

void AggregateToScalar::addOneExpr(const Expr *Exp)
{
  IndexVector *Idx = new IndexVector();
  const Expr *BaseE = getBaseExprAndIdxs(Exp, *Idx);
  TransAssert(BaseE && "Invalid Base Expr for ArraySubscriptExpr!");

  const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(BaseE);
  if (!DRE) {
    delete Idx;
    return;
  }

  const ValueDecl *OrigDecl = DRE->getDecl();
  const VarDecl *VD = dyn_cast<VarDecl>(OrigDecl);
  if (!VD) {
    delete Idx;
    return;
  }

  if (dyn_cast<ParmVarDecl>(VD)) {
    delete Idx;
    return;
  }

  const VarDecl *CanonicalDecl = VD->getCanonicalDecl();
  IdxVectorSet *IdxSet = ValidVars[CanonicalDecl];
  if (!IdxSet) {
    IdxSet = new IdxVectorSet();
    ValidVars[VD] = IdxSet;
    addOneIdx(Exp, VD, IdxSet, Idx);
    return;
  }

  IndexVector *CachedIdx = NULL;
  for (IdxVectorSet::iterator I = IdxSet->begin(), E = IdxSet->end();
       I != E; ++I) {
    if (isStructuralEqualVectors(*I, Idx)) {
      CachedIdx = (*I);
      break;
    }
  }

  if (!CachedIdx) {
    addOneIdx(Exp, VD, IdxSet, Idx);
    return;
  }

  ExprSet *CachedESet = ValidExprs[CachedIdx];
  TransAssert(CachedESet && "NULL CachedESet!");
  CachedESet->insert(Exp);
  delete Idx;
}

AggregateToScalar::~AggregateToScalar(void)
{
  delete AggregateAccessVisitor;

  for (VarToIdx::iterator I = ValidVars.begin(),
       E = ValidVars.end(); I != E; ++I) {
    delete (*I).second;
  }

  for (IdxToExpr::iterator I = ValidExprs.begin(),
       E = ValidExprs.end(); I != E; ++I) {
    delete (*I).first;
    delete (*I).second;
  }
}

