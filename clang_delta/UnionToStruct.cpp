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

#include "UnionToStruct.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;
using namespace llvm;

static const char *DescriptionMsg =
"Change a union declaration to a struct declaration. \
For a union declaration which has only scalar fields, i.e., fields \
with integer or pointer types, the transformation will set up an \
appropriate initializer for a variable which is declared with a union \
type under transformed: \n\
  * if originally a pointer field gets initialization value, then \
other pointer fields with the same type will get the same initialization \
value, and all other fields will be initialized to 0; \n\
  * if originally a integer field gets initialization value, then all \
other integer fields will be initialized to the same value, while all \
pointer fields will have 0. \n\
\n\
For other cases, all initializers will be dropped. (Note that this could \n\
bring in uninitialized local variables.) \n";

static RegisterTransformation<UnionToStruct>
         Trans("union-to-struct", DescriptionMsg);

class UnionToStructCollectionVisitor : public 
  RecursiveASTVisitor<UnionToStructCollectionVisitor> {

public:
  explicit UnionToStructCollectionVisitor(UnionToStruct *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitFunctionDecl(FunctionDecl *FD);

  bool VisitVarDecl(VarDecl *VD);

  bool VisitFieldDecl(FieldDecl *FD);

  bool VisitRecordDecl(RecordDecl *RD);

  bool VisitDeclStmt(DeclStmt *DS);

private:

  UnionToStruct *ConsumerInstance;
};

bool UnionToStructCollectionVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  const Type *T = FD->getResultType().getTypePtr();
  ConsumerInstance->addOneDeclarator(FD, T);
  return true;
}

bool UnionToStructCollectionVisitor::VisitVarDecl(VarDecl *VD)
{
  const Type *T = VD->getType().getTypePtr();
  ConsumerInstance->addOneDeclarator(VD, T);
  return true;
}

bool UnionToStructCollectionVisitor::VisitFieldDecl(FieldDecl *FD)
{
  const Type *T = FD->getType().getTypePtr();
  ConsumerInstance->addOneDeclarator(FD, T);
  return true;
}

bool UnionToStructCollectionVisitor::VisitRecordDecl(RecordDecl *RD)
{
  if (RD->isUnion())
    ConsumerInstance->addOneRecord(RD);

  return true;
}

bool UnionToStructCollectionVisitor::VisitDeclStmt(DeclStmt *DS)
{
  for (DeclStmt::decl_iterator I = DS->decl_begin(),
       E = DS->decl_end(); I != E; ++I) {
    if (VarDecl *CurrVD = dyn_cast<VarDecl>(*I))
      ConsumerInstance->VarToDeclStmt[CurrVD] = DS;
  }
  return true;
}

void UnionToStruct::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new UnionToStructCollectionVisitor(this);
}

bool UnionToStruct::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    if (VarDecl *VD = dyn_cast<VarDecl>(*I))
      VarToDeclGroup[VD] = D;

    CollectionVisitor->TraverseDecl(*I);
  }
  return true;
}
 
