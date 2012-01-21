#include "RemoveNestedFunction.h"

#include <sstream>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"

#include "RewriteUtils.h"
#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Remove a nested function invocation from its enclosing \
expression. The transformation will create a temporary \
variable with the correct type, assgin the return value \
of the selected nested function to the created variable, \
and replace the function invocation with this temporary \
variable. \n";


static RegisterTransformation<RemoveNestedFunction>
         Trans("remove-nested-function", DescriptionMsg);

class RNFCollectionVisitor : public RecursiveASTVisitor<RNFCollectionVisitor> {
public:

  typedef RecursiveASTVisitor<RNFCollectionVisitor> Super;

  RNFCollectionVisitor(RemoveNestedFunction *Instance)
    : ConsumerInstance(Instance),
      CurrentFuncDecl(NULL),
      CurrentStmt(NULL),
      NeedParen(false)
  { }

  bool VisitCompoundStmt(CompoundStmt *S);

  bool VisitIfStmt(IfStmt *IS);

  bool VisitForStmt(ForStmt *FS);

  bool VisitWhileStmt(WhileStmt *WS);

  bool VisitDoStmt(DoStmt *DS);

  bool VisitSwitchStmt(SwitchStmt *SS);

  bool VisitCaseStmt(CaseStmt *CS);

  bool VisitDefaultStmt(DefaultStmt *DS);

  void visitNonCompoundStmt(Stmt *S);

  bool VisitCallExpr(CallExpr *CallE);

  void setCurrentFuncDecl(FunctionDecl *FD) {
    CurrentFuncDecl = FD;
  }

private:

  RemoveNestedFunction *ConsumerInstance;

  FunctionDecl *CurrentFuncDecl;

  Stmt *CurrentStmt;

  bool NeedParen;

};

bool RNFCollectionVisitor::VisitCompoundStmt(CompoundStmt *CS)
{
  for (CompoundStmt::body_iterator I = CS->body_begin(),
       E = CS->body_end(); I != E; ++I) {
    CurrentStmt = (*I);
    TraverseStmt(*I);
  }
  return true;
}

void RNFCollectionVisitor::visitNonCompoundStmt(Stmt *S)
{
  if (S && isa<CompoundStmt>(S))
    return;

  CurrentStmt = (S);
  NeedParen = true;
  TraverseStmt(S);
  NeedParen = false;
}

// It is used to handle the case where if-then or else branch
// is not treated as a CompoundStmt. So it cannot be traversed
// from VisitCompoundStmt, e.g.,
//   if (x)
//     foo(bar())
bool RNFCollectionVisitor::VisitIfStmt(IfStmt *IS)
{
  Expr *E = IS->getCond();
  TraverseStmt(E);

  Stmt *ThenB = IS->getThen();
  visitNonCompoundStmt(ThenB);

  Stmt *ElseB = IS->getElse();
  visitNonCompoundStmt(ElseB);

  return true;
}

// It causes unsound transformation because 
// the semantics of loop execution has been changed. 
// For example,
//   int foo(int x)
//   {
//     int i;
//     for(i = 0; i < bar(bar(x)); i++)
//       ...
//   }
// will be transformed to:
//   int foo(int x)
//   {
//     int i;
//     int tmp_var = bar(x);
//     for(i = 0; i < bar(tmp_var); i++)
//       ...
//   }
bool RNFCollectionVisitor::VisitForStmt(ForStmt *FS)
{
  Expr *E = FS->getCond();
  TraverseStmt(E);

  Stmt *Body = FS->getBody();
  visitNonCompoundStmt(Body);
  return true;
}

bool RNFCollectionVisitor::VisitWhileStmt(WhileStmt *WS)
{
  Expr *E = WS->getCond();
  TraverseStmt(E);

  Stmt *Body = WS->getBody();
  visitNonCompoundStmt(Body);
  return true;
}

bool RNFCollectionVisitor::VisitDoStmt(DoStmt *DS)
{
  Stmt *Body = DS->getBody();
  visitNonCompoundStmt(Body);
  return true;
}

bool RNFCollectionVisitor::VisitSwitchStmt(SwitchStmt *SS)
{
  Stmt *Body = SS->getBody();
  visitNonCompoundStmt(Body);
  return true;
}

bool RNFCollectionVisitor::VisitCaseStmt(CaseStmt *CS)
{
  Stmt *Body = CS->getSubStmt();
  visitNonCompoundStmt(Body);
  return true;
}

bool RNFCollectionVisitor::VisitDefaultStmt(DefaultStmt *DS)
{
  Stmt *Body = DS->getSubStmt();
  visitNonCompoundStmt(Body);
  return true;
}

