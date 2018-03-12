//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2014, 2015, 2016, 2017 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "ReducePointerLevel.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Reduce pointer indirect level for a global/local variable. \
All valid variables are sorted by their indirect levels. \
The pass will ensure to first choose a valid variable \
with the largest indirect level. This approach could \
reduce the complexity of our implementation, because \
we don't have to consider the case where the chosen variable \
with the largest indirect level would be address-taken. \
Variables at non-largest-indirect-level are ineligible \
if they: \n\
  * being address-taken \n\
  * OR being used as LHS in any pointer form, e.g., \n\
    p, *p(assume *p is of pointer type), \n\
    while the RHS is NOT a UnaryOperator. \n";

static RegisterTransformation<ReducePointerLevel>
         Trans("reduce-pointer-level", DescriptionMsg);

class PointerLevelCollectionVisitor : public
  RecursiveASTVisitor<PointerLevelCollectionVisitor> {

public:

  explicit PointerLevelCollectionVisitor(ReducePointerLevel *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitDeclaratorDecl(DeclaratorDecl *DD);

  bool VisitUnaryOperator(UnaryOperator *UO);

  bool VisitBinaryOperator(BinaryOperator *BO);

private:

  ReducePointerLevel *ConsumerInstance;

  int getPointerIndirectLevel(const Type *Ty);

  bool isVAArgField(DeclaratorDecl *DD);

};

class PointerLevelRewriteVisitor : public
  RecursiveASTVisitor<PointerLevelRewriteVisitor> {

public:

  explicit PointerLevelRewriteVisitor(ReducePointerLevel *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitVarDecl(VarDecl *VD);

  bool VisitFieldDecl(FieldDecl *FD);

  bool VisitUnaryOperator(UnaryOperator *UO);

  bool VisitBinaryOperator(BinaryOperator *BO);

  bool VisitDeclRefExpr(DeclRefExpr *DRE);

  bool VisitMemberExpr(MemberExpr *ME);

  bool VisitCXXDependentScopeMemberExpr(CXXDependentScopeMemberExpr *ME);

private:

  ReducePointerLevel *ConsumerInstance;

};

int PointerLevelCollectionVisitor::getPointerIndirectLevel(const Type *Ty)
{
  int IndirectLevel = 0;
  QualType QT = Ty->getPointeeType();
  while (!QT.isNull()) {
    IndirectLevel++;
    QT = QT.getTypePtr()->getPointeeType();
  }
  return IndirectLevel;
}

// Any better way to ginore these two fields
// coming from __builtin_va_arg ?
bool PointerLevelCollectionVisitor::isVAArgField(DeclaratorDecl *DD)
{
  std::string Name = DD->getNameAsString();
  return (!Name.compare("reg_save_area") ||
          !Name.compare("overflow_arg_area"));
}

// I skipped IndirectFieldDecl for now
bool PointerLevelCollectionVisitor::VisitDeclaratorDecl(DeclaratorDecl *DD)
{
  if (ConsumerInstance->isInIncludedFile(DD) || isVAArgField(DD))
    return true;

  // Only consider FieldDecl and VarDecl
  Decl::Kind K = DD->getKind();
  if ((K != Decl::Field) && (K != Decl::Var))
    return true;

  const Type *Ty = DD->getType().getTypePtr();
  // omit SubstTemplateTypeParmType for now, .e.g.,
  //  template <typename T>
  //  void foo(T last) { T t; }
  //  void bar(void) { int a; foo(&a); }
  // in this example, T is inferred as int *,
  // but we cannot do anything useful for "T t"
  if (dyn_cast<SubstTemplateTypeParmType>(Ty))
    return true;

  if (const ArrayType *ArrayTy = dyn_cast<ArrayType>(Ty))
    Ty = ConsumerInstance->getArrayBaseElemType(ArrayTy);
  if (!Ty->isPointerType() || Ty->isVoidPointerType())
    return true;

  const Type *PointeeTy = Ty->getPointeeType().getTypePtr();
  if (PointeeTy->isIncompleteType() ||
      ConsumerInstance->isPointerToSelf(PointeeTy, DD))
    return true;

  DeclaratorDecl *CanonicalDD = dyn_cast<DeclaratorDecl>(DD->getCanonicalDecl());
  TransAssert(CanonicalDD && "Bad DeclaratorDecl!");
  if (ConsumerInstance->VisitedDecls.count(CanonicalDD))
    return true;

  ConsumerInstance->ValidDecls.insert(CanonicalDD);
  ConsumerInstance->VisitedDecls.insert(CanonicalDD);
  int IndirectLevel = getPointerIndirectLevel(Ty);
  TransAssert((IndirectLevel > 0) && "Bad indirect level!");
  if (IndirectLevel > ConsumerInstance->MaxIndirectLevel)
    ConsumerInstance->MaxIndirectLevel = IndirectLevel;

  ConsumerInstance->addOneDecl(CanonicalDD, IndirectLevel);
  return true;
}

bool PointerLevelCollectionVisitor::VisitUnaryOperator(UnaryOperator *UO)
{
  UnaryOperator::Opcode Op = UO->getOpcode();
  if (Op == UO_Deref) {
    ConsumerInstance->checkPrefixAndPostfix(UO);
    return true;
  }

  if (Op != UO_AddrOf)
    return true;

  const Expr *SubE = UO->getSubExpr()->IgnoreParenCasts();
  if (!dyn_cast<DeclRefExpr>(SubE) && !dyn_cast<MemberExpr>(SubE) &&
      !dyn_cast<ArraySubscriptExpr>(SubE))
    return true;

  if (const DeclaratorDecl *DD = ConsumerInstance->getRefDecl(SubE))
    ConsumerInstance->AddrTakenDecls.insert(DD);
  return true;
}

bool PointerLevelCollectionVisitor::VisitBinaryOperator(BinaryOperator *BO)
{
  if (!BO->isAssignmentOp() && !BO->isCompoundAssignmentOp())
    return true;

  Expr *Lhs = BO->getLHS();
  const Type *Ty = Lhs->getType().getTypePtr();
  if (!Ty->isPointerType())
    return true;

  Expr *Rhs = BO->getRHS()->IgnoreParenCasts();
  if (dyn_cast<DeclRefExpr>(Rhs) ||
      dyn_cast<UnaryOperator>(Rhs) ||
      dyn_cast<ArraySubscriptExpr>(Rhs) ||
      dyn_cast<MemberExpr>(Rhs) ||
      dyn_cast<IntegerLiteral>(Rhs) ||
      dyn_cast<CXXNewExpr>(Rhs))
    return true;

  const DeclaratorDecl *DD = ConsumerInstance->getRefDecl(Lhs);
  TransAssert(DD && "NULL DD!");

  ConsumerInstance->ValidDecls.erase(DD);
  return true;
}

bool PointerLevelRewriteVisitor::VisitFieldDecl(FieldDecl *FD)
{
  const FieldDecl *TheFD = dyn_cast<FieldDecl>(ConsumerInstance->TheDecl);
  // TheDecl is a VarDecl
  if (!TheFD)
    return true;

  const FieldDecl *CanonicalFD = dyn_cast<FieldDecl>(FD->getCanonicalDecl());
  if (CanonicalFD == TheFD)
    ConsumerInstance->rewriteFieldDecl(FD);

  return true;
}

bool PointerLevelRewriteVisitor::VisitVarDecl(VarDecl *VD)
{
  const VarDecl *TheVD = dyn_cast<VarDecl>(ConsumerInstance->TheDecl);
  if (TheVD) {
    const VarDecl *CanonicalVD = VD->getCanonicalDecl();
    if (CanonicalVD == TheVD) {
      ConsumerInstance->rewriteVarDecl(VD);
    }

    // Because VD must not be addr-taken, we don't have cases like:
    //  int **p2 = &p1;
    //  where p1 is TheVD
    // It's safe to return from here.
    return true;
  }

  // TheDecl is a FieldDecl.
  // But we still need to handle VarDecls which are type of
  // struct/union where TheField could reside, if these VarDecls
  // have initializers

  if (!VD->hasInit())
    return true;

  const Type *VDTy = VD->getType().getTypePtr();
  if (!VDTy->isAggregateType())
    return true;

  if (const ArrayType *ArrayTy = dyn_cast<ArrayType>(VDTy)) {
    const Type *ArrayElemTy = ConsumerInstance->getArrayBaseElemType(ArrayTy);
    if (!ArrayElemTy->isStructureType() && !ArrayElemTy->isUnionType())
      return true;
    if (const RecordType *RDTy = ArrayElemTy->getAs<RecordType>()) {
      const RecordDecl *RD = RDTy->getDecl();
      ConsumerInstance->rewriteArrayInit(RD, VD->getInit());
    }
    return true;
  }

  if (const RecordType *RDTy = VDTy->getAs<RecordType>()) {
    const RecordDecl *RD = RDTy->getDecl();
    ConsumerInstance->rewriteRecordInit(RD, VD->getInit());
  }

  return true;
}

bool PointerLevelRewriteVisitor::VisitUnaryOperator(UnaryOperator *UO)
{
  UnaryOperator::Opcode Op = UO->getOpcode();
  if (Op != UO_Deref)
    return true;

  const Expr *SubE = UO->getSubExpr();
  const DeclaratorDecl *DD = ConsumerInstance->getRefDecl(SubE);
  if (DD != ConsumerInstance->TheDecl)
    return true;

  const DeclRefExpr *DRE = ConsumerInstance->getDeclRefExpr(SubE);
  if (DRE) {
    if (ConsumerInstance->VisitedDeclRefExprs.count(DRE))
      return true;
    ConsumerInstance->VisitedDeclRefExprs.insert(DRE);
  }
  else {
    const MemberExpr *ME = dyn_cast<MemberExpr>(SubE);
    if (ConsumerInstance->VisitedMemberExprs.count(ME))
      return true;
    ConsumerInstance->VisitedMemberExprs.insert(ME);
  }

  ConsumerInstance->rewriteDerefOp(UO);

  return true;
}

bool PointerLevelRewriteVisitor::VisitBinaryOperator(BinaryOperator *BO)
{
  if (!BO->isAssignmentOp() && !BO->isCompoundAssignmentOp())
    return true;

  const Expr *Lhs = BO->getLHS();
  // Lhs could be CallExpr
  if (dyn_cast<CallExpr>(Lhs) || dyn_cast<CXXConstructExpr>(Lhs) ||
      dyn_cast<CXXUnresolvedConstructExpr>(Lhs))
    return true;
  const DeclaratorDecl *DD = ConsumerInstance->getRefDecl(Lhs);
  // it's not always we could have a nonnull DD
  // TransAssert(DD && "NULL DD!");
  if (DD != ConsumerInstance->TheDecl)
    return true;

  const DeclRefExpr *DRE = ConsumerInstance->getDeclRefExpr(Lhs);
  if (DRE) {
    if (ConsumerInstance->VisitedDeclRefExprs.count(DRE))
      return true;
    ConsumerInstance->VisitedDeclRefExprs.insert(DRE);
  }
  else {
    const MemberExpr *ME = dyn_cast<MemberExpr>(Lhs);
    if (ConsumerInstance->VisitedMemberExprs.count(ME))
      return true;
    ConsumerInstance->VisitedMemberExprs.insert(ME);
  }

  const Expr *Rhs = BO->getRHS();
  const Type *Ty = Lhs->getType().getTypePtr();
  if (Ty->isPointerType()) {
    // Prefer removing a '*' on LHS, because it's less-likely to generate
    // bad code, e.g.,
    //   int *a, **c = &a, d, *f = &d;
    //  **c = f;
    // if we change the code above to:
    //  **c = *f;
    // **c is a derefence to a NULL pointer.
    // On the other hand, *c = f is still valid.
    const Expr *DirectLhs = Lhs->IgnoreParenCasts();
    const UnaryOperator *LhsUO = dyn_cast<UnaryOperator>(DirectLhs);
    if (LhsUO && (LhsUO->getOpcode() == UO_Deref)) {
      return ConsumerInstance->RewriteHelper->removeAStarAfter(Lhs);
    }

    const Expr *DirectRhs = Rhs->IgnoreParenCasts();
    const UnaryOperator *UO = dyn_cast<UnaryOperator>(DirectRhs);
    if (UO && (UO->getOpcode() == UO_AddrOf)) {
      return ConsumerInstance->RewriteHelper->removeAnAddrOfAfter(Rhs);
    }

    return ConsumerInstance->RewriteHelper->insertAStarBefore(Rhs);
  }
  else if (Ty->isStructureType() || Ty->isUnionType() ||
           Ty->isIntegerType()) {
    if (const ArraySubscriptExpr *ASE = dyn_cast<ArraySubscriptExpr>(Lhs))
      return ConsumerInstance->RewriteHelper->removeArraySubscriptExpr(
               ASE->getIdx());
    else
      return ConsumerInstance->RewriteHelper->removeAStarAfter(Lhs);
  }
  return true;
}

bool PointerLevelRewriteVisitor::VisitDeclRefExpr(DeclRefExpr *DRE)
{
  const ValueDecl *OrigDecl = DRE->getDecl();
  if (dyn_cast<EnumConstantDecl>(OrigDecl))
    return true;

  const DeclaratorDecl *DD = dyn_cast<DeclaratorDecl>(OrigDecl);
  TransAssert(DD && "Bad VarDecl!");

  if ((DD == ConsumerInstance->TheDecl) &&
     !(ConsumerInstance->VisitedDeclRefExprs.count(DRE))) {
    // FIXME: handle cases below
    // int foo(void) { int *a; return a[0]; }
    ConsumerInstance->rewriteDeclRefExpr(DRE);
  }

  return true;
}

bool PointerLevelRewriteVisitor::VisitMemberExpr(MemberExpr *ME)
{
  if (ConsumerInstance->VisitedMemberExprs.count(ME))
    return true;

  const ValueDecl *OrigDecl = ME->getMemberDecl();
  const DeclaratorDecl *DD = dyn_cast<DeclaratorDecl>(OrigDecl);
  if (!DD)
    return true;

  DD = dyn_cast<DeclaratorDecl>(DD->getCanonicalDecl());
  TransAssert(DD && "Bad DeclaratorDecl!");
  if (DD == ConsumerInstance->TheDecl) {
    ConsumerInstance->RewriteHelper->insertAnAddrOfBefore(ME);
    return true;
  }

  // change x->y to x.y if x is TheDecl
  if (!ME->isArrow())
    return true;

  const Expr *Base = ME->getBase()->IgnoreParenCasts();
  if (const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(Base)) {
    OrigDecl = DRE->getDecl();
    DD = dyn_cast<DeclaratorDecl>(OrigDecl);
    TransAssert(DD && "Bad VarDecl!");
    if (DD == ConsumerInstance->TheDecl) {
      ConsumerInstance->VisitedDeclRefExprs.insert(DRE);
      ConsumerInstance->replaceArrowWithDot(ME);
    }
    return true;
  }

  if (const MemberExpr *BaseME = dyn_cast<MemberExpr>(Base)) {
    OrigDecl = BaseME->getMemberDecl();
    DD = dyn_cast<DeclaratorDecl>(OrigDecl);
    TransAssert(DD && "Bad FieldDecl!");
    if (DD == ConsumerInstance->TheDecl) {
      ConsumerInstance->VisitedMemberExprs.insert(BaseME);
      ConsumerInstance->replaceArrowWithDot(ME);
    }
  }

  return true;
}

bool PointerLevelRewriteVisitor::VisitCXXDependentScopeMemberExpr(
       CXXDependentScopeMemberExpr *ME)
{
  if (ME->isImplicitAccess())
    return true;

  const Expr *Base = ME->getBase()->IgnoreParenCasts();
  if (const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(Base)) {
    const DeclaratorDecl *DD = dyn_cast<DeclaratorDecl>(DRE->getDecl());
    TransAssert(DD && "Bad VarDecl!");
    if (DD == ConsumerInstance->TheDecl) {
      ConsumerInstance->VisitedDeclRefExprs.insert(DRE);
      ConsumerInstance->replaceArrowWithDot(ME);
    }
  }
  return true;
}

void ReducePointerLevel::Initialize(ASTContext &context)
{
  Transformation::Initialize(context);
  CollectionVisitor = new PointerLevelCollectionVisitor(this);
  RewriteVisitor = new PointerLevelRewriteVisitor(this);
}

void ReducePointerLevel::HandleTranslationUnit(ASTContext &Ctx)
{
  CollectionVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
  doAnalysis();

  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  TransAssert(CollectionVisitor && "NULL CollectionVisitor!");
  TransAssert(RewriteVisitor && "NULL CollectionVisitor!");
  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);
  TransAssert(TheDecl && "NULL TheDecl!");

  setRecordDecl();
  RewriteVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void ReducePointerLevel::doAnalysis(void)
{
  DeclSet *Decls;

  Decls = AllPtrDecls[MaxIndirectLevel];
  if (Decls) {
    for (DeclSet::const_iterator I = Decls->begin(), E = Decls->end();
         I != E; ++I) {
      if (!ValidDecls.count(*I))
        continue;
      ValidInstanceNum++;
      if (TransformationCounter == ValidInstanceNum)
        TheDecl = *I;
    }
  }

  for (int Idx = MaxIndirectLevel - 1; Idx > 0; --Idx) {
    Decls = AllPtrDecls[Idx];
    if (!Decls)
      continue;

    for (DeclSet::const_iterator I = Decls->begin(), E = Decls->end();
         I != E; ++I) {
      if (!ValidDecls.count(*I) || AddrTakenDecls.count(*I))
        continue;
      ValidInstanceNum++;
      if (TransformationCounter == ValidInstanceNum)
        TheDecl = *I;
    }
  }
}

void ReducePointerLevel::setRecordDecl(void)
{
  const FieldDecl *TheFD = dyn_cast<FieldDecl>(TheDecl);
  if (!TheFD)
    return;

  TheRecordDecl = TheFD->getParent();
}

const DeclRefExpr *ReducePointerLevel::getDeclRefExpr(const Expr *Exp)
{
  const Expr *E = ignoreSubscriptExprParenCasts(Exp);

  if (const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E))
    return DRE;

  if (dyn_cast<MemberExpr>(E)) {
    return NULL;
  }

  const UnaryOperator *UO = dyn_cast<UnaryOperator>(E);
  TransAssert(UO && "Bad UnaryOperator!");
  UnaryOperator::Opcode Op = UO->getOpcode();
  (void)Op;
  TransAssert(((Op == UO_Deref) || (Op == UO_AddrOf)) &&
              "Invalid Unary Opcode!");
  const Expr *SubE = UO->getSubExpr();
  return getDeclRefExpr(SubE);
}

const DeclaratorDecl *ReducePointerLevel::getRefDecl(const Expr *Exp)
{
  const Expr *E = ignoreSubscriptExprParenCasts(Exp);

  if (dyn_cast<CXXThisExpr>(E))
    return NULL;

  if (const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E))
    return getCanonicalDeclaratorDecl(DRE);

  if (const MemberExpr *ME = dyn_cast<MemberExpr>(E))
    return getCanonicalDeclaratorDecl(ME);

  const UnaryOperator *UO = dyn_cast<UnaryOperator>(E);
  // In some case, E could not be of UO if the program under transformation
  // is invalid.
  if (!UO)
    return NULL;

  const Expr *SubE = UO->getSubExpr();
  return getRefDecl(SubE);
}

