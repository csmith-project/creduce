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

#include "ReplaceCallExpr.h"

#include <sstream>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Replace a CallExpr with a return expression from \
this callee's body. The transformation fires only \
if the components of the return expression are one \
of the following: \n\
  * global variables \n\
  * parameters \n\
  * constants \n\
If a callee has several return statements, all of them \
will be exercised separately, i.e., the transformation \
will choose one for each iteration. \
This pass is unsound because the side-effect on globals \
and parameters caused by callees are not captured. \n";

static RegisterTransformation<ReplaceCallExpr>
         Trans("replace-callexpr", DescriptionMsg);

class ReplaceCallExprVisitor : public
  RecursiveASTVisitor<ReplaceCallExprVisitor> {

public:

  explicit ReplaceCallExprVisitor(ReplaceCallExpr *Instance)
    : ConsumerInstance(Instance),
      CurrentReturnStmt(NULL)
  { }

  bool VisitFunctionDecl(FunctionDecl *FD);

  bool VisitCallExpr(CallExpr *CE);

  bool VisitReturnStmt(ReturnStmt *RS);

private:

  bool isValidReturnStmt(ReturnStmt *RS);

  bool isValidExpr(const Expr *E);

  bool isValidNamedDecl(const NamedDecl *ND);

  bool isValidValueDecl(const ValueDecl *VD);

  bool isValidDeclRefExpr(const DeclRefExpr *DE);

  ReplaceCallExpr *ConsumerInstance;

  ReturnStmt *CurrentReturnStmt;
};

class ExprCountVisitor : public
  RecursiveASTVisitor<ExprCountVisitor> {

public:

  ExprCountVisitor(void)
    : NumExprs(0)
  { }

  unsigned int getNumExprs(void) {
    return NumExprs;
  }

  void resetNumExprs(void) {
    NumExprs = 0;
  }

  bool VisitExpr(Expr *E);

private:
  unsigned int NumExprs;

};

bool ExprCountVisitor::VisitExpr(Expr *E)
{
  NumExprs++;
  return true;
}

bool ReplaceCallExprVisitor::isValidReturnStmt(ReturnStmt *RS)
{
  Expr *E = RS->getRetValue();
  if (!E)
    return false;

  const Type *T = E->getType().getTypePtr();
  if (T->isVoidType())
    return false;


  CurrentReturnStmt = RS;
  bool RV = isValidExpr(E);
  CurrentReturnStmt = NULL;
  return RV;
}

bool ReplaceCallExprVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  if (FD->isThisDeclarationADefinition()) {
    ConsumerInstance->CurrentFD = FD;
  }
  else {
    // We skip locally declared function decls.
    if (FD->getLexicalParent()->getDeclKind() != Decl::Function)
      ConsumerInstance->CurrentFD = NULL;
  }

  return true;
}

bool ReplaceCallExprVisitor::VisitReturnStmt(ReturnStmt *RS)
{
  TransAssert(ConsumerInstance->CurrentFD && "Bad CurrentFD!");
  if (!isValidReturnStmt(RS))
    return true;

  ConsumerInstance->addOneReturnStmt(RS);
  return true;
}

bool ReplaceCallExprVisitor::VisitCallExpr(CallExpr *CE)
{
  if (ConsumerInstance->isInIncludedFile(CE))
    return true;
  FunctionDecl *FD = CE->getDirectCallee();
  if (!FD)
    return true;

  const Type *T;
  // Because CE->getCallReturnType() fails on builtin functions,
  // try to get returntype from FD (probably not really accurate thought)
  if (FD->getBuiltinID())
    T = FD->getReturnType().getTypePtr();
  else
    T = CE->getCallReturnType(FD->getASTContext()).getTypePtr();
  if (T->isVoidType())
    return true;

  ConsumerInstance->AllCallExprs.push_back(CE);
  return true;
}

bool ReplaceCallExprVisitor::isValidValueDecl(const ValueDecl *ValueD)
{
  const VarDecl *VarD = dyn_cast<VarDecl>(ValueD);

  if (!VarD || VarD->isLocalVarDecl())
    return false;

  if (VarD->hasGlobalStorage())
    return true;

  return (dyn_cast<ParmVarDecl>(VarD) != NULL);
}

