#include "FuncParamReplacement.h"

#include <sstream>
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Remove an integaral/enumaration parameter from the declaration \
of a function. Define the removed parameter as a local variable \
of the same function. Initialze the newly local variable to be \
0. Also, make corresponding changes on all of the call sites of \
the modified function.\n";
 
static RegisterTransformation<FuncParamReplacement> 
         Trans("func-param-replacement", DescriptionMsg);

class FPRASTVisitor : public RecursiveASTVisitor<FPRASTVisitor> {
public:
  typedef RecursiveASTVisitor<FPRASTVisitor> Inherited;

  FPRASTVisitor(FuncParamReplacement *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitCallExpr(CallExpr *E);

  bool VisitFunctionDecl(FunctionDecl *FD);

private:

  FuncParamReplacement *ConsumerInstance;

  bool rewriteFuncDecl(FunctionDecl *FP);

  bool rewriteParam(const ParmVarDecl *PV, 
                    unsigned int NumParams);

  bool rewriteCalleeExpr(CallExpr *CallE);

  bool makeParamAsLocalVar(FunctionDecl *FP,
                           const ParmVarDecl *PV);

};

void FuncParamReplacement::Initialize(ASTContext &context) 
{
  Context = &context;
  SrcManager = &Context->getSourceManager();
  TransformationASTVisitor = new FPRASTVisitor(this);
  TheRewriter.setSourceMgr(Context->getSourceManager(), 
                           Context->getLangOptions());

  TheFuncDecl = NULL;
  TheParamPos = -1;
}

void FuncParamReplacement::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    FunctionDecl *FD = dyn_cast<FunctionDecl>(*I);
    if (FD && isValidFuncDecl(FD->getCanonicalDecl())) {
      ValidFuncDecls.push_back(FD);
    }
  }
}
 
void FuncParamReplacement::HandleTranslationUnit(ASTContext &Ctx)
{
  if (TransformationCounter > ValidInstanceNum) {
      TransError = TransMaxInstanceError;
      return;
  }

  assert(TransformationASTVisitor && "NULL TransformationASTVisitor!");
  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);
  assert(TheFuncDecl && "NULL TheFuncDecl!");
  assert((TheParamPos >= 0) && "Invalid parameter position!");

  TransformationASTVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

bool FPRASTVisitor::rewriteParam(const ParmVarDecl *PV, 
                                 unsigned int NumParams)
{
  SourceRange ParamLocRange = PV->getSourceRange();
  SourceLocation StartLoc = ParamLocRange.getBegin();
  int RangeSize = 
    ConsumerInstance->TheRewriter.getRangeSize(ParamLocRange);

  if (RangeSize == -1)
    return false;

  // The param is the only parameter of the function declaration.
  // Replace it with void
  if ((ConsumerInstance->TheParamPos == 0) && (NumParams == 1)) {
    // Note that ')' is included in ParamLocRange for unnamed parameter
    if (PV->getDeclName())
      return !(ConsumerInstance->TheRewriter.ReplaceText(StartLoc,
                 RangeSize, "void"));
    else
      return !(ConsumerInstance->TheRewriter.ReplaceText(StartLoc,
                 RangeSize - 1, "void"));
  }

  // The param is the last parameter
  if (ConsumerInstance->TheParamPos == static_cast<int>(NumParams - 1)) {
    int Offset = 0;
    const char *StartBuf = 
      ConsumerInstance->SrcManager->getCharacterData(StartLoc);

    assert(StartBuf && "Invalid start buffer!");
    while (*StartBuf != ',') {
      StartBuf--;
      Offset--;
    }

    SourceLocation NewStartLoc = 
      StartLoc.getLocWithOffset(Offset);

    // Note that ')' is included in ParamLocRange for unnamed parameter
    if (PV->getDeclName())
      return !(ConsumerInstance->TheRewriter.RemoveText(NewStartLoc, 
                 RangeSize - Offset));
    else
      return !(ConsumerInstance->TheRewriter.RemoveText(NewStartLoc, 
                 RangeSize - Offset - 1));
  }
 
  // Clang gives inconsistent RangeSize for named and unnamed parameter decls.
  // For example, for the first parameter, 
  //   foo(int, int);  -- RangeSize is 4, i.e., "," is counted
  //   foo(int x, int);  -- RangeSize is 5, i.e., ","is not included
  if (PV->getDeclName()) {
    // We cannot use the code below:
    //   SourceLocation EndLoc = ParamLocRange.getEnd();
    //   const char *EndBuf = 
    //     ConsumerInstance->SrcManager->getCharacterData(EndLoc);
    // Because getEnd() returns the start of the last token if this
    // is a token range. For example, in the above example, 
    // getEnd() points to the start of "x"
    // See the comments on getRangeSize in clang/lib/Rewriter/Rewriter.cpp
    int NewRangeSize = 0;
    const char *StartBuf = 
      ConsumerInstance->SrcManager->getCharacterData(StartLoc);

    while (NewRangeSize < RangeSize) {
      StartBuf++;
      NewRangeSize++;
    }

    assert(StartBuf && "Invalid start buffer!");
    while (*StartBuf != ',') {
      StartBuf++;
      NewRangeSize++;
    }

    return !(ConsumerInstance->TheRewriter.RemoveText(StartLoc, 
                                                      NewRangeSize + 1));
  }
  else {
    return !(ConsumerInstance->TheRewriter.RemoveText(StartLoc, RangeSize));
  }
}

