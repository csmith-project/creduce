#ifndef FUNC_PARAM_REPLACEMENT_H
#define FUNC_PARAM_REPLACEMENT_H

#include <string>
#include "llvm/ADT/SmallVector.h"
#include "Transformation.h"

namespace clang {
  class FunctionDecl;
  class DeclGroupRef;
  class ASTContext;
}

class FPRASTVisitor;

class FuncParamReplacement : public Transformation {
friend class FPRASTVisitor;

public:

  FuncParamReplacement(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc)
  { }

  ~FuncParamReplacement(void);

private:
  
  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  bool isValidFuncDecl(clang::FunctionDecl *FD);

  llvm::SmallVector<clang::FunctionDecl *, 10> ValidFuncDecls;

  FPRASTVisitor *TransformationASTVisitor;

  clang::FunctionDecl *TheFuncDecl;

  int TheParamPos;

  // Unimplemented
  FuncParamReplacement(void);

  FuncParamReplacement(const FuncParamReplacement &);

  void operator=(const FuncParamReplacement &);
};

#endif
