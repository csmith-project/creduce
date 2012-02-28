//===----------------------------------------------------------------------===//
// 
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ReducePointerLevel.h"

#include <sstream>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"

#include "RewriteUtils.h"
#include "TransformationManager.h"

using namespace clang;
using namespace llvm;

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

private:

  ReducePointerLevel *ConsumerInstance;

};

int PointerLevelCollectionVisitor::getPointerIndirectLevel(const Type *Ty)
{
  int IndirectLevel = 0;
  QualType QT = Ty->getPointeeType();;
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
  if (isVAArgField(DD))
    return true;

  // Only consider FieldDecl and VarDecl
  Decl::Kind K = DD->getKind();
  if (!(K == Decl::Field) && !(K == Decl::Var))
    return true;

  const Type *Ty = DD->getType().getTypePtr();
  const ArrayType *ArrayTy = dyn_cast<ArrayType>(Ty);
  if (ArrayTy)
    Ty = ConsumerInstance->getArrayBaseElemType(ArrayTy);
  if (!Ty->isPointerType())
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
  if (UO->getOpcode() != UO_AddrOf)
    return true;

  const Expr *SubE = UO->getSubExpr()->IgnoreParenCasts();
  if (!dyn_cast<DeclRefExpr>(SubE) && !dyn_cast<MemberExpr>(SubE) &&
      !dyn_cast<ArraySubscriptExpr>(SubE))
    return true;

/*
  const DeclaratorDecl *DD = 
    ConsumerInstance->getCanonicalDeclaratorDecl(SubE);
*/
  const DeclaratorDecl *DD = 
    ConsumerInstance->getRefDecl(SubE);
  TransAssert(DD && "NULL DD!");

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
      dyn_cast<MemberExpr>(Rhs))
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

  const ArrayType *ArrayTy = dyn_cast<ArrayType>(VDTy);
  if (ArrayTy) {
    const Type *ArrayElemTy = ConsumerInstance->getArrayBaseElemType(ArrayTy);
    if (!ArrayElemTy->isStructureType() && !ArrayElemTy->isUnionType())
      return true;
    const RecordType *RDTy = ConsumerInstance->getRecordType(ArrayElemTy);
    TransAssert(RDTy && "Bad RDTy!");
    const RecordDecl *RD = RDTy->getDecl();
    ConsumerInstance->rewriteArrayInit(RD, VD->getInit());
    return true;
  }

  const RecordType *RDTy = ConsumerInstance->getRecordType(VDTy);
  TransAssert(RDTy && "Bad RecordType!");
  const RecordDecl *RD = RDTy->getDecl();
  ConsumerInstance->rewriteRecordInit(RD, VD->getInit());
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
  if (ConsumerInstance->VisitedDeclRefExprs.count(DRE))
    return true;

  ConsumerInstance->VisitedDeclRefExprs.insert(DRE);
  ConsumerInstance->rewriteDerefOp(UO);

  return true;
}

bool PointerLevelRewriteVisitor::VisitBinaryOperator(BinaryOperator *BO)
{
  if (!BO->isAssignmentOp() && !BO->isCompoundAssignmentOp())
    return true;

  const Expr *Lhs = BO->getLHS();
  const DeclaratorDecl *DD = ConsumerInstance->getRefDecl(Lhs);
  TransAssert(DD && "NULL DD!");
  if (DD != ConsumerInstance->TheDecl)
    return true;

  const DeclRefExpr *DRE = ConsumerInstance->getDeclRefExpr(Lhs);
  if (ConsumerInstance->VisitedDeclRefExprs.count(DRE))
    return true;
  ConsumerInstance->VisitedDeclRefExprs.insert(DRE);

  const Expr *Rhs = BO->getRHS();
  const Type *Ty = Lhs->getType().getTypePtr();
  if (Ty->isPointerType()) {
    const Expr *DirectRhs = Rhs->IgnoreParenCasts();
    const UnaryOperator *UO = dyn_cast<UnaryOperator>(DirectRhs);
    if (UO && (UO->getOpcode() == UO_AddrOf)) {
      return RewriteUtils::removeAnAddrOfAfter(Rhs, 
                    &ConsumerInstance->TheRewriter,
                    ConsumerInstance->SrcManager);
    }

    return RewriteUtils::insertAStarBefore(Rhs, &ConsumerInstance->TheRewriter,
                                           ConsumerInstance->SrcManager);
  }
  else if (Ty->isStructureType() || Ty->isUnionType() || 
           Ty->isIntegerType()) {
    return RewriteUtils::removeAStarAfter(Lhs, &ConsumerInstance->TheRewriter,
                                          ConsumerInstance->SrcManager);
  }
  return true;
}