bool ReplaceCallExprVisitor::isValidNamedDecl(const NamedDecl *ND)
{
  const DeclContext *Ctx = ND->getDeclContext();
  const FunctionDecl *FD = dyn_cast<FunctionDecl>(Ctx);

  // local named decl
  if (FD)
    return false;

  const ValueDecl *VD = dyn_cast<ValueDecl>(ND);
  return (VD && isValidValueDecl(VD));
}

bool ReplaceCallExprVisitor::isValidDeclRefExpr(const DeclRefExpr *DE)
{
  const ValueDecl *OrigDecl = DE->getDecl();
  bool RV = isValidValueDecl(OrigDecl);
  if (!RV)
    return false;

  if (!dyn_cast<ParmVarDecl>(OrigDecl))
    return RV;

  ConsumerInstance->addOneParmRef(CurrentReturnStmt, DE);

  return RV;
}

bool ReplaceCallExprVisitor::isValidExpr(const Expr *E)
{
  TransAssert(E && "NULL Expr!");

  switch(E->getStmtClass()) {
  case Expr::FloatingLiteralClass:
  case Expr::StringLiteralClass:
  case Expr::IntegerLiteralClass:
  case Expr::GNUNullExprClass:
  case Expr::CharacterLiteralClass: // Fall-through
    return true;

  case Expr::ParenExprClass:
    return isValidExpr(cast<ParenExpr>(E)->getSubExpr());

  case Expr::ImplicitCastExprClass:
  case Expr::CStyleCastExprClass: // Fall-through
    return isValidExpr(cast<CastExpr>(E)->getSubExpr());

  case Expr::UnaryOperatorClass:
    return isValidExpr(cast<UnaryOperator>(E)->getSubExpr());

  case Expr::BinaryOperatorClass: {
    const BinaryOperator *BE = cast<BinaryOperator>(E);
    return isValidExpr(BE->getLHS()) && isValidExpr(BE->getRHS());
  }

  case Expr::BinaryConditionalOperatorClass: {
    const BinaryConditionalOperator *BE = cast<BinaryConditionalOperator>(E);
    return isValidExpr(BE->getCommon()) && isValidExpr(BE->getFalseExpr());
  }

  case Expr::ConditionalOperatorClass: {
    const ConditionalOperator *CE = cast<ConditionalOperator>(E);
    return (isValidExpr(CE->getCond()) &&
            isValidExpr(CE->getTrueExpr()) &&
            isValidExpr(CE->getFalseExpr()));
  }

  case Expr::SizeOfPackExprClass: {
    const SizeOfPackExpr *SE = cast<SizeOfPackExpr>(E);
    NamedDecl *ND = SE->getPack();
    return isValidNamedDecl(ND);
  }

  case Expr::OffsetOfExprClass: {
    const OffsetOfExpr *OE = cast<OffsetOfExpr>(E);
    for (unsigned Idx = 0; Idx < OE->getNumExpressions(); ++Idx) {
      if (!isValidExpr(OE->getIndexExpr(Idx)))
        return false;
    }
    return true;
  }

  case Expr::MemberExprClass: {
    const MemberExpr *ME = cast<MemberExpr>(E);
    return isValidExpr(ME->getBase());
  }

  case Expr::ArraySubscriptExprClass: {
    const ArraySubscriptExpr *AE = cast<ArraySubscriptExpr>(E);
    return isValidExpr(AE->getIdx()) && isValidExpr(AE->getBase());
  }

  case Expr::DeclRefExprClass: {
    const DeclRefExpr *DE = cast<DeclRefExpr>(E);
    return isValidDeclRefExpr(DE);
  }

  default:
    return false;
  }

  TransAssert(0 && "Unreachable code!");
  return false;
}

void ReplaceCallExpr::Initialize(ASTContext &context)
{
  Transformation::Initialize(context);
  CollectionVisitor = new ReplaceCallExprVisitor(this);
}

