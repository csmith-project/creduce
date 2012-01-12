#include "FuncParamReplacement.h"

#include <iostream>
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Rewrite/Rewriter.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static RegisterTransformation<FuncParamReplacement> 
         Trans("func-param-replacement");

class FPRASTVisitor : public RecursiveASTVisitor<FPRASTVisitor> {
public:
  typedef RecursiveASTVisitor<FPRASTVisitor> Inherited;

  FPRASTVisitor(FPRASTConsumer *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitCallExpr(CallExpr *E) {
    return true;
  }

  bool VisitFunctionDecl(FunctionDecl *FD);

private:
  FPRASTConsumer *ConsumerInstance;

  bool rewriteFuncDecl(FunctionDecl *FP);

  bool rewriteParam(const ParmVarDecl *PV, 
                    unsigned int NumParams);

  void insertAndRemove(Rewriter &RW, 
                       SourceRange OrigRange,
                       const char *ReplacementStr);
};

class FPRASTConsumer : public ASTConsumer {
  friend class FPRASTVisitor;

public:

  FPRASTConsumer(int Counter) 
    : TransformationCounter(Counter)
  { }

  virtual void Initialize(ASTContext &context) {
    Context = &context;
    SrcManager = &Context->getSourceManager();
    TransformationASTVisitor = new FPRASTVisitor(this);
    TheRewriter.setSourceMgr(Context->getSourceManager(), 
                             Context->getLangOptions());

    ValidInstanceNum = 0;
    TransError = TransSuccess;
    TheFuncDecl = NULL;
    TheParamPos = -1;
  }

  bool transSuccess(void) {
    return (TransError == TransSuccess);
  }
  
  bool transInternalError(void) {
    return (TransError == TransInternalError);
  }

  virtual void HandleTopLevelDecl(DeclGroupRef D) {
    for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
      FunctionDecl *FD = dyn_cast<FunctionDecl>(*I);
      if (FD && isValidFuncDecl(FD->getCanonicalDecl())) {
        ValidFuncDecls.push_back(FD);
      }
    }
  }

  virtual void HandleTranslationUnit(ASTContext &Ctx);

  void outputTransformedSource(void);

  void outputOriginalSource(void);

  ~FPRASTConsumer(void) {
    delete TransformationASTVisitor;
  }

private:

  SmallVector<FunctionDecl *, 10> ValidFuncDecls;

  ASTContext *Context;

  SourceManager *SrcManager;

  FPRASTVisitor *TransformationASTVisitor;

  Rewriter TheRewriter;

  int ValidInstanceNum;

  const int TransformationCounter;

  TransformationError TransError;

  FunctionDecl *TheFuncDecl;

  int TheParamPos;

  bool isValidFuncDecl(FunctionDecl *FD);
};
 
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
    // Note that ')' is included in ParamLocRange
    // insertAndRemove(ConsumerInstance->TheRewriter, ParamLocRange, "void");
    return ConsumerInstance->TheRewriter.ReplaceText(StartLoc, 
                                                     RangeSize - 1, "void");
  }

  // The param is the last parameter
  if (ConsumerInstance->TheParamPos == static_cast<int>(NumParams - 1)) {
    int offset = 0;
    const char *StartBuf = 
      ConsumerInstance->SrcManager->getCharacterData(StartLoc);

    assert(StartBuf && "Invalid start buffer!");
    while (*StartBuf != ',') {
      StartBuf--;
      offset--;
    }

    SourceLocation NewStartLoc = 
      StartLoc.getLocWithOffset(offset);
    return ConsumerInstance->TheRewriter.RemoveText(NewStartLoc, 
                                                    RangeSize - offset - 1);
  }
 
  // The param is in the middle
  return ConsumerInstance->TheRewriter.RemoveText(StartLoc, RangeSize);
}

bool FPRASTVisitor::rewriteFuncDecl(FunctionDecl *FD) 
{
  const ParmVarDecl *PV = NULL;
  int ParamPos = -1;

  // Get the parameter to replace
  for (FunctionDecl::param_const_iterator PI = FD->param_begin(),
       PE = FD->param_end(); PI != PE; ++PI) {
    ParamPos++;
    if (ParamPos == ConsumerInstance->TheParamPos) {
      PV = (*PI);
      break;
    }
  }

  assert(PV && "Unmatched ParamPos!");
  if (!rewriteParam(PV, FD->getNumParams()))
    return false;

  if (FD->isThisDeclarationAsDefinition()) {

  }
  return true;
}

bool FPRASTVisitor::VisitFunctionDecl(FunctionDecl *FD) {
  FunctionDecl *CanonicalFD = FD->getCanonicalDecl();

  if (CanonicalFD == ConsumerInstance->TheFuncDecl)
    return rewriteFuncDecl(FD);

  return true;
}

void FPRASTConsumer::HandleTranslationUnit(ASTContext &Ctx)
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

bool FPRASTConsumer::isValidFuncDecl(FunctionDecl *FD) 
{
  bool IsValid = false;
  int ParamPos = 0;

  assert(isa<FunctionDecl>(FD) && "Must be a FunctionDecl");

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

void FPRASTConsumer::outputTransformedSource(void)
{
  FileID MainFileID = SrcManager->getMainFileID();
  const RewriteBuffer *RWBuf = TheRewriter.getRewriteBufferFor(MainFileID);
  assert(RWBuf && "Empty RewriteBuffer!");
  llvm::outs() << std::string(RWBuf->begin(), RWBuf->end());
}

void FPRASTConsumer::outputOriginalSource(void)
{
  llvm::outs() << "No Change!\n";
}

FuncParamReplacement::FuncParamReplacement(const char *TransName)
  : Transformation(TransName),
    TransformationASTConsumer(NULL)
{
  // Nothing to do
}

FuncParamReplacement::~FuncParamReplacement(void)
{
  // Nothing to do
}

void FuncParamReplacement::initializeTransformation(void)
{
  assert(ClangInstance && "NULL ClangInstance!");
  // Freed with deleting ClangInstance
  TransformationASTConsumer = new FPRASTConsumer(TransformationCounter);

  assert(TransformationASTConsumer && "NULL TransformationASTConsumer!");
  assert(!ClangInstance->hasSema() && "Cannot have Sema!");

  ClangInstance->setASTConsumer(TransformationASTConsumer);
  ClangInstance->createSema(TU_Complete, 0);

  ClangInstance->getDiagnostics().setSuppressAllDiagnostics(true);
}

bool FuncParamReplacement::doTransformation(void)
{
  ParseAST(ClangInstance->getSema());

  ClangInstance->getDiagnosticClient().EndSourceFile();

  if (TransformationASTConsumer->transSuccess()) {
    TransformationASTConsumer->outputTransformedSource();
    return true;
  }
  else if (TransformationASTConsumer->transInternalError()) {
    TransformationASTConsumer->outputOriginalSource();
    return true;
  }
  else {
    return false;
  }
}