bool PointerLevelRewriteVisitor::VisitDeclRefExpr(DeclRefExpr *DRE)
{
  const ValueDecl *OrigDecl = DRE->getDecl();
  const DeclaratorDecl *DD = dyn_cast<DeclaratorDecl>(OrigDecl);
  TransAssert(DD && "Bad VarDecl!");

  if ((DD == ConsumerInstance->TheDecl) &&
     !(ConsumerInstance->VisitedDeclRefExprs.count(DRE))) {
    ConsumerInstance->rewriteDeclRefExpr(DRE);
  }

  return true;
}

void ReducePointerLevel::Initialize(ASTContext &context) 
{
  Context = &context;
  SrcManager = &Context->getSourceManager();
  CollectionVisitor = new PointerLevelCollectionVisitor(this);
  RewriteVisitor = new PointerLevelRewriteVisitor(this);
  TheRewriter.setSourceMgr(Context->getSourceManager(), 
                           Context->getLangOptions());
}

void ReducePointerLevel::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    CollectionVisitor->TraverseDecl(*I);
  }
}
 
void ReducePointerLevel::HandleTranslationUnit(ASTContext &Ctx)
{
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
  const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E);

  if (DRE)
    return DRE;

  const UnaryOperator *UO = dyn_cast<UnaryOperator>(E);
  TransAssert(UO && "Bad UnaryOperator!");
  UnaryOperator::Opcode Op = UO->getOpcode();
  TransAssert(((Op == UO_Deref) || (Op == UO_AddrOf)) && 
              "Non-Deref-or-AddrOf Opcode!");
  const Expr *SubE = UO->getSubExpr();
  return getDeclRefExpr(SubE);
}