bool RNFCollectionVisitor::VisitCallExpr(CallExpr *CallE) 
{
  if ((std::find(ConsumerInstance->ValidCallExprs.begin(), 
                 ConsumerInstance->ValidCallExprs.end(), CallE) 
          == ConsumerInstance->ValidCallExprs.end()) && 
      !ConsumerInstance->CallExprQueue.empty()) {

    ConsumerInstance->ValidInstanceNum++;
    ConsumerInstance->ValidCallExprs.push_back(CallE);

    if (ConsumerInstance->ValidInstanceNum == 
        ConsumerInstance->TransformationCounter) {
      ConsumerInstance->TheFuncDecl = CurrentFuncDecl;
      ConsumerInstance->TheStmt = CurrentStmt;
      ConsumerInstance->TheCallExpr = CallE;
      ConsumerInstance->NeedParen = NeedParen;
    }
  }

  ConsumerInstance->CallExprQueue.push_back(CallE);

  for (CallExpr::arg_iterator I = CallE->arg_begin(),
       E = CallE->arg_end(); I != E; ++I) {
      TraverseStmt(*I);
  }

  ConsumerInstance->CallExprQueue.pop_back();

  return true;
}

void RemoveNestedFunction::Initialize(ASTContext &context) 
{
  Context = &context;
  SrcManager = &Context->getSourceManager();
  NestedInvocationVisitor = new RNFCollectionVisitor(this);
  TheRewriter.setSourceMgr(Context->getSourceManager(), 
                           Context->getLangOptions());
}

void RemoveNestedFunction::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    FunctionDecl *FD = dyn_cast<FunctionDecl>(*I);
    if (FD && FD->isThisDeclarationADefinition()) {
      NestedInvocationVisitor->setCurrentFuncDecl(FD);
      NestedInvocationVisitor->TraverseDecl(FD);
      NestedInvocationVisitor->setCurrentFuncDecl(FD);
    }
  }
}
 
void RemoveNestedFunction::HandleTranslationUnit(ASTContext &Ctx)
{
  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  TransAssert(TheFuncDecl && "NULL TheFuncDecl!");
  TransAssert(TheStmt && "NULL TheStmt!");
  TransAssert(TheCallExpr && "NULL TheCallExpr");

  addNewTmpVariable();
  addNewAssignStmt();
  replaceCallExpr();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

bool RemoveNestedFunction::addNewTmpVariable(void)
{
  QualType QT = TheCallExpr->getCallReturnType();
  std::string VarStr;
  std::stringstream SS;

  SS << "__trans_tmp_" << (uintptr_t)&QT;
  VarStr = SS.str();
  setTmpVarName(VarStr);

  QT.getAsStringInternal(VarStr,
                         Context->getPrintingPolicy());

  VarStr += ";";
  return RewriteUtils::addLocalVarToFunc(VarStr, TheFuncDecl, 
                                         &TheRewriter, SrcManager);
}

bool RemoveNestedFunction::addNewAssignStmt(void)
{
  std::string IndentStr = 
    RewriteUtils::getStmtIndentString(TheStmt, SrcManager);

  if (NeedParen) {
    SourceRange StmtRange = TheStmt->getSourceRange();
    SourceLocation LocEnd = 
      RewriteUtils::getEndLocationFromBegin(StmtRange, &TheRewriter);

    std::string PostStr = "\n" + IndentStr + "}";
    if (TheRewriter.InsertTextAfterToken(LocEnd, PostStr))
      return false;
  }

  SourceLocation StmtLocStart = TheStmt->getLocStart();

  std::string CallExprStr;
  RewriteUtils::getExprString(TheCallExpr, CallExprStr,
                              &TheRewriter, SrcManager);

  std::string AssignStmtStr;
  
  if (NeedParen) {
    AssignStmtStr = "{\n";
    AssignStmtStr += IndentStr + "  " + getTmpVarName() + " = ";
    AssignStmtStr += CallExprStr;
    AssignStmtStr += ";\n" + IndentStr + "  ";
  }
  else {
    AssignStmtStr = getTmpVarName() + " = ";
    AssignStmtStr += CallExprStr;
    AssignStmtStr += ";\n" + IndentStr;
  }
  
  return !(TheRewriter.InsertText(StmtLocStart, 
             AssignStmtStr, /*InsertAfter=*/false));
}

bool RemoveNestedFunction::replaceCallExpr(void)
{
  return RewriteUtils::replaceExpr(TheCallExpr, TmpVarName,
                                   &TheRewriter, SrcManager);
}

RemoveNestedFunction::~RemoveNestedFunction(void)
{
  if (NestedInvocationVisitor)
    delete NestedInvocationVisitor;
}