void ReducePointerLevel::checkPrefixAndPostfix(const UnaryOperator *UO)
{
  const Expr *SubE = UO->getSubExpr()->IgnoreParenCasts();
  const UnaryOperator *SubUO = dyn_cast<UnaryOperator>(SubE);
  if (!SubUO)
    return;
  if (!SubUO->isPrefix() && !SubUO->isPostfix())
    return;
  const DeclaratorDecl *DD = getRefDecl(SubUO->getSubExpr());
  if (DD) {
    ValidDecls.erase(DD);
  }
}

void ReducePointerLevel::addOneDecl(const DeclaratorDecl *DD,
                                    int IndirectLevel)
{
  DeclSet *DDSet = AllPtrDecls[IndirectLevel];
  if (!DDSet) {
    DDSet = new DeclSet();
    AllPtrDecls[IndirectLevel] = DDSet;
  }
  DDSet->insert(DD);
}

const DeclaratorDecl *
ReducePointerLevel::getCanonicalDeclaratorDecl(const Expr *E)
{
  const DeclaratorDecl *DD = NULL;

  if (const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E)) {
    const ValueDecl *ValueD = DRE->getDecl();
    DD = dyn_cast<DeclaratorDecl>(ValueD);
    if (!DD)
      return NULL;
  }
  else if (const MemberExpr *ME = dyn_cast<MemberExpr>(E)) {
    ValueDecl *OrigDecl = ME->getMemberDecl();

    // in C++, getMemberDecl returns a CXXMethodDecl.
    TransAssert(isa<FieldDecl>(OrigDecl) && "Unsupported C++ getMemberDecl!\n");
    DD = dyn_cast<DeclaratorDecl>(OrigDecl);
  }
  else {
    TransAssert(0 && "Bad Decl!");
  }

  const DeclaratorDecl *CanonicalDD =
    dyn_cast<DeclaratorDecl>(DD->getCanonicalDecl());
  TransAssert(CanonicalDD && "NULL CanonicalDD!");
  return CanonicalDD;
}