bool FPRASTVisitor::makeParamAsLocalVar(FunctionDecl *FP,
                                        const ParmVarDecl *PV)
{
  Stmt *Body = FP->getBody();
  assert(Body && "NULL body for a function definition!");
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

bool FPRASTVisitor::rewriteFuncDecl(FunctionDecl *FD) 
{
  const ParmVarDecl *PV = 
    FD->getParamDecl(ConsumerInstance->TheParamPos);  

  assert(PV && "Unmatched ParamPos!");
  if (!rewriteParam(PV, FD->getNumParams()))
    return false;

  if (FD->isThisDeclarationADefinition()) {
    if (!makeParamAsLocalVar(FD, PV))
      return false;
  }
  return true;
}

bool FPRASTVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  FunctionDecl *CanonicalFD = FD->getCanonicalDecl();

  if (CanonicalFD == ConsumerInstance->TheFuncDecl)
    return rewriteFuncDecl(FD);

  return true;
}

bool FPRASTVisitor::rewriteCalleeExpr(CallExpr *CallE)
{
  Expr *Arg = CallE->getArg(ConsumerInstance->TheParamPos);
  assert(Arg && "Null arg!");

  SourceRange ArgRange = Arg->getSourceRange();
  int RangeSize = ConsumerInstance->TheRewriter.getRangeSize(ArgRange);

  if (RangeSize == -1)
    return false;

  SourceLocation StartLoc = ArgRange.getBegin();
  unsigned int NumArgs = CallE->getNumArgs();

  if ((ConsumerInstance->TheParamPos == 0) && (NumArgs == 1)) {
    // Note that ')' is included in ParamLocRange
    return !(ConsumerInstance->TheRewriter.RemoveText(ArgRange));
  }

  // The param is the last parameter
  if (ConsumerInstance->TheParamPos == static_cast<int>(NumArgs - 1)) {
    int Offset = 0;
    const char *StartBuf = 
      ConsumerInstance->SrcManager->getCharacterData(StartLoc);

    assert(StartBuf && "Invalid start buffer!");
    while (*StartBuf != ',') {
      StartBuf--;
      Offset--;
    }

    SourceLocation NewStartLoc = 
      StartLoc.getLocWithOffset(Offset);
    return !(ConsumerInstance->TheRewriter.RemoveText(NewStartLoc,
                                                      RangeSize - Offset));
  }

  int NewRangeSize = 0;
  const char *StartBuf =
      ConsumerInstance->SrcManager->getCharacterData(StartLoc);

  assert(StartBuf && "Invalid start buffer!");
  while (NewRangeSize < RangeSize) {
    StartBuf++;
    NewRangeSize++;
  }

  assert(StartBuf && "Invalid start buffer!");
  while (*StartBuf != ',') {
    StartBuf++;
    NewRangeSize++;
  }

  ConsumerInstance->TheRewriter.RemoveText(StartLoc, NewRangeSize + 1);
  return true;
}

bool FPRASTVisitor::VisitCallExpr(CallExpr *CallE) 
{
  FunctionDecl *CalleeDecl = CallE->getDirectCallee();
  if (!CalleeDecl)
    return true;

  if (CalleeDecl->getCanonicalDecl() != ConsumerInstance->TheFuncDecl)
    return true;

  // We now have a correct CallExpr
  return rewriteCalleeExpr(CallE);
}

bool FuncParamReplacement::isValidFuncDecl(FunctionDecl *FD) 
{
  bool IsValid = false;
  int ParamPos = 0;

  assert(isa<FunctionDecl>(FD) && "Must be a FunctionDecl");

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

FuncParamReplacement::~FuncParamReplacement(void)
{
  delete TransformationASTVisitor;
}

