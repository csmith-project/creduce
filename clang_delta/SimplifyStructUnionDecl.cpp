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

#include "SimplifyStructUnionDecl.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;
using namespace llvm;

static const char *DescriptionMsg =
"This pass combines the declaration of global vars declared \
as struct/union with the corresponding struct/union \
declaration. \n";

static RegisterTransformation<SimplifyStructUnionDecl>
         Trans("simplify-struct-union-decl", DescriptionMsg);

class SimplifyStructUnionDeclVisitor : public 
  RecursiveASTVisitor<SimplifyStructUnionDeclVisitor> {

public:
  explicit SimplifyStructUnionDeclVisitor(SimplifyStructUnionDecl *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitFunctionDecl(FunctionDecl *FD);

  bool VisitVarDecl(VarDecl *VD);

  bool VisitFieldDecl(FieldDecl *FD);

private:

  SimplifyStructUnionDecl *ConsumerInstance;
};

bool SimplifyStructUnionDeclVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  const Type *T = FD->getResultType().getTypePtr();
  return ConsumerInstance->handleOneDeclarator(T);
}

bool SimplifyStructUnionDeclVisitor::VisitVarDecl(VarDecl *VD)
{
  if (ConsumerInstance->CombinedVars.count(VD))
    return true;

  const Type *T = VD->getType().getTypePtr();
  return ConsumerInstance->handleOneDeclarator(T);
}

bool SimplifyStructUnionDeclVisitor::VisitFieldDecl(FieldDecl *FD)
{
  const Type *T = FD->getType().getTypePtr();
  return ConsumerInstance->handleOneDeclarator(T);
}

void SimplifyStructUnionDecl::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  AnalysisVisitor = new SimplifyStructUnionDeclVisitor(this);
}

bool SimplifyStructUnionDecl::HandleTopLevelDecl(DeclGroupRef DGR) 
{
  DeclGroupRef::iterator DI = DGR.begin();
  const RecordDecl *RD = dyn_cast<RecordDecl>(*DI);
  if (RD) {
    addOneRecordDecl(RD, DGR);
    return true;
  }

  VarDecl *VD = dyn_cast<VarDecl>(*DI);
  if (!VD)
    return true;

  const Type *T = VD->getType().getTypePtr();

  RD = getBaseRecordDecl(T);
  if (!RD)
    return true;

  const Decl *CanonicalD = RD->getCanonicalDecl();
  void *DGRPointer = RecordDeclToDeclGroup[CanonicalD];
  if (!DGRPointer)
    return true;

  ValidInstanceNum++;
  if (ValidInstanceNum != TransformationCounter)
    return true;

  TheRecordDecl = dyn_cast<RecordDecl>(CanonicalD);
  TheDeclGroupRefs.push_back(DGRPointer);
  TheDeclGroupRefs.push_back(DGR.getAsOpaquePtr());

  for (DeclGroupRef::iterator I = DGR.begin(), E = DGR.end(); I != E; ++I) {
    VarDecl *VD = dyn_cast<VarDecl>(*I);
    TransAssert(VD && "Bad VarDecl!");
    CombinedVars.insert(VD);
  }

  DeclGroupRef DefDGR = DeclGroupRef::getFromOpaquePtr(DGRPointer);
  for (DeclGroupRef::iterator I = DefDGR.begin(), 
       E = DefDGR.end(); I != E; ++I) {
    VarDecl *VD = dyn_cast<VarDecl>(*I);
    if (VD)
      CombinedVars.insert(VD);
  }
  return true;
}

void SimplifyStructUnionDecl::HandleTranslationUnit(ASTContext &Ctx)
{
  if (QueryInstanceOnly) {
    return;
  }

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  TransAssert(AnalysisVisitor && "NULL AnalysisVisitor!");
  TransAssert(TheRecordDecl && "NULL RecordDecl!");
  AnalysisVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
  doCombination();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void SimplifyStructUnionDecl::doCombination(void)
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

  // it's a single RecordDecl
  if (FirstDGR.isSingleDecl())
    TheRewriter.InsertText(EndLoc, DStr, /*InsertAfter=*/false);
  else
    TheRewriter.InsertText(EndLoc, ", " + DStr, /*InsertAfter=*/false);

  if (isSafeToRemoveName()) {
    SourceLocation NameLocStart = TheRecordDecl->getLocation();
    std::string Name = TheRecordDecl->getNameAsString();
    TheRewriter.RemoveText(NameLocStart, Name.size());
  }
}

bool SimplifyStructUnionDecl::isSafeToRemoveName(void)
{
  if (!SafeToRemoveName)
    return false;

  const RecordDecl *RD = 
    dyn_cast<RecordDecl>(TheRecordDecl->getCanonicalDecl());
  RecordDecl::redecl_iterator I = RD->redecls_begin();
  RecordDecl::redecl_iterator E = RD->redecls_end();
  ++I;
  return (I == E);
}

void SimplifyStructUnionDecl::addOneRecordDecl(const RecordDecl *RD,
                                               DeclGroupRef DGR)
{
  const Decl *CanonicalD = RD->getCanonicalDecl();

  const RecordDecl *RDDef = RD->getDefinition();
  if (!RDDef || (RD != RDDef))
    return;

  if (!RecordDeclToDeclGroup[CanonicalD])
    RecordDeclToDeclGroup[CanonicalD] = (DGR.getAsOpaquePtr());
}

const RecordDecl *SimplifyStructUnionDecl::getBaseRecordDecl(const Type *T)
{
  const ArrayType *ArrayTy = dyn_cast<ArrayType>(T);
  if (ArrayTy)
    T = getArrayBaseElemType(ArrayTy);
  if (T->isPointerType())
    T = getBasePointerElemType(T);

  const RecordType *RT = NULL;
  if (T->isUnionType())
    RT = T->getAsUnionType();
  else if (T->isStructureType())
    RT = T->getAsStructureType();
  else
    return NULL;

  return RT->getDecl();
}

bool SimplifyStructUnionDecl::handleOneDeclarator(const Type *Ty)
{
  const RecordDecl *RD = getBaseRecordDecl(Ty);
  if (!RD)
    return true;

  const RecordDecl *CanonicalRD = dyn_cast<RecordDecl>(RD->getCanonicalDecl());
  if (CanonicalRD == TheRecordDecl) {
    SafeToRemoveName = false;
  }
  return SafeToRemoveName;
}

SimplifyStructUnionDecl::~SimplifyStructUnionDecl(void)
{
  delete AnalysisVisitor;
}

