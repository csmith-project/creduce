#include "ParamToLocal.h"

#include <sstream>
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"
#include "RewriteUtils.h"

using namespace clang;

static const char *DescriptionMsg =
"Remove an integaral/enumeration parameter from the declaration \
of a function. Define the removed parameter as a local variable \
of the same function. Initialze the newly local variable to be \
0. Also, make corresponding changes on all of the call sites of \
the modified function.\n";
 
static RegisterTransformation<ParamToLocal> 
         Trans("param-to-local", DescriptionMsg);

class PToLASTVisitor : public RecursiveASTVisitor<PToLASTVisitor> {
public:
  typedef RecursiveASTVisitor<PToLASTVisitor> Inherited;

  PToLASTVisitor(ParamToLocal *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitCallExpr(CallExpr *E);

  bool VisitFunctionDecl(FunctionDecl *FD);

private:

  ParamToLocal *ConsumerInstance;

  bool rewriteFuncDecl(FunctionDecl *FP);

  bool rewriteParam(const ParmVarDecl *PV, 
                    unsigned int NumParams);

  bool rewriteCalleeExpr(CallExpr *CallE);

  bool makeParamAsLocalVar(FunctionDecl *FP,
                           const ParmVarDecl *PV);

};

void ParamToLocal::Initialize(ASTContext &context) 
{
  Context = &context;
  SrcManager = &Context->getSourceManager();
  TransformationASTVisitor = new PToLASTVisitor(this);
  TheRewriter.setSourceMgr(Context->getSourceManager(), 
                           Context->getLangOptions());
}

void ParamToLocal::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    FunctionDecl *FD = dyn_cast<FunctionDecl>(*I);
    if (FD && isValidFuncDecl(FD->getCanonicalDecl())) {
      ValidFuncDecls.push_back(FD->getCanonicalDecl());
    }
  }
}
 
void ParamToLocal::HandleTranslationUnit(ASTContext &Ctx)
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

bool ParamToLocal::isValidFuncDecl(FunctionDecl *FD) 
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

ParamToLocal::~ParamToLocal(void)
{
  if (TransformationASTVisitor)
    delete TransformationASTVisitor;
}

bool PToLASTVisitor::rewriteParam(const ParmVarDecl *PV, 
                                 unsigned int NumParams)
{
  return 
    RewriteUtils::removeParamFromFuncDecl(PV, 
                                          NumParams,
                                          ConsumerInstance->TheParamPos,
                                          &(ConsumerInstance->TheRewriter),
                                          ConsumerInstance->SrcManager);
}

bool PToLASTVisitor::makeParamAsLocalVar(FunctionDecl *FP,
                                        const ParmVarDecl *PV)
{
  Stmt *Body = FP->getBody();
  TransAssert(Body && "NULL body for a function definition!");
  std::string LocalVarStr;

  LocalVarStr = " ";
  LocalVarStr += PV->getType().getAsString();
  LocalVarStr += " ";
  LocalVarStr += PV->getNameAsString();
  LocalVarStr += " = 0";
  LocalVarStr += ";";

  SourceLocation StartLoc = Body->getLocStart();
  return !(ConsumerInstance->TheRewriter.InsertTextAfterToken(StartLoc, 
                                                              LocalVarStr));
}

bool PToLASTVisitor::rewriteFuncDecl(FunctionDecl *FD) 
{
  const ParmVarDecl *PV = 
    FD->getParamDecl(ConsumerInstance->TheParamPos);  

  TransAssert(PV && "Unmatched ParamPos!");
  if (!rewriteParam(PV, FD->getNumParams()))
    return false;

  if (FD->isThisDeclarationADefinition()) {
    if (!makeParamAsLocalVar(FD, PV))
      return false;
  }
  return true;
}

bool PToLASTVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  FunctionDecl *CanonicalFD = FD->getCanonicalDecl();

  if (CanonicalFD == ConsumerInstance->TheFuncDecl)
    return rewriteFuncDecl(FD);

  return true;
}

bool PToLASTVisitor::rewriteCalleeExpr(CallExpr *CallE)
{
  return 
    RewriteUtils::removeArgFromCallExpr(CallE, 
                                        ConsumerInstance->TheParamPos,
                                        &(ConsumerInstance->TheRewriter),
                                        ConsumerInstance->SrcManager);
}

bool PToLASTVisitor::VisitCallExpr(CallExpr *CallE) 
{
  FunctionDecl *CalleeDecl = CallE->getDirectCallee();
  if (!CalleeDecl)
    return true;

  if (CalleeDecl->getCanonicalDecl() != ConsumerInstance->TheFuncDecl)
    return true;

  // We now have a correct CallExpr
  return rewriteCalleeExpr(CallE);
}

