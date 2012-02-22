#include "CopyPropagation.h"

#include <sstream>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"

#include "RewriteUtils.h"
#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"A simply copy propagation pass. \
Copy propagation only happens to: \n\
  * variables \n\
  * member expressions \n\
  * array subscript expressions \n\
Only those value obtained through simple \
assignment or initilizer will be propagated. \
Here \"simple assignment\" is defined as: \n\
  x = y \n\
where both x and y are either variables, member expressions \
or array subscript expressions. For example, \
x = foo() is not considered as a simple assignment. \
Copy propagation of LHS will stop upon \
any non-simple assignment. Therefore, in the above example, foo() \
will not be propagated. \n";

static RegisterTransformation<CopyPropagation>
         Trans("copy-propagation", DescriptionMsg);

class CopyPropCollectionVisitor : public RecursiveASTVisitor<CopyPropCollectionVisitor> {
public:

  explicit CopyPropCollectionVisitor(CopyPropagation *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitVarDecl(VarDecl *VD);

  bool VisitBinaryOperator(BinaryOperator *BO);

  bool VisitDeclRefExpr(DeclRefExpr *DRE);

  bool VisitMemberExpr(MemberExpr *ME);

  bool VisitArraySubscriptExpr(ArraySubscriptExpr *ASE);

private:

  CopyPropagation *ConsumerInstance;

};

bool CopyPropCollectionVisitor::VisitVarDecl(VarDecl *VD)
{
  if (!VD->hasInit())
    return true;

  const Expr *Init = VD->getInit()->IgnoreParenCasts();

  // Skip init list, note it also skips code like:
  //   int i = {1};
  // But this kind of code almost couldn't happen,
  // because c_delta will remove {} pair.
  if (dyn_cast<InitListExpr>(Init))
    return true;

  if (ConsumerInstance->isValidExpr(Init)) {
    const VarDecl *CanonicalVD = VD->getCanonicalDecl();
    ConsumerInstance->VarToExpr[CanonicalVD] = Init;
  }

  return true;
}

bool CopyPropCollectionVisitor::VisitBinaryOperator(BinaryOperator *BO)
{
  if (!BO->isAssignmentOp() && !BO->isCompoundAssignmentOp())
    return true;

  const Expr *Lhs = BO->getLHS()->IgnoreParenCasts();
  if (!ConsumerInstance->isValidExpr(Lhs))
    return true;

  const Expr *Rhs = BO->getRHS()->IgnoreParenCasts();
  if (!ConsumerInstance->isValidExpr(Rhs)) {
    ConsumerInstance->invalidateExpr(Lhs);
    return true;
  }
  
  ConsumerInstance->updateExpr(Lhs, Rhs);
  return true;
}

bool CopyPropCollectionVisitor::VisitDeclRefExpr(DeclRefExpr *DRE)
{
  return true;
}

bool CopyPropCollectionVisitor::VisitMemberExpr(MemberExpr *ME)
{
  return true;
}

bool 
CopyPropCollectionVisitor::VisitArraySubscriptExpr(ArraySubscriptExpr *ASE)
{
  return true;
}

void CopyPropagation::Initialize(ASTContext &context) 
{
  Context = &context;
  SrcManager = &Context->getSourceManager();
  CollectionVisitor = new CopyPropCollectionVisitor(this);
  TheRewriter.setSourceMgr(Context->getSourceManager(), 
                           Context->getLangOptions());
}

void CopyPropagation::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I)
    CollectionVisitor->TraverseDecl(*I);
}
 
void CopyPropagation::HandleTranslationUnit(ASTContext &Ctx)
{
  doAnalysis();
  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void CopyPropagation::doAnalysis(void)
{
  // TODO
}

bool CopyPropagation::isValidExpr(const Expr *E)
{
  return (dyn_cast<DeclRefExpr>(E) ||
          dyn_cast<MemberExpr>(E) ||
          dyn_cast<ArraySubscriptExpr>(E));
}

void CopyPropagation::updateExpr(const Expr *E, const Expr *CopyE)
{
  switch (E->getStmtClass()) {
  case Expr::DeclRefExprClass: {
    const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E);
    const ValueDecl *OrigDecl = DRE->getDecl();
    const VarDecl *VD = dyn_cast<VarDecl>(OrigDecl);

    TransAssert(VD && "Bad VD!");
    VarToExpr[VD] = CopyE;
    return;
  }

  case Expr::MemberExprClass: {
    const MemberExpr *ME = dyn_cast<MemberExpr>(E);
    MemberExprToExpr[ME] = CopyE;
    return;
  }

  case Expr::ArraySubscriptExprClass: {
    const ArraySubscriptExpr *ASE = dyn_cast<ArraySubscriptExpr>(E);
    ArraySubToExpr[ASE] = CopyE;
    return;
  }

  default: 
    TransAssert(0 && "Uncatched Expr!");
  }

  TransAssert(0 && "Unreachable code!");
}

void CopyPropagation::invalidateExpr(const Expr *E)
{
  updateExpr(E, NULL);
}

CopyPropagation::~CopyPropagation(void)
{
  if (CollectionVisitor)
    delete CollectionVisitor;
}
