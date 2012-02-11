#include "SimpleInliner.h"

#include <sstream>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"

#include "RewriteUtils.h"
#include "TransformationManager.h"

using namespace clang;
using namespace llvm;

static const char *DescriptionMsg =
"A really simple inliner\n";

static RegisterTransformation<SimpleInliner>
         Trans("simple-inliner", DescriptionMsg);

class SimpleInlinerCollectionVisitor : public 
  RecursiveASTVisitor<SimpleInlinerCollectionVisitor> {

public:

  explicit SimpleInlinerCollectionVisitor(SimpleInliner *Instance)
    : ConsumerInstance(Instance),
      NumStmts(0)
  { }

  bool VisitCallExpr(CallExpr *CE);
  bool VisitBreakStmt(BreakStmt *S);
  bool VisitCompoundStmt(CompoundStmt *S);
  bool VisitContinueStmt(ContinueStmt *S);
  bool VisitDeclStmt(DeclStmt *S);
  bool VisitDoStmt(DoStmt *S);
  bool VisitForStmt(ForStmt *S);
  bool VisitGotoStmt(GotoStmt *S);
  bool VisitIfStmt(IfStmt *S);
  bool VisitIndirectGotoStmt(IndirectGotoStmt *S);
  bool VisitReturnStmt(ReturnStmt *S);
  bool VisitSwitchCase(SwitchCase *S);
  bool VisitSwitchStmt(SwitchStmt *S);
  bool VisitWhileStmt(WhileStmt *S);
  bool VisitBinaryOperator(BinaryOperator *S);

  unsigned int getNumStmts(void) {
    return NumStmts;
  }

  void setNumStmts(unsigned int Num) {
    NumStmts = Num;
  }

private:

  SimpleInliner *ConsumerInstance;

  unsigned int NumStmts;
};

class SimpleInlinerFunctionVisitor : public 
  RecursiveASTVisitor<SimpleInlinerFunctionVisitor> {

public:

  explicit SimpleInlinerFunctionVisitor(SimpleInliner *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitReturnStmt(ReturnStmt *RS);

  bool VisitDeclRefExpr(DeclRefExpr *DRE);

private:

  SimpleInliner *ConsumerInstance;

};

bool SimpleInlinerCollectionVisitor::VisitCallExpr(CallExpr *CE)
{
  FunctionDecl *FD = CE->getDirectCallee();
  if (!FD)
    return true;

  ConsumerInstance->AllCallExprs.push_back(CE);
  ConsumerInstance->CalleeToCallerMap[CE] = ConsumerInstance->CurrentFD;
  NumStmts++;
  return true;
}

bool SimpleInlinerCollectionVisitor::VisitBreakStmt(BreakStmt *S)
{
  NumStmts++;
  return true;
}

bool SimpleInlinerCollectionVisitor::VisitCompoundStmt(CompoundStmt *S)
{
  NumStmts++;
  return true;
}

bool SimpleInlinerCollectionVisitor::VisitContinueStmt(ContinueStmt *S)
{
  NumStmts++;
  return true;
}

bool SimpleInlinerCollectionVisitor::VisitDeclStmt(DeclStmt *S)
{
  NumStmts++;
  return true;
}

bool SimpleInlinerCollectionVisitor::VisitDoStmt(DoStmt *S)
{
  NumStmts++;
  return true;
}

bool SimpleInlinerCollectionVisitor::VisitForStmt(ForStmt *S)
{
  NumStmts++;
  return true;
}

bool SimpleInlinerCollectionVisitor::VisitGotoStmt(GotoStmt *S)
{
  NumStmts++;
  return true;
}

bool SimpleInlinerCollectionVisitor::VisitIfStmt(IfStmt *S)
{
  NumStmts++;
  return true;
}

bool SimpleInlinerCollectionVisitor::VisitIndirectGotoStmt(IndirectGotoStmt *S)
{
  NumStmts++;
  return true;
}

bool SimpleInlinerCollectionVisitor::VisitReturnStmt(ReturnStmt *S)
{
  NumStmts++;
  return true;
}

bool SimpleInlinerCollectionVisitor::VisitSwitchCase(SwitchCase *S)
{
  NumStmts++;
  return true;
}

bool SimpleInlinerCollectionVisitor::VisitSwitchStmt(SwitchStmt *S)
{
  NumStmts++;
  return true;
}

bool SimpleInlinerCollectionVisitor::VisitWhileStmt(WhileStmt *S)
{
  NumStmts++;
  return true;
}

bool SimpleInlinerCollectionVisitor::VisitBinaryOperator(BinaryOperator *S)
{
  NumStmts++;
  return true;
}

bool SimpleInlinerFunctionVisitor::VisitReturnStmt(ReturnStmt *RS)
{
  ConsumerInstance->ReturnStmts.push_back(RS);
  return true;
}

bool SimpleInlinerFunctionVisitor::VisitDeclRefExpr(DeclRefExpr *DRE)
{
  const ValueDecl *OrigDecl = DRE->getDecl();
  const ParmVarDecl *PD = dyn_cast<ParmVarDecl>(OrigDecl);
  if (PD)
     ConsumerInstance->ParmRefs.push_back(DRE); 
  return true;
}

void SimpleInliner::Initialize(ASTContext &context) 
{
  Context = &context;
  SrcManager = &Context->getSourceManager();
  NameQueryWrap = 
    new TransNameQueryWrap(RewriteUtils::getTmpVarNamePrefix());
  CollectionVisitor = new SimpleInlinerCollectionVisitor(this);
  FunctionVisitor = new SimpleInlinerFunctionVisitor(this);
  TheRewriter.setSourceMgr(Context->getSourceManager(), 
                           Context->getLangOptions());
}

void SimpleInliner::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    FunctionDecl *FD = dyn_cast<FunctionDecl>(*I);
    if (!(FD && FD->isThisDeclarationADefinition()))
      continue;

    CurrentFD = FD;
    CollectionVisitor->setNumStmts(0);
    CollectionVisitor->TraverseDecl(FD);

    if ((CollectionVisitor->getNumStmts() <= MaxNumStmts) &&
        !FD->isVariadic()) {
      ValidFunctionDecls.insert(FD->getCanonicalDecl());
    }
  }
}