const DeclaratorDecl *ReducePointerLevel::getRefDecl(const Expr *Exp)
{
  const Expr *E = ignoreSubscriptExprParenCasts(Exp);
  const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E);

  if (DRE)
    return getCanonicalDeclaratorDecl(DRE);

  const MemberExpr *ME = dyn_cast<MemberExpr>(E);
  if (ME)
    return getCanonicalDeclaratorDecl(ME);

  const UnaryOperator *UO = dyn_cast<UnaryOperator>(E);
  TransAssert(UO && "Bad UnaryOperator!");
  UnaryOperator::Opcode Op = UO->getOpcode();
  TransAssert(((Op == UO_Deref) || (Op == UO_AddrOf)) && 
              "Non-Deref-or-AddrOf Opcode!");
  const Expr *SubE = UO->getSubExpr();
  return getRefDecl(SubE);
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
  const DeclaratorDecl *DD;
  const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E);
  const MemberExpr *ME = dyn_cast<MemberExpr>(E);

  if (DRE) {
    const ValueDecl *ValueD = DRE->getDecl();
    DD = dyn_cast<DeclaratorDecl>(ValueD);
    TransAssert(DD && "Bad Declarator!"); 
  }
  else if (ME) {
    ValueDecl *OrigDecl = ME->getMemberDecl();
    FieldDecl *FD = dyn_cast<FieldDecl>(OrigDecl);

    // in C++, getMemberDecl returns a CXXMethodDecl.
    TransAssert(FD && "Unsupported C++ getMemberDecl!\n");
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
    const VarDecl *VD = dyn_cast<VarDecl>(OrigDecl);
    TransAssert(VD && "Bad VarDecl!");
    const Expr *InitE = VD->getAnyInitializer();
    if (!InitE)
      return;

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
    RewriteUtils::getExprString(InitE, InitStr,
                                &TheRewriter, SrcManager);

    return;
  }
  
  case Expr::ArraySubscriptExprClass: {
    const ArraySubscriptExpr *ASE = dyn_cast<ArraySubscriptExpr>(E);
    const Expr *ElemE = getArraySubscriptElem(ASE);
    if (ElemE)
      RewriteUtils::getExprString(ElemE, InitStr,
                                  &TheRewriter, SrcManager);
    return;
  }

  case Expr::MemberExprClass: {
    const MemberExpr *ME = dyn_cast<MemberExpr>(E);
    const Expr *ElemE = getMemberExprElem(ME);
    if (ElemE)
      RewriteUtils::getExprString(ElemE, InitStr,
                                  &TheRewriter, SrcManager);
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
    RewriteUtils::getExprString(Init, InitStr, &TheRewriter, SrcManager);
    return;

  case Expr::StringLiteralClass:
    InitStr = 'a';
    return;

  case Expr::UnaryOperatorClass: {
    const UnaryOperator *UO = dyn_cast<UnaryOperator>(E);
    TransAssert((UO->getOpcode() == UO_AddrOf) && "None Unary Operator!");

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
    RewriteUtils::getExprString(E, InitStr, &TheRewriter, SrcManager);
    return;

  case Expr::StringLiteralClass:
    InitStr = 'a';
    return;

  case Expr::UnaryOperatorClass: {
    const UnaryOperator *UO = dyn_cast<UnaryOperator>(E);
    const Expr *SubE = UO->getSubExpr();
    TransAssert(SubE && "Bad Sub Expr!");
    RewriteUtils::getExprString(E, InitStr, &TheRewriter, SrcManager);

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
    RewriteUtils::getExprString(E, InitStr, &TheRewriter, SrcManager);

    const Type *VT = DE->getType().getTypePtr();
    const ArrayType *AT = dyn_cast<ArrayType>(VT);
    // handle case like:
    // int a[10];
    // int *p = (int*)a;
    if (AT) {
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
  case Expr::ArraySubscriptExprClass: {
    RewriteUtils::getExprString(E, InitStr, &TheRewriter, SrcManager);
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
    TransAssert(0 && "Uncatched initializer!");
  }

  TransAssert(0 && "Unreachable code!");
}

void ReducePointerLevel::rewriteVarDecl(const VarDecl *VD)
{
  RewriteUtils::removeAStarBefore(VD, &TheRewriter, SrcManager); 
  const Expr *Init = VD->getInit();
  if (!Init)
    return;
  
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
    RewriteUtils::removeVarInitExpr(VD, &TheRewriter, SrcManager);
  else
    RewriteUtils::replaceExpr(Init, NewInitStr, &TheRewriter, SrcManager);
}

const RecordType *ReducePointerLevel::getRecordType(const Type *T)
{
  if (T->isUnionType())
    return T->getAsUnionType();
  else if (T->isStructureType())
    return T->getAsStructureType();
  else
    return NULL;
}

void ReducePointerLevel::rewriteFieldDecl(const FieldDecl *FD)
{
  RewriteUtils::removeAStarBefore(FD, &TheRewriter, SrcManager); 
}

void ReducePointerLevel::rewriteDerefOp(const UnaryOperator *UO)
{
  RewriteUtils::removeAStarAfter(UO, &TheRewriter, SrcManager);
}

void ReducePointerLevel::rewriteDeclRefExpr(const DeclRefExpr *DRE)
{
  RewriteUtils::insertAnAddrOfBefore(DRE, &TheRewriter, SrcManager);
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
  if (CollectionVisitor)
    delete CollectionVisitor;
  if (RewriteVisitor)
    delete RewriteVisitor;

  for (LevelToDeclMap::iterator I = AllPtrDecls.begin(), 
       E = AllPtrDecls.end(); I != E; ++I) {
    delete (*I).second;
  }
}