const Expr *ReducePointerLevel::getFirstInitListElem(const InitListExpr *ILE)
{
  const Expr *E = NULL;
  unsigned InitNum = ILE->getNumInits();
  for (unsigned int I = 0; I < InitNum; ++I) {
    E = ILE->getInit(I);
    ILE = dyn_cast<InitListExpr>(E);
    if (ILE) {
      E = getFirstInitListElem(ILE);
    }

    if (E)
      return E;
  }
  return NULL;
}

void ReducePointerLevel::copyInitStr(const Expr *Exp,
                                     std::string &InitStr)
{
  const Expr *E = Exp->IgnoreParenCasts();

  switch(E->getStmtClass()) {
  case Expr::DeclRefExprClass: {
    const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E);
    const ValueDecl *OrigDecl = DRE->getDecl();
    if (dyn_cast<FunctionDecl>(OrigDecl)) {
      InitStr = "0";
      return;
    }

    const VarDecl *VD = dyn_cast<VarDecl>(OrigDecl);
    TransAssert(VD && "Bad VarDecl!");
    const Expr *InitE = VD->getAnyInitializer();
    if (!InitE) {
      const Type *Ty = VD->getType().getTypePtr();
      if (Ty->isIntegerType() || Ty->isPointerType())
        InitStr = "0";
      return;
    }

    const Type *VT = VD->getType().getTypePtr();
    const ArrayType *AT = dyn_cast<ArrayType>(VT);
    if (AT) {
      const InitListExpr *ILE = dyn_cast<InitListExpr>(InitE);
      if (ILE) {
        const Expr *ElemE = getFirstInitListElem(ILE);
        if (!ElemE)
          return;
        InitE = ElemE;
      }
    }
    RewriteHelper->getExprString(InitE, InitStr);

    return;
  }

  case Expr::ArraySubscriptExprClass: {
    const ArraySubscriptExpr *ASE = dyn_cast<ArraySubscriptExpr>(E);
    if (const Expr *ElemE = getArraySubscriptElem(ASE))
      RewriteHelper->getExprString(ElemE, InitStr);
    return;
  }

  case Expr::MemberExprClass: {
    const MemberExpr *ME = dyn_cast<MemberExpr>(E);
    if (const Expr *ElemE = getMemberExprElem(ME))
      RewriteHelper->getExprString(ElemE, InitStr);
    return;
  }

  default:
    TransAssert(0 && "Uncatched initializer!");
  }
  TransAssert(0 && "Unreachable code!");
}