void SimpleInliner::HandleTranslationUnit(ASTContext &Ctx)
{
  doAnalysis();
  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  TransAssert(CurrentFD && "NULL CurrentFD!");
  TransAssert(TheCallExpr && "NULL TheCallExpr!");

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  NameQueryWrap->TraverseDecl(Ctx.getTranslationUnitDecl());
  NamePostfix = NameQueryWrap->getMaxNamePostfix() + 1;

  FunctionVisitor->TraverseDecl(CurrentFD);
  replaceCallExpr();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

bool SimpleInliner::isValidArgExpr(const Expr *E)
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
    return isValidArgExpr(cast<ParenExpr>(E)->getSubExpr());

  case Expr::ImplicitCastExprClass:
  case Expr::CStyleCastExprClass: // Fall-through
    return isValidArgExpr(cast<CastExpr>(E)->getSubExpr());

  case Expr::MemberExprClass:
    return true;

  case Expr::ArraySubscriptExprClass: {
    const ArraySubscriptExpr *AE = cast<ArraySubscriptExpr>(E);
    return isValidArgExpr(AE->getIdx());
  }

  case Expr::DeclRefExprClass:
    return true;

  default:
    return false;
  }
  TransAssert(0 && "Unreachable code!");
  return false;
}

bool SimpleInliner::hasValidArgExprs(const CallExpr *CE)
{
  for(CallExpr::const_arg_iterator I = CE->arg_begin(), E = CE->arg_end();
      I != E; ++I) {
    if (!isValidArgExpr(*I))
      return false;
  }
  return true;
}

void SimpleInliner::doAnalysis(void)
{
  for (SmallVector<CallExpr *, 10>::iterator CI = AllCallExprs.begin(),
       CE = AllCallExprs.end(); CI != CE; ++CI) {

    FunctionDecl *CalleeDecl = (*CI)->getDirectCallee(); 
    TransAssert(CalleeDecl && "Bad CalleeDecl!");
    FunctionDecl *CanonicalDecl = CalleeDecl->getCanonicalDecl();
    if (!ValidFunctionDecls.count(CanonicalDecl))
      continue;

    if (!hasValidArgExprs(*CI))
      continue;

    ValidInstanceNum++;
    if (TransformationCounter == ValidInstanceNum) {
      CurrentFD = CalleeDecl;
      TheCaller = CalleeToCallerMap[(*CI)];
      TransAssert(TheCaller && "NULL TheCaller!");
      TheCallExpr = (*CI);
    }
  }
}

std::string SimpleInliner::getNewTmpName(void)
{
  std::stringstream SS;
  SS << RewriteUtils::getTmpVarNamePrefix() << NamePostfix;
  NamePostfix++;
  return SS.str();
}

void SimpleInliner::createReturnVar(void)
{
  const Type *FDType = CurrentFD->getResultType().getTypePtr();
  const Type *CallExprType = TheCallExpr->getCallReturnType().getTypePtr();

  // We don't need tmp var
  if (FDType->isVoidType() && CallExprType->isVoidType()) {
    return; 
  }

  TmpVarName = getNewTmpName();
  std::string VarStr = TmpVarName;
  CurrentFD->getResultType().getAsStringInternal(VarStr, 
                               Context->getPrintingPolicy());
  VarStr += ";";
  RewriteUtils::addLocalVarToFunc(VarStr, TheCaller,
                                 &TheRewriter, SrcManager);
}

