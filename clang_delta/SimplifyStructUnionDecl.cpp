//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "SimplifyStructUnionDecl.h"

#include "clang/AST/RecursiveASTVisitor.h"
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

void SimplifyStructUnionDecl::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
}

void SimplifyStructUnionDecl::HandleTopLevelDecl(DeclGroupRef DGR) 
{
  DeclGroupRef::iterator DI = DGR.begin();
  const RecordDecl *RD = dyn_cast<RecordDecl>(*DI);
  if (RD) {
    return addOneRecordDecl(RD, DGR);
  }

  VarDecl *VD = dyn_cast<VarDecl>(*DI);
  if (!VD)
    return;

  const Type *T = VD->getType().getTypePtr();
  const ArrayType *ArrayTy = dyn_cast<ArrayType>(T);
  if (ArrayTy)
    T = getArrayBaseElemType(ArrayTy);
  if (T->isPointerType())
    T = getBasePointerElemType(T);

  const RecordType *RT;
  if (T->isUnionType())
    RT = T->getAsUnionType();
  else if (T->isStructureType())
    RT = T->getAsStructureType();
  else
    return;

  RD = RT->getDecl();
  const Decl *CanonicalD = RD->getCanonicalDecl();
  void *DGRPointer = RecordDeclToDeclGroup[CanonicalD];
  if (!DGRPointer)
    return;

  ValidInstanceNum++;
  if (ValidInstanceNum == TransformationCounter) {
    TheDeclGroupRefs.push_back(DGRPointer);
    TheDeclGroupRefs.push_back(DGR.getAsOpaquePtr());
  }
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

SimplifyStructUnionDecl::~SimplifyStructUnionDecl(void)
{
  // Nothing to do
}

