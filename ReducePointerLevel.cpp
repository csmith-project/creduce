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
with the largest indirect level. This mechanism could \
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

  unsigned int getPointerIndirectLevel(const Type *Ty);

};

class PointerLevelRewriteVisitor : public 
  RecursiveASTVisitor<PointerLevelRewriteVisitor> {

public:

  explicit PointerLevelRewriteVisitor(ReducePointerLevel *Instance)
    : ConsumerInstance(Instance)
  { }

  // bool VisitAssignmentStmt(AssignmentStmt *AS);

  // bool VisitUnaryOperator(UnaryOperator *UO);

private:

  ReducePointerLevel *ConsumerInstance;

};

unsigned int 
PointerLevelCollectionVisitor::getPointerIndirectLevel(const Type *Ty)
{
  unsigned int IndirectLevel = 0;
  QualType QT = Ty->getPointeeType();;
  while (!QT.isNull()) {
    IndirectLevel++;
    QT = QT.getTypePtr()->getPointeeType();
  }
  return IndirectLevel;
}
 
// I skipped IndirectFieldDecl for now
bool PointerLevelCollectionVisitor::VisitDeclaratorDecl(DeclaratorDecl *DD)
{
  // Only consider FieldDecl and VarDecl
  Kind K = DD->getKind();
  if (!(K == Decl::FieldDecl) && !(K == Decl::VarDecl))
    return true;

  const Type *Ty = DD->getType().getTypePtr();
  if (!Ty->isPointerType())
    return true;

  DeclaratorDecl *CanonicalDD = DD->getCanonicalDecl();
  if (ConsumerInstance->VisitedDecls.count(CanonicalDD))
    return true;

  ConsumerInstance->ValidDecls.insert(CanonicalDD);
  ConsumerInstance->VisitedDecls.insert(CanonicalDD);
  unsigned int IndirectLevel = getPointerIndirectLevel(Ty);
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
  DeclaratorDecl *DD = getCanonicalDeclaratorDecl(SubE);
  TransAssert(DD && "NULL DD!");

  ConsumerInstance->AddrTakenDecls.insert(DD);
  return true;
}

bool PointerLevelCollectionVisitor::VisitBinaryOperator(BinaryOperator *BO)
{
  if (!BO->isAssignmentOp() && !BO->isCompoundAssignmentOp())
    return true;

  Expr *Lhs = BO->getLHS();
  const Type *Tp = Lhs->getType().getTypePtr();
  if (!Tp->isPointerType())
    return true;

  Expr *Rhs = BO->getRHS()->IgnoreParenCasts();
  if (dyn_cast<DeclRefExpr>(Rhs) || dyn_cast<UnaryOperator>(Rhs))
    return true;

  DeclaratorDecl *DD = getCanonicalDeclaratorDecl(Lhs);
  TransAssert(DD && "NULL DD!");

  TransAssert(ConsumerInstance->ValidDecls.count(DD) && 
              "DeclaratorDecl doesn't exist!");
  ConsumerInstance->ValidDecls.erase(DD);
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

  RewriteVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void ReducePointerLevel::doAnalysis(void)
{
  // TODO
}

const DeclRefExpr *ReducePointerLevel::getRefDecl(const Expr *Exp)
{
  // FIXME
  const Expr *E = Exp->IgnoreParens();
  const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E);
  if (DRE)
    return DRE;

  const UnaryOperator *DerefE = dyn_cast<UnaryOperator>(DRE);
  TransAssert(DerefE && "Bad DerefE!");
  TransAssert((DerefE->getOpcode() == UO_Deref) && "Non-Deref Opcode!");
  const Expr *SubE = DerefE->getSubExpr();
  return getRefDecl(SubE);
}

void ReducePointerLevel::addOneDecl(const DeclaratorDecl *DD, 
                                       unsigned int IndirectLevel)
{
  DeclSet *DDSet = AllPtrDecls[IndirectLevel];
  if (!DDSet) {
    DDSet = new DeclSet::SmallPtrSet();
    AllPtrDecls[IndirectLevel] = DDSet;
  }
  DDSet->insert(DD);
}

const DeclaratorDecl *
ReducePointerLevel::getCanonicalDeclaratorDecl(const Expr *E)
{
  const DeclaratorDecl *DD;
  const DeclRefExpr *DRE;
  const MemberExpr *ME;

  if ( (DRE = dyn_cast<DeclRef>(E)) ) {
    const ValueDecl *ValueD = DRE->getDecl();
    DD = dyn_cast<DeclaratorDecl>(ValueD);
    TransAssert(DD && "Bad Declarator!"); 
  }
  else if ( (ME = dyn_cast<MemberExpr>(E)) ) {
    ValueDecl *OrigDecl = ME->getMemberDecl();
    FieldDecl *FD = dyn_cast<FieldDecl>(OrigDecl);

    // in C++, getMemberDecl returns a CXXMethodDecl.
    TransAssert(FD && "Unsupported C++ getMemberDecl!\n");
    DD = dyn_cast<DeclaratorDecl>(OrigDecl);
  }

  return DD->getCanonicalDecl();
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