void SimpleInliner::generateParamStrings(void)
{
  unsigned int ArgNum = TheCallExpr->getNumArgs();
  FunctionDecl *FD = TheCallExpr->getDirectCallee();
  unsigned int Idx;

  for(Idx = 0; Idx < FD->getNumParams(); ++Idx) {
    const ParmVarDecl *PD = FD->getParamDecl(Idx);
    std::string ParmStr = PD->getNameAsString();
    PD->getType().getAsStringInternal(ParmStr, 
                                      Context->getPrintingPolicy());
    if (Idx < ArgNum) {
      const Expr *Arg = TheCallExpr->getArg(Idx);
      ParmStr += " = ";
      std::string ArgStr("");
      RewriteUtils::getExprString(Arg, ArgStr, &TheRewriter, SrcManager);
      ParmStr += ArgStr;
    }
    ParmStr += ";\n";
    ParmStrings.push_back(ParmStr);
  }
}

void SimpleInliner::insertReturnStmt
      (std::vector< std::pair<ReturnStmt *, int> > &SortedReturnStmts,
       ReturnStmt *RS, int Off)
{
  std::pair<ReturnStmt *, int> ReturnStmtOffPair(RS, Off);
  if (SortedReturnStmts.empty()) {
    SortedReturnStmts.push_back(ReturnStmtOffPair);
    return;
  }

  std::vector< std::pair<ReturnStmt *, int> >::iterator I, E;
  for(I = SortedReturnStmts.begin(), E = SortedReturnStmts.end(); I != E; ++I) {
    int TmpOff = (*I).second;
    if (Off < TmpOff)
      break;
  }

  if (I == E)
    SortedReturnStmts.push_back(ReturnStmtOffPair);
  else 
    SortedReturnStmts.insert(I, ReturnStmtOffPair);
}
void SimpleInliner::sortReturnStmtsByOffs(const char *StartBuf, 
       std::vector< std::pair<ReturnStmt *, int> > &SortedReturnStmts)
{
  for (ReturnStmtsVector::iterator I = ReturnStmts.begin(), 
       E = ReturnStmts.end(); I != E; ++I) {
    ReturnStmt *RS = (*I);
    SourceLocation RSLocStart = RS->getLocStart();
    const char *RSStartBuf = SrcManager->getCharacterData(RSLocStart);
    int Off = RSStartBuf - StartBuf;
    TransAssert((Off >= 0) && "Bad Offset!");
  }
}

void SimpleInliner::copyFunctionBody(void)
{
  Stmt *Body = CurrentFD->getBody();
  TransAssert(Body && "NULL Body!");

  std::string FuncBodyStr("");
  RewriteUtils::getStmtString(Body, FuncBodyStr, &TheRewriter, SrcManager);
  TransAssert(FuncBodyStr[0] == '{');

  SourceLocation StartLoc = Body->getLocStart();
  const char *StartBuf = SrcManager->getCharacterData(StartLoc);

  std::vector< std::pair<ReturnStmt *, int> > SortedReturnStmts;
  sortReturnStmtsByOffs(StartBuf, SortedReturnStmts);

  // Now we start rewriting
  int Delta = 1; // skip the first { symbol
  for(SmallVector<std::string, 10>::iterator I = ParmStrings.begin(),
       E = ParmStrings.end(); I != E; ++I) {
    std::string PStr = (*I);
    FuncBodyStr.insert(Delta, PStr);
    Delta += PStr.size();
  }

  int ReturnSZ = 6;
  int TmpVarNameSize = static_cast<int>(TmpVarName.size());

  for(std::vector< std::pair<ReturnStmt *, int> >::iterator
      I = SortedReturnStmts.begin(), E = SortedReturnStmts.end(); 
      I != E; ++I) {

    ReturnStmt *RS = (*I).first;
    int Off = (*I).second + Delta;
    Expr *Exp = RS->getRetValue();
    if (Exp) {
      const Type *T = Exp->getType().getTypePtr();
      if (!T->isVoidType()) {
        FuncBodyStr.replace(Off, ReturnSZ, TmpVarName);
        Delta += (TmpVarNameSize - ReturnSZ);
        continue;
      }
    }
    FuncBodyStr.replace(Off, ReturnSZ, "");
    Delta -= ReturnSZ;
  }

  // TODO
  SourceLocation TmpLoc = TheCallExpr->getLocStart();
  TheRewriter.InsertText(TmpLoc, FuncBodyStr, /*InsertAfter=*/false);
}

void SimpleInliner::replaceCallExpr(void)
{
  // Create a new tmp var for return value
  createReturnVar();
  generateParamStrings();
  copyFunctionBody();
  RewriteUtils::replaceExpr(TheCallExpr, TmpVarName, 
                            &TheRewriter, SrcManager);
}

SimpleInliner::~SimpleInliner(void)
{
  if (CollectionVisitor)
    delete CollectionVisitor;
  if (FunctionVisitor)
    delete FunctionVisitor;
}

