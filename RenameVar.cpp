#include "RenameVar.h"

#include <algorithm>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"
#include "RewriteUtils.h"

using namespace clang;
using namespace llvm;

static const char *DescriptionMsg =
"To increase readability, rename global and local variables \
to a, b, ..., z. The transformation returns with error message \
if there are more than 26 different variables. \n";

static RegisterTransformation<RenameVar>
         Trans("rename-var", DescriptionMsg);

class RNVCollectionVisitor : public RecursiveASTVisitor<RNVCollectionVisitor> {
public:

  explicit RNVCollectionVisitor(RenameVar *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitVarDecl(VarDecl *VD);

private:

  RenameVar *ConsumerInstance;

};

class RenameVarVisitor : public RecursiveASTVisitor<RenameVarVisitor> {
public:

  explicit RenameVarVisitor(RenameVar *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitVarDecl(VarDecl *VD);

  bool VisitDeclRefExpr(DeclRefExpr *DRE);

private:

  RenameVar *ConsumerInstance;

};

bool RNVCollectionVisitor::VisitVarDecl(VarDecl *VD)
{
  ParmVarDecl *PV = dyn_cast<ParmVarDecl>(VD);
  // Skip parameters
  if (PV)
    return true;

  VarDecl *CanonicalVD = VD->getCanonicalDecl();
  ConsumerInstance->addVar(CanonicalVD);
  return true;
}

bool RenameVarVisitor::VisitVarDecl(VarDecl *VD)
{
  VarDecl *CanonicalDecl = VD->getCanonicalDecl();
  llvm::DenseMap<VarDecl *, char>::iterator I = 
    ConsumerInstance->VarToNameMap.find(CanonicalDecl);

  if (I == ConsumerInstance->VarToNameMap.end())
    return true;

  std::string Name(1, (*I).second);
  return RewriteUtils::replaceVarDeclName(VD, Name, 
           &ConsumerInstance->TheRewriter, ConsumerInstance->SrcManager);
}

bool RenameVarVisitor::VisitDeclRefExpr(DeclRefExpr *DRE)
{
  ValueDecl *OrigDecl = DRE->getDecl();
  VarDecl *VD = dyn_cast<VarDecl>(OrigDecl);
  if (!VD)
    return true;

  VarDecl *CanonicalDecl = VD->getCanonicalDecl();
  llvm::DenseMap<VarDecl *, char>::iterator I = 
    ConsumerInstance->VarToNameMap.find(CanonicalDecl);

  if (I == ConsumerInstance->VarToNameMap.end())
    return true;

  std::string Name(1, (*I).second);
  return RewriteUtils::replaceExpr(DRE, Name, 
           &ConsumerInstance->TheRewriter, ConsumerInstance->SrcManager);
}

void RenameVar::Initialize(ASTContext &context) 
{
  Context = &context;
  SrcManager = &Context->getSourceManager();
  VarCollectionVisitor = new RNVCollectionVisitor(this);
  RenameVisitor = new RenameVarVisitor(this);
  TheRewriter.setSourceMgr(Context->getSourceManager(), 
                           Context->getLangOptions());

  for (char C = 'z'; C >= 'a'; C--) {
    AvailableNames.push_back(C);
  }
  ValidInstanceNum = 1;
}

void RenameVar::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    VarCollectionVisitor->TraverseDecl(*I);
  }
}
 
void RenameVar::HandleTranslationUnit(ASTContext &Ctx)
{
  unsigned int NumNames = AvailableNames.size();
  unsigned int NumVars = ValidVars.size();

  if ((NumVars == 0) || (NumVars > NumNames))
    ValidInstanceNum = 0;

  if (QueryInstanceOnly) {
    return;
  }

  if (NumVars > NumNames) {
    TransError = TransMaxVarsError;
    return;
  }
  else if (NumVars == 0) {
    TransError = TransNoValidVarsError;
    return;
  }
  else if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  TransAssert(RenameVisitor && "NULL RenameVisitor!");
  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  collectVars();
  RenameVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void RenameVar::addVar(VarDecl *VD)
{
  std::string Name = VD->getNameAsString();
  if (Name.size() > 1) {
    ValidVars.push_back(VD);
    return;
  }

  char C = Name[0];
  if ((C < 'a') || (C > 'z'))
    return;

  SmallVector<char, 26>::iterator I = 
    std::find(AvailableNames.begin(), AvailableNames.end(), C);

  // It could be the case where C has been erased, e.g.,
  // two variables from different scopes have the same name
  if (I == AvailableNames.end())
    return;    

  AvailableNames.erase(I);
}

void RenameVar::collectVars(void)
{
  for (std::vector<VarDecl*>::iterator I = ValidVars.begin(),
       E = ValidVars.end(); I != E; ++I) {
    VarDecl *VD = (*I);
    char Name = AvailableNames.back();
    AvailableNames.pop_back();
    VarToNameMap[VD] = Name;
  }
}

RenameVar::~RenameVar(void)
{
  if (VarCollectionVisitor)
    delete VarCollectionVisitor;
  if (RenameVisitor)
    delete RenameVisitor;
}