void UnionToStruct::HandleTranslationUnit(ASTContext &Ctx)
{
  doAnalysis();
  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  TransAssert(TheRecordDecl && "Cannot have NULL TheRecordDecl!");
  TransAssert(TheDeclaratorSet && "Cannot have NULL TheDeclaratorSet!");

  rewriteRecordDecls();
  rewriteDeclarators();

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

bool UnionToStruct::isValidRecordDecl(const RecordDecl *RD)
{
  if (!RD->isCompleteDefinition())
    return false;

  if (RD->isAnonymousStructOrUnion())
    return false;

  for (RecordDecl::field_iterator I = RD->field_begin(),
       E = RD->field_end(); I != E; ++I) {
    const FieldDecl *FD = (*I);
    const Type *FDTy = FD->getType().getTypePtr();
    if (!FDTy->isScalarType())
      return false;
  }
  return true;
}

void UnionToStruct::doAnalysis(void)
{
  for (RecordDeclToDeclaratorDeclMap::iterator I = RecordToDeclarator.begin(),
       E = RecordToDeclarator.end(); I != E; ++I) {
    ValidInstanceNum++;
    if (ValidInstanceNum == TransformationCounter) {
      // TheRecordDecl = ((*I).first)->getDefinition();
      TheRecordDecl = (*I).first;
      TheDeclaratorSet = (*I).second;
    }
  }
}

void UnionToStruct::rewriteOneRecordDecl(const RecordDecl *RD)
{
  TransAssert(RD && "NULL RecordDecl!");
  RewriteHelper->replaceUnionWithStruct(RD);
}

void UnionToStruct::rewriteRecordDecls(void)
{
  const RecordDecl *RD = 
    dyn_cast<RecordDecl>(TheRecordDecl->getCanonicalDecl());
  TransAssert(RD && "NULL RecordDecl!");
  for(RecordDecl::redecl_iterator I = RD->redecls_begin(),
      E = RD->redecls_end(); I != E; ++I) {
    rewriteOneRecordDecl(dyn_cast<RecordDecl>(*I));
  }
}

void UnionToStruct::rewriteOneFieldDecl(const FieldDecl *FD)
{
  const DeclContext *Ctx = TheRecordDecl->getLexicalParent();
  // Skip the case where we have:
  // struct {
  //  union U {
  //    int f;
  //  } f1, f2;
  // };
  // union U is handled by rewriteOneRecordDecl;
  if (!dyn_cast<RecordDecl>(Ctx))
    RewriteHelper->replaceUnionWithStruct(FD);
}

void UnionToStruct::getInitStrWithPointerType(const Expr *Exp, std::string &Str)
{
  std::string ExprStr("");
  RewriteHelper->getExprString(Exp, ExprStr);

  RecordDecl::field_iterator I = TheRecordDecl->field_begin();
  RecordDecl::field_iterator E = TheRecordDecl->field_end();
  TransAssert((I != E) && "Empty RecordDecl!");
  Str = ExprStr;
  ++I;

  if (I == E)
    return;

  QualType ETy = Exp->getType().getCanonicalType();
  for (; I != E; ++I) {
    Str += ",";
    const FieldDecl *FD = (*I);
    QualType FTy = FD->getType().getCanonicalType();
    if (ETy == FTy)
      Str += ExprStr;
    else
      Str += "0";
  }
}

void UnionToStruct::getInitStrWithNonPointerType(const Expr *Exp, 
                                                 std::string &Str)
{
  std::string ExprStr("");
  RewriteHelper->getExprString(Exp, ExprStr);

  RecordDecl::field_iterator I = TheRecordDecl->field_begin();
  RecordDecl::field_iterator E = TheRecordDecl->field_end();
  TransAssert((I != E) && "Empty RecordDecl!");
  Str = ExprStr;
  ++I;

  if (I == E)
    return;

  for (; I != E; ++I) {
    Str += ",";
    const FieldDecl *FD = (*I);
    const Type *FTy = FD->getType().getTypePtr();
    if (FTy->isPointerType())
      Str += "0";
    else
      Str += ExprStr;
  }
}

bool UnionToStruct::isTheFirstDecl(const VarDecl *VD)
{
  // always return false if VD is declared in a LinkageSpecDecl,
  // because the first decl should be implicitly declared union record,
  // which is handled by rewriteOneRecordDecl
  const DeclContext *Ctx = VD->getDeclContext();
  if (dyn_cast<LinkageSpecDecl>(Ctx)) {
    return false;
  }

  DeclGroupRef DGR;
  if (const DeclStmt *DS = VarToDeclStmt[VD])
    DGR = DS->getDeclGroup();
  else
    DGR = VarToDeclGroup[VD];

  TransAssert(!DGR.isNull() && "Bad DeclRefGroup!");

  if (DGR.isSingleDecl())
    return true;

  DeclGroupRef::const_iterator I = DGR.begin();
  const VarDecl *FirstVD = dyn_cast<VarDecl>(*I);
  if (!FirstVD)
    return false;

  return (VD == FirstVD);
}

void UnionToStruct::rewriteOneVarDecl(const VarDecl *VD)
{
  if (dyn_cast<ParmVarDecl>(VD)) {
    RewriteHelper->replaceUnionWithStruct(VD);
    return; 
  }

  // If the first decl is RecordDecl, it will be handled
  // by rewriteOneRecordDecl.
  if (isTheFirstDecl(VD))
    RewriteHelper->replaceUnionWithStruct(VD);

  const Type *VDTy = VD->getType().getTypePtr();
  if (!VD->hasInit())
    return;

  if (const ArrayType *ArrayTy = dyn_cast<ArrayType>(VDTy)) {
    VDTy = getArrayBaseElemType(ArrayTy);
    // We remove the initializer for an array of unions
    if (!VDTy->isUnionType()) {
      return;
    }

    const Expr *IE = VD->getInit();
    const InitListExpr *ILE = dyn_cast<InitListExpr>(IE);
    // handle a special case where we have code like this:
    //   union U a[][1] = {};
    // In this case, it's safe to keep the empty initializer
    if (!ILE->getNumInits())
      return;

    RewriteHelper->removeVarInitExpr(VD);
    return;
  }

  if (!VDTy->isUnionType()) {
    return;
  }
  
  const Expr *IE = VD->getInit();
  // Looks like we are safe to skip this
  // e.g. 
  // union U { int x; };
  // void foo() { U y; }
  if (dyn_cast<CXXConstructExpr>(IE))
    return;
  if (!isValidRecordDecl(TheRecordDecl)) {
    RewriteHelper->removeVarInitExpr(VD);
    return;
  }

  const InitListExpr *ILE = dyn_cast<InitListExpr>(IE);
  TransAssert(ILE && "Bad InitListExpr!");

  if (ILE->getNumInits() != 1) {
    RewriteHelper->removeVarInitExpr(VD);
    return;
  }

  const Expr *FirstE = ILE->getInit(0);
  const Type *ExprTy = FirstE->getType().getTypePtr();
  std::string NewInitStr;

  if (ExprTy->isPointerType()) {
    getInitStrWithPointerType(FirstE, NewInitStr);
  }
  else {
    getInitStrWithNonPointerType(FirstE, NewInitStr);
  }

  RewriteHelper->replaceExpr(FirstE, NewInitStr);
}

void UnionToStruct::rewriteOneFunctionDecl(const FunctionDecl *FD)
{
  RewriteHelper->replaceUnionWithStruct(FD);
}

void UnionToStruct::rewriteDeclarators(void)
{
  for (DeclaratorDeclSet::const_iterator I = TheDeclaratorSet->begin(),
       E = TheDeclaratorSet->end(); I != E; ++I) {
    if (const FieldDecl *FD = dyn_cast<FieldDecl>(*I)) {
      rewriteOneFieldDecl(FD);
      continue;
    }

    if (const FunctionDecl *FunD = dyn_cast<FunctionDecl>(*I)) {
      rewriteOneFunctionDecl(FunD);
      continue;
    }

    const VarDecl *VD = dyn_cast<VarDecl>(*I);
    TransAssert(VD && "Invalid Declarator kind!");
    rewriteOneVarDecl(VD);
  }
}

void UnionToStruct::addOneDeclarator(const DeclaratorDecl *DD, const Type *T)
{
  if (const ArrayType *ArrayTy = dyn_cast<ArrayType>(T))
    T = getArrayBaseElemType(ArrayTy);

  if (T->isPointerType())
    T = getBasePointerElemType(T);

  if (!T->isUnionType())
    return;

  if (T->getTypeClass() == Type::Typedef)
    return;

  const RecordType *RDTy = T->getAsUnionType();
  TransAssert(RDTy && "Bad RecordType!");
  if (RDTy->isIncompleteType())
    return;

  const RecordDecl *RD = RDTy->getDecl(); 
  const RecordDecl *CanonicalRD = 
    dyn_cast<RecordDecl>(RD->getCanonicalDecl());
  TransAssert(CanonicalRD && "NULL CanonicalRD!");
  DeclaratorDeclSet *DDSet = RecordToDeclarator[CanonicalRD];
  TransAssert(DDSet && "Cannot find VarDeclSet for a given RecordDecl!");
  DDSet->insert(DD);
}

void UnionToStruct::addOneRecord(const RecordDecl *RD)
{
  const RecordDecl *CanonicalRD = 
    dyn_cast<RecordDecl>(RD->getCanonicalDecl());
  TransAssert(CanonicalRD && "NULL CanonicalRD!");
  if (RecordToDeclarator[CanonicalRD])
    return;

  DeclaratorDeclSet *DDSet = new DeclaratorDeclSet();
  TransAssert(DDSet && "Member allocation failure!");
  RecordToDeclarator[CanonicalRD] = DDSet;
}

UnionToStruct::~UnionToStruct(void)
{
  delete CollectionVisitor;

  for (RecordDeclToDeclaratorDeclMap::iterator I = RecordToDeclarator.begin(),
       E = RecordToDeclarator.end(); I != E; ++I) {
    delete (*I).second;
  }
}

