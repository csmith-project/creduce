#include "SimpleInliner.h"

#include <sstream>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"

#include "RewriteUtils.h"
#include "TransformationManager.h"

using namespace clang;
using namespace llvm;

static const char *DescriptionMsg =
"A really simple inliner replaces a callexpr if it \n\
  * stands for a call to a leave function; \n\
  * AND is a standalone statement;\n";

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

void SimpleInliner::Initialize(ASTContext &context) 
{
  Context = &context;
  SrcManager = &Context->getSourceManager();
  CollectionVisitor = new SimpleInlinerCollectionVisitor(this);
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

    if (CollectionVisitor->getNumStmts() <= MaxNumStmts) {
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

  TransAssert(TheCallExpr && "NULL TheCallExpr!");

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

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

void SimpleInliner::replaceCallExpr(void)
{
  // TODO
}

SimpleInliner::~SimpleInliner(void)
{
  if (CollectionVisitor)
    delete CollectionVisitor;
}