void ReducePointerLevel::getInitListExprString(const InitListExpr *ILE,
                                               std::string &NewInitStr,
                                               InitListHandler Handler)
{
  unsigned int NumInits = ILE->getNumInits();
  NewInitStr = "{";
  for (unsigned int I = 0; I < NumInits; ++I) {
    const Expr *SubInitE = ILE->getInit(I);
    std::string SubInitStr("");
    (this->*Handler)(SubInitE, SubInitStr);
    if (SubInitStr == "") {
      NewInitStr = "{}";
      return;
    }

    if (I == 0)
      NewInitStr += SubInitStr;
    else
      NewInitStr += ("," + SubInitStr);
  }
  NewInitStr += "}";
  return;
}

void ReducePointerLevel::getNewGlobalInitStr(const Expr *Init,
                                             std::string &InitStr)
{
  const Expr *E = Init->IgnoreParenCasts();

  switch(E->getStmtClass()) {
  case Expr::IntegerLiteralClass:
    RewriteHelper->getExprString(Init, InitStr);
    return;

  case Expr::StringLiteralClass:
    InitStr = 'a';
    return;

  case Expr::UnaryOperatorClass: {
    const UnaryOperator *UO = dyn_cast<UnaryOperator>(E);
    TransAssert((UO->getOpcode() == UO_AddrOf) && "Non-Unary Operator!");

    const Expr *SubE = UO->getSubExpr();
    TransAssert(SubE && "Bad Sub Expr!");
    // Now we try to get the init string of this addr-taken var/array_var/field
    copyInitStr(SubE, InitStr);
    return;
  }

  case Expr::InitListExprClass: {
    const InitListExpr *ILE = dyn_cast<InitListExpr>(E);
    getInitListExprString(ILE, InitStr,
                          &ReducePointerLevel::getNewGlobalInitStr);
    return;
  }

  case Expr::DeclRefExprClass: {
    const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E);
    copyInitStr(DRE, InitStr);
    return;
  }

  // it could happen if E is call to a static method of a class
  case Expr::CallExprClass: {
    const CallExpr *CE = dyn_cast<CallExpr>(E);
    const FunctionDecl *FD = CE->getDirectCallee();
    TransAssert(FD && "Invalid Function Decl!");
    const CXXMethodDecl *MDecl = dyn_cast<CXXMethodDecl>(FD); (void)MDecl;
    TransAssert(MDecl->isStatic() && "Non static CXXMethodDecl!");
    InitStr = "";
    return;
  }

  case Expr::CXXNewExprClass: {
    InitStr = "";
    return;
  }

  default:
    TransAssert(0 && "Uncatched initializer!");
  }
  TransAssert(0 && "Unreachable code!");
}

