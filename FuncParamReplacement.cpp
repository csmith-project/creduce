#include "FuncParamReplacement.h"

#include <iostream>
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Rewrite/Rewriter.h"

#include "TransformationManager.h"

using namespace clang;

static RegisterTransformation<FuncParamReplacement> Trans("func-param-replacement");

class FPRASTVisitor : public RecursiveASTVisitor<FPRASTVisitor> {
public:
    typedef RecursiveASTVisitor<FPRASTVisitor> Inherited;

    bool TraverseCallExpr(CallExpr *E) {
        return true;
    }

};

class FPRASTConsumer : public ASTConsumer {
public:

  FPRASTConsumer(int Counter) 
    : TransformationCounter(Counter)
  { }

  virtual void Initialize(ASTContext &context) {
    Context = &context;
    TransformationASTVisitor = new FPRASTVisitor();
    TheRewriter.setSourceMgr(Context->getSourceManager(), Context->getLangOptions());
    ValidInstanceNum = 0;
    TransFailed = false;
  }

  bool isTransFailed(void) {
    return TransFailed;
  }

  virtual void HandleTopLevelDecl(DeclGroupRef D) {
    for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
      FunctionDecl *FD = dyn_cast<FunctionDecl>(*I);
      if (FD && isValidFuncDecl(FD->getCanonicalDecl())) {
        ValidFuncDecls.push_back(FD);
      }
    }
  }

  virtual void HandleTranslationUnit(ASTContext &Ctx) {
    if (TransformationCounter > ValidInstanceNum) {
        TransFailed = true;
        return;
    }

    assert(TransformationASTVisitor && "NULL TransformationASTVisitor!");
    Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

    TransformationASTVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
    std::cout << "Instance Num: " << ValidInstanceNum << "\n";
    TransFailed = 
      (Ctx.getDiagnostics().hasErrorOccurred() ||
       Ctx.getDiagnostics().hasFatalErrorOccurred());
  }

  ~FPRASTConsumer(void) {
    delete TransformationASTVisitor;
  }

private:

  SmallVector<FunctionDecl *, 10> ValidFuncDecls;

  ASTContext *Context;

  FPRASTVisitor *TransformationASTVisitor;

  Rewriter TheRewriter;

  int ValidInstanceNum;

  const int TransformationCounter;

  bool TransFailed;

  bool isValidFuncDecl(const FunctionDecl *FD);
};

bool FPRASTConsumer::isValidFuncDecl(const FunctionDecl *FD) 
{
  bool IsValid = false;
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
      IsValid = true;
    }
  }
  return IsValid;
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

  return (!TransformationASTConsumer->isTransFailed());
}