void ReplaceCallExpr::HandleTranslationUnit(ASTContext &Ctx)
{
  CollectionVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
  doAnalysis();
  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  TransAssert(TheCallExpr && "NULL TheCallExpr!");
  TransAssert(TheReturnStmt && "NULL TheReturnStmt");

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  replaceCallExpr();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void ReplaceCallExpr::addOneReturnStmt(ReturnStmt *RS)
{
  llvm::DenseMap<FunctionDecl *, ReturnStmtsVector *>::iterator I =
    FuncToReturnStmts.find(CurrentFD);
  ReturnStmtsVector *V;
  if (I == FuncToReturnStmts.end()) {
    V = new ReturnStmtsVector();
    TransAssert(V);
    FuncToReturnStmts[CurrentFD] = V;
  }
  else {
    V = (*I).second;
  }

  TransAssert((std::find(V->begin(), V->end(), RS) == V->end()) &&
              "Duplicated ReturnStmt!");
  V->push_back(RS);
}

void ReplaceCallExpr::addOneParmRef(ReturnStmt *RS, const DeclRefExpr *DE)
{
  TransAssert(RS && "NULL ReturnStmt!");
  llvm::DenseMap<ReturnStmt *, ParmRefsVector *>::iterator I =
    ReturnStmtToParmRefs.find(RS);
  ParmRefsVector *V;
  if (I == ReturnStmtToParmRefs.end()) {
    V = new ParmRefsVector();
    TransAssert(V);
    ReturnStmtToParmRefs[RS] = V;
  }
  else {
    V = (*I).second;
  }

  TransAssert((std::find(V->begin(), V->end(), DE) == V->end()) &&
              "Duplicated ParmRef!");
  V->push_back(DE);
}

void ReplaceCallExpr::getParmPosVector(ParameterPosVector &PosVector,
                                       ReturnStmt *RS, CallExpr *CE)
{
  llvm::DenseMap<ReturnStmt *, ParmRefsVector *>::iterator RI =
    ReturnStmtToParmRefs.find(RS);
  if (RI == ReturnStmtToParmRefs.end())
    return;

  ParmRefsVector *PVector = (*RI).second;

  FunctionDecl *FD = CE->getDirectCallee();
  for (ParmRefsVector::const_iterator PI = PVector->begin(),
       PE = PVector->end(); PI != PE; ++PI) {

    const ValueDecl *OrigDecl = (*PI)->getDecl();
    const ParmVarDecl *PD = dyn_cast<ParmVarDecl>(OrigDecl);
    unsigned int Pos = 0;
    for(FunctionDecl::param_const_iterator I = FD->param_begin(),
        E = FD->param_end(); I != E; ++I) {
      if (PD == (*I))
        break;
      Pos++;
    }
    PosVector.push_back(Pos);
  }
}

bool ReplaceCallExpr::hasUnmatchedParmArg(const ParameterPosVector &PosVector,
                                          CallExpr *CE)
{
  unsigned int ArgNum = CE->getNumArgs();

  for (ParameterPosVector::const_iterator I = PosVector.begin(),
       E = PosVector.end(); I != E; ++I) {
    if ((*I) >= ArgNum)
      return true;
  }
  return false;
}

// A heuristic way to check if replacing CallExpr could cause
// code bloat.
bool ReplaceCallExpr::hasBadEffect(const ParameterPosVector &PosVector,
                                   ReturnStmt *RS, CallExpr *CE)
{
  ExprCountVisitor ECVisitor;

  Expr *RVExpr = RS->getRetValue();
  TransAssert(RVExpr && "Bad Return Expr!");
  ECVisitor.TraverseStmt(RVExpr);
  unsigned int RVNumExprs = ECVisitor.getNumExprs();
  TransAssert(RVNumExprs && "Bad NumExprs!");

  // really conservatively set 5 as a threshold value
  unsigned int ArgNum = CE->getNumArgs();
  if (!ArgNum)
    return (RVNumExprs > 5);

  llvm::SmallVector<unsigned int, 10> ArgNumExprs;
  unsigned int Num;
  unsigned int CallExprsNum = 0;
  for (unsigned int I = 0; I < ArgNum; ++I) {
    ECVisitor.TraverseStmt(CE->getArg(I));
    Num = ECVisitor.getNumExprs();
    TransAssert(Num && "Bad NumExprs!");
    ArgNumExprs.push_back(Num);
    CallExprsNum += Num;
  }

  // Adjust the size of RVExpr
  for (ParameterPosVector::const_iterator I = PosVector.begin(),
       E = PosVector.end(); I != E; ++I) {
    unsigned int Pos = (*I);
    TransAssert((Pos < ArgNum) && "Bad ParmPos!");
    Num = ArgNumExprs[Pos];
    RVNumExprs += (Num - 1);
  }

  return (RVNumExprs > (CallExprsNum + 5));
}

void ReplaceCallExpr::doAnalysis(void)
{
  for (SmallVector<CallExpr *, 10>::iterator CI = AllCallExprs.begin(),
       CE = AllCallExprs.end(); CI != CE; ++CI) {
    FunctionDecl *CalleeDecl = (*CI)->getDirectCallee();
    TransAssert(CalleeDecl && "Bad CalleeDecl!");

    llvm::DenseMap<FunctionDecl *, ReturnStmtsVector *>::iterator I =
      FuncToReturnStmts.find(CalleeDecl);
    if (I == FuncToReturnStmts.end())
      continue;

    ReturnStmtsVector *RVector = (*I).second;
    TransAssert(RVector && "NULL RVector!");
    for (ReturnStmtsVector::iterator RI = RVector->begin(),
         RE = RVector->end(); RI != RE; ++RI) {

      ParameterPosVector PosVector;
      getParmPosVector(PosVector, *RI, *CI);

      if (hasUnmatchedParmArg(PosVector, *CI))
        continue;

      if (hasBadEffect(PosVector, *RI, *CI))
        continue;

      ValidInstanceNum++;
      if (TransformationCounter != ValidInstanceNum)
        continue;

      TheCallExpr = (*CI);
      TheReturnStmt = (*RI);
    }
  }
}

void ReplaceCallExpr::getNewParmRefStr(const DeclRefExpr *DE,
                                       std::string &ParmRefStr)
{
  const ValueDecl *OrigDecl = DE->getDecl();
  const ParmVarDecl *PD = dyn_cast<ParmVarDecl>(OrigDecl);
  TransAssert(PD && "Bad ParmVarDecl!");

  FunctionDecl *FD = TheCallExpr->getDirectCallee();

  unsigned int Pos = 0;
  for(FunctionDecl::param_const_iterator I = FD->param_begin(),
      E = FD->param_end(); I != E; ++I) {
    TransAssert((Pos < TheCallExpr->getNumArgs()) &&
                "Unmatched Parm and Arg!");
    if (PD != (*I)) {
      Pos++;
      continue;
    }

    const Expr *Arg = TheCallExpr->getArg(Pos)->IgnoreParenImpCasts();
    RewriteHelper->getExprString(Arg, ParmRefStr);
    ParmRefStr = "(" + ParmRefStr + ")";

    const Type *ParmT = PD->getType().getTypePtr();
    const Type *CanParmT = Context->getCanonicalType(ParmT);
    const Type *ArgT = Arg->getType().getTypePtr();
    const Type *CanArgT = Context->getCanonicalType(ArgT);
    if (CanParmT != CanArgT) {
      std::string TypeCastStr = PD->getType().getAsString();
      ParmRefStr = "(" + TypeCastStr + ")" + ParmRefStr;
    }
    return;
  }
  TransAssert(0 && "Unreachable Code!");
}

void ReplaceCallExpr::insertParmRef
      (std::vector< std::pair<const DeclRefExpr *, int> > &SortedParmRefs,
       const DeclRefExpr *ParmRef, int Off)
{
  std::pair<const DeclRefExpr *, int> ParmOffPair(ParmRef, Off);
  if (SortedParmRefs.empty()) {
    SortedParmRefs.push_back(ParmOffPair);
    return;
  }

  std::vector< std::pair<const DeclRefExpr *, int> >::iterator I, E;
  for(I = SortedParmRefs.begin(), E = SortedParmRefs.end(); I != E; ++I) {
    int TmpOff = (*I).second;
    if (Off < TmpOff)
      break;
  }

  if (I == E)
    SortedParmRefs.push_back(ParmOffPair);
  else
    SortedParmRefs.insert(I, ParmOffPair);
}

void ReplaceCallExpr::sortParmRefsByOffs(const char *StartBuf,
       llvm::DenseMap<const DeclRefExpr *, std::string> &ParmRefToStrMap,
       std::vector< std::pair<const DeclRefExpr *, int> > &SortedParmRefs)
{
  for(llvm::DenseMap<const DeclRefExpr *, std::string>::iterator
      I = ParmRefToStrMap.begin(), E = ParmRefToStrMap.end(); I != E; ++I) {

    const DeclRefExpr *ParmRef = (*I).first;
    SourceLocation ParmRefLocStart = ParmRef->getLocStart();
    const char *ParmRefStartBuf =
      SrcManager->getCharacterData(ParmRefLocStart);

    int Off = ParmRefStartBuf - StartBuf;
    TransAssert((Off >= 0) && "Bad Offset!");
    insertParmRef(SortedParmRefs, ParmRef, Off);
  }
}

void ReplaceCallExpr::replaceParmRefs(std::string &RetStr, const Expr *RetE,
       llvm::DenseMap<const DeclRefExpr *, std::string> &ParmRefToStrMap)
{
  SourceLocation StartLoc = RetE->getLocStart();
  const char *StartBuf = SrcManager->getCharacterData(StartLoc);

  std::vector< std::pair<const DeclRefExpr *, int> > SortedParmRefs;
  // Must sort ParmRefs to make Delta value correct
  sortParmRefsByOffs(StartBuf, ParmRefToStrMap, SortedParmRefs);

  int Delta = 0;
  for(std::vector< std::pair<const DeclRefExpr *, int> >::iterator
      I = SortedParmRefs.begin(), E = SortedParmRefs.end(); I != E; ++I) {

    const DeclRefExpr *ParmRef = (*I).first;
    const ValueDecl *OrigDecl = ParmRef->getDecl();
    size_t ParmRefSize = OrigDecl->getNameAsString().size();

    int Off = (*I).second + Delta;
    std::string NewStr = ParmRefToStrMap[ParmRef];
    RetStr.replace(Off, ParmRefSize, NewStr);
    Delta += (NewStr.size() - ParmRefSize);
  }
}

void ReplaceCallExpr::replaceCallExpr(void)
{
  Expr *RetE = TheReturnStmt->getRetValue();
  TransAssert(RetE && "Bad Return Value!");

  llvm::DenseMap<const DeclRefExpr *, std::string> ParmRefToStrMap;

  llvm::DenseMap<ReturnStmt *, ParmRefsVector *>::iterator I =
    ReturnStmtToParmRefs.find(TheReturnStmt);

  if (I != ReturnStmtToParmRefs.end()) {
    ParmRefsVector *PVector = (*I).second;
    TransAssert(PVector);
    for (ParmRefsVector::const_iterator I = PVector->begin(),
         E = PVector->end(); I != E; ++I) {
      std::string ParmRefStr("");
      getNewParmRefStr((*I), ParmRefStr);
      ParmRefToStrMap[(*I)] = ParmRefStr;
    }
  }

  std::string RetString;
  RewriteHelper->getExprString(RetE, RetString);

  replaceParmRefs(RetString, RetE, ParmRefToStrMap);
  std::string ParenRetString = "(" + RetString + ")";
  RewriteHelper->replaceExpr(TheCallExpr, ParenRetString);
}

ReplaceCallExpr::~ReplaceCallExpr(void)
{
  delete CollectionVisitor;

  for (llvm::DenseMap<FunctionDecl *, ReturnStmtsVector *>::iterator
       I = FuncToReturnStmts.begin(), E = FuncToReturnStmts.end();
       I != E; ++I) {
    delete (*I).second;
  }

  for (llvm::DenseMap<ReturnStmt *, ParmRefsVector *>::iterator
       I = ReturnStmtToParmRefs.begin(), E = ReturnStmtToParmRefs.end();
       I != E; ++I) {
    delete (*I).second;
  }
}