void ReducePointerLevel::getNewLocalInitStr(const Expr *Init,
                                            std::string &InitStr)
{
  const Expr *E = Init->IgnoreParenCasts();

  switch(E->getStmtClass()) {
  // catch the case like int *p = 0;
  case Expr::IntegerLiteralClass:
    RewriteHelper->getExprString(E, InitStr);
    return;

  // FIXME: not quite correct to set the InitStr to an empty string
  case Expr::GNUNullExprClass:
    InitStr = "";
    return;

  case Expr::CXXNewExprClass:
    InitStr = "";
    return;

  case Expr::StringLiteralClass:
    InitStr = 'a';
    return;

  case Expr::ConditionalOperatorClass:
    InitStr = "";
    return;

  case Expr::StmtExprClass:
    InitStr = "";
    return;

  case Expr::UnaryOperatorClass: {
    const UnaryOperator *UO = dyn_cast<UnaryOperator>(E);
    TransAssert(UO->getSubExpr() && "Bad Sub Expr!");
    RewriteHelper->getExprString(E, InitStr);

    size_t Pos;
    UnaryOperator::Opcode Op = UO->getOpcode();
    if (Op == UO_AddrOf) {
      Pos = InitStr.find_first_of('&');
      TransAssert((Pos != std::string::npos) && "No & operator!");
      InitStr.erase(Pos, 1);
    }
    else if (Op == UO_Deref) {
      Pos = InitStr.find_first_of('*');
      TransAssert((Pos != std::string::npos) && "No & operator!");
      InitStr.insert(Pos, "*");
    }
    else {
      TransAssert(0 && "Bad UnaryOperator!");
    }

    return;
  }

  case Expr::DeclRefExprClass: {
    const DeclRefExpr *DE = dyn_cast<DeclRefExpr>(E);
    RewriteHelper->getExprString(E, InitStr);

    const Type *VT = DE->getType().getTypePtr();
    // handle case like:
    // int a[10];
    // int *p = (int*)a;
    if (const ArrayType *AT = dyn_cast<ArrayType>(VT)) {
      unsigned int Dim = getArrayDimension(AT);
      std::string ArrayElemsStr("");
      for (unsigned int I = 0; I < Dim; ++I) {
        ArrayElemsStr += "[0]";
      }
      InitStr += ArrayElemsStr;
    }
    else {
      InitStr = "*" + InitStr;
    }
    return;
  }

  case Expr::MemberExprClass: // Fall-through
  case Expr::BinaryOperatorClass:
  case Expr::CXXMemberCallExprClass:
  case Expr::CallExprClass:
  case Expr::ArraySubscriptExprClass: {
    RewriteHelper->getExprString(E, InitStr);
    InitStr = "*(" + InitStr + ")";
    return;
  }

  case Expr::InitListExprClass: {
    const InitListExpr *ILE = dyn_cast<InitListExpr>(E);
    getInitListExprString(ILE, InitStr,
                          &ReducePointerLevel::getNewLocalInitStr);
    return;
  }

  default:
    TransAssert(0 && "Uncaught initializer!");
  }

  TransAssert(0 && "Unreachable code!");
}

