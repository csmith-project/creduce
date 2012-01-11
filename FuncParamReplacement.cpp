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

#if 0
    virtual bool VisitNamedDecl(NamedDecl *Decl) {
      // std::cout << "name: " << Decl->getQualifiedNameAsString() << "\n";
      return true;
    }
#endif
};

class FPRASTConsumer : public ASTConsumer {
public:

  FPRASTConsumer(void) { }

  virtual void Initialize(ASTContext &context) {
    Context = &context;
    TheRewriter.setSourceMgr(Context->getSourceManager(), Context->getLangOptions());
    TransformationASTVisitor = new FPRASTVisitor();
  }

  virtual void HandleTopLevelDecl(DeclGroupRef D) {
    for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
      FunctionDecl *FD = dyn_cast<FunctionDecl>(*I);
      if (FD && isValidFuncDecl(FD)) {
        ValidFuncDecls.push_back(FD);
      }
    }
  }

  virtual void HandleTranslationUnit(ASTContext &Ctx) {
    assert(TransformationASTVisitor && "NULL TransformationASTVisitor!");
    Ctx.getDiagnostics().setSuppressAllDiagnostics(false);
    TransformationASTVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
  }

  ~FPRASTConsumer(void) {
    delete TransformationASTVisitor;
  }

private:

  SmallVector<FunctionDecl *, 10> ValidFuncDecls;

  FPRASTVisitor *TransformationASTVisitor;

  Rewriter TheRewriter;

  ASTContext *Context;

  bool isValidFuncDecl(const FunctionDecl *FD);

};

bool FPRASTConsumer::isValidFuncDecl(const FunctionDecl *FD) 
{
  return true;
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
  TransformationASTConsumer = new FPRASTConsumer();

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
  return !(ClangInstance->getDiagnostics().hasErrorOccurred() ||
           ClangInstance->getDiagnostics().hasFatalErrorOccurred());
}

