#include "ParamToGlobal.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"
#include "RewriteUtils.h"

using namespace clang;

static const char *DescriptionMsg =
"Lift the declaraion of a parameter from a function \
to the global scope. Also rename the lifted parameter \
to avoid possible name conflicts. Currently only support \
lifting integaral/enumeration parameters. \n";

static RegisterTransformation<ParamToGlobal>
         Trans("param-to-global", DescriptionMsg);

class PToGASTVisitor : public RecursiveASTVisitor<PToGASTVisitor> {
public:
  typedef RecursiveASTVisitor<PToGASTVisitor> Inherited;

  PToGASTVisitor(ParamToGlobal *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitCallExpr(CallExpr *E);

  bool VisitFunctionDecl(FunctionDecl *FD);

  bool VisitDeclRefExpr(DeclRefExpr *ParmRefExpr);

private:

  ParamToGlobal *ConsumerInstance;

  bool rewriteFuncDecl(FunctionDecl *FP);

  bool rewriteParam(const ParmVarDecl *PV, 
                    unsigned int NumParams);

  bool rewriteCalleeExpr(CallExpr *CallE);

  bool makeParamAsGlobalVar(FunctionDecl *FP,
                            const ParmVarDecl *PV);

  std::string getNewName(FunctionDecl *FP,
                         const ParmVarDecl *PV);
};

void ParamToGlobal::Initialize(ASTContext &context) 
{
  Context = &context;
  SrcManager = &Context->getSourceManager();
  TransformationASTVisitor = new PToGASTVisitor(this);
  TheRewriter.setSourceMgr(Context->getSourceManager(), 
                           Context->getLangOptions());
}

void ParamToGlobal::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    FunctionDecl *FD = dyn_cast<FunctionDecl>(*I);
    if (FD && isValidFuncDecl(FD->getCanonicalDecl())) {
      ValidFuncDecls.push_back(FD->getCanonicalDecl());
    }
  }
}
 
void ParamToGlobal::HandleTranslationUnit(ASTContext &Ctx)
{
  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  TransAssert(TransformationASTVisitor && "NULL TransformationASTVisitor!");
  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);
  TransAssert(TheFuncDecl && "NULL TheFuncDecl!");
  TransAssert((TheParamPos >= 0) && "Invalid parameter position!");

  TransformationASTVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

bool ParamToGlobal::isValidFuncDecl(FunctionDecl *FD) 
{
  bool IsValid = false;
  int ParamPos = 0;

  TransAssert(isa<FunctionDecl>(FD) && "Must be a FunctionDecl");

  // Skip the case like foo(int, ...), because we cannot remove
  // the "int" there
  if (FD->isVariadic() && (FD->getNumParams() == 1)) {
    return false;
  }

  // Avoid duplications
  if (std::find(ValidFuncDecls.begin(), 
                ValidFuncDecls.end(), FD) != 
      ValidFuncDecls.end())
    return false;

  for (FunctionDecl::param_const_iterator PI = FD->param_begin(),
       PE = FD->param_end(); PI != PE; ++PI) {
    const ParmVarDecl *PV = (*PI);
    QualType PVType = PV->getOriginalType();
    if (PVType.getTypePtr()->isIntegralOrEnumerationType()) {
      ValidInstanceNum++;

      if (ValidInstanceNum == TransformationCounter) {
        TheFuncDecl = FD;
        TheParamPos = ParamPos;
      }

      IsValid = true;
    }
    ParamPos++;
  }
  return IsValid;
}

ParamToGlobal::~ParamToGlobal(void)
{
  if (TransformationASTVisitor)
    delete TransformationASTVisitor;
}

bool PToGASTVisitor::rewriteParam(const ParmVarDecl *PV, 
                                 unsigned int NumParams)
{
  return 
    RewriteUtils::removeParamFromFuncDecl(PV, 
                                          NumParams,
                                          ConsumerInstance->TheParamPos,
                                          &(ConsumerInstance->TheRewriter),
                                          ConsumerInstance->SrcManager);
}

std::string PToGASTVisitor::getNewName(FunctionDecl *FP,
                                       const ParmVarDecl *PV)
{
  std::string NewName;
  NewName = FP->getNameInfo().getAsString();
  NewName += "_";
  NewName += PV->getNameAsString();

  // also backup the new name
  ConsumerInstance->TheNewDeclName = NewName;
  return NewName;
}

bool PToGASTVisitor::makeParamAsGlobalVar(FunctionDecl *FP,
                                          const ParmVarDecl *PV)
{
  std::string GlobalVarStr;

  GlobalVarStr = PV->getType().getAsString();
  GlobalVarStr += " ";
  GlobalVarStr += getNewName(FP, PV);
  GlobalVarStr += ";\n";

  SourceRange FuncRange = FP->getSourceRange();
  SourceLocation StartLoc = FuncRange.getBegin();
  return !(ConsumerInstance->TheRewriter.InsertText(StartLoc, 
                                                    GlobalVarStr,
                                                    false));
}

bool PToGASTVisitor::rewriteFuncDecl(FunctionDecl *FD) 
{
  const ParmVarDecl *PV = 
    FD->getParamDecl(ConsumerInstance->TheParamPos);  

  TransAssert(PV && "Unmatched ParamPos!");
  if (!rewriteParam(PV, FD->getNumParams()))
    return false;

  if (FD->isThisDeclarationADefinition()) {
    ConsumerInstance->TheParmVarDecl = PV;
    if (!makeParamAsGlobalVar(FD, PV))
      return false;
  }
  return true;
}

bool PToGASTVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  FunctionDecl *CanonicalFD = FD->getCanonicalDecl();

  if (CanonicalFD == ConsumerInstance->TheFuncDecl)
    return rewriteFuncDecl(FD);

  return true;
}

bool PToGASTVisitor::rewriteCalleeExpr(CallExpr *CallE)
{
  return 
    RewriteUtils::removeArgFromCallExpr(CallE, 
                                        ConsumerInstance->TheParamPos,
                                        &(ConsumerInstance->TheRewriter),
                                        ConsumerInstance->SrcManager);
}

bool PToGASTVisitor::VisitCallExpr(CallExpr *CallE) 
{
  FunctionDecl *CalleeDecl = CallE->getDirectCallee();
  if (!CalleeDecl)
    return true;

  if (CalleeDecl->getCanonicalDecl() != ConsumerInstance->TheFuncDecl)
    return true;

  // We now have a correct CallExpr
  return rewriteCalleeExpr(CallE);
}

bool PToGASTVisitor::VisitDeclRefExpr(DeclRefExpr *ParmRefExpr)
{
  const ValueDecl *OrigDecl = ParmRefExpr->getDecl();

  if (!ConsumerInstance->TheParmVarDecl)
    return true;

  if (OrigDecl != ConsumerInstance->TheParmVarDecl)
    return true;

  SourceRange ExprRange = ParmRefExpr->getSourceRange();
  return 
    !(ConsumerInstance->TheRewriter.ReplaceText(ExprRange,
        ConsumerInstance->TheNewDeclName));
}