void ReducePointerLevel::rewriteVarDecl(const VarDecl *VD)
{
  RewriteHelper->removeAStarBefore(VD);
  const Expr *Init = VD->getInit();
  if (!Init)
    return;

  const Type *Ty = VD->getType().getTypePtr();
  if (Ty->isPointerType()) {
    const Type *PointeeTy = Ty->getPointeeType().getTypePtr();
    if (PointeeTy->isRecordType()) {
      const Expr *E = Init->IgnoreParenCasts();
      Expr::StmtClass SC = E->getStmtClass();
      if ((SC == Expr::IntegerLiteralClass) ||
          (SC == Expr::StringLiteralClass)) {
        RewriteHelper->removeVarInitExpr(VD);
        return;
      }
    }
  }

  std::string NewInitStr("");
  if (VD->hasLocalStorage()) {
    getNewLocalInitStr(Init, NewInitStr);
  }
  else {
    // Global var cannot have non-const initializer,
    // e.g., "int *p = &g;" ==> "int p = g" is invalid.
    // So we need to do more work.
    // Get the init string of RHS var:
    // The transformation will look like:
    // (1) int g = 1;
    //     int *p = &g; ==> int p = 1;
    // (2) int g[2] = {1, 2};
    //     int *p = &g[1]; ==> int p = 2;
    // (2) int g[2] = {1, 2};
    //     int *p = &g; ==> int p = 1;
    // (4) struct S g = {1, 2};
    //     int *p = &g.f1; ==> int p = 1;
    getNewGlobalInitStr(Init, NewInitStr);
  }

  if (NewInitStr.empty())
    RewriteHelper->removeVarInitExpr(VD);
  else
    RewriteHelper->replaceExpr(Init, NewInitStr);
}

void ReducePointerLevel::rewriteFieldDecl(const FieldDecl *FD)
{
  RewriteHelper->removeAStarBefore(FD);
}

void ReducePointerLevel::rewriteDerefOp(const UnaryOperator *UO)
{
  RewriteHelper->removeAStarAfter(UO);
}

void ReducePointerLevel::rewriteDeclRefExpr(const DeclRefExpr *DRE)
{
  RewriteHelper->insertAnAddrOfBefore(DRE);
}

void ReducePointerLevel::replaceArrowWithDot(const Expr *E)
{
  std::string ES;
  RewriteHelper->getExprString(E, ES);
  SourceLocation LocStart = E->getLocStart();

  size_t ArrowPos = ES.find("->");
  TransAssert((ArrowPos != std::string::npos) && "Cannot find Arrow!");
  LocStart = LocStart.getLocWithOffset(ArrowPos);
  TheRewriter.ReplaceText(LocStart, 2, ".");
}

bool ReducePointerLevel::isPointerToSelf(const Type *Ty,
                                         const DeclaratorDecl *DD)
{
  const RecordType *RTy = Ty->getAs<RecordType>();
  if (!RTy)
    return false;

  const DeclContext *Ctx = DD->getDeclContext();
  const RecordDecl *RD = dyn_cast<RecordDecl>(Ctx);
  if (!RD)
    return false;

  const RecordDecl *NestedRD = RTy->getDecl();
  return (RD->getCanonicalDecl() == NestedRD->getCanonicalDecl());
}

void ReducePointerLevel::rewriteRecordInit(const RecordDecl *RD,
                                           const Expr *Init)
{
  // FIXME: Csmith doesn't have pointer fields, I will
  // leave this function as future work
}

void ReducePointerLevel::rewriteArrayInit(const RecordDecl *RD,
                                          const Expr *Init)
{
  // FIXME: Csmith doesn't have pointer fields, I will
  // leave this function as future work
}

ReducePointerLevel::~ReducePointerLevel(void)
{
  delete CollectionVisitor;
  delete RewriteVisitor;

  for (LevelToDeclMap::iterator I = AllPtrDecls.begin(),
       E = AllPtrDecls.end(); I != E; ++I) {
    delete (*I).second;
  }
}

