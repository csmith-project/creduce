#ifndef SIMPLE_INLINER_H
#define SIMPLE_INLINER_H

#include <string>
#include <vector>
#include <utility>
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/DenseMap.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class FunctionDecl;
  class CallExpr;
  class DeclRefExpr;
  class ReturnStmt;
  class Expr;
  class ParmVarDecl;
}

class SimpleInlinerCollectionVisitor;

class SimpleInliner : public Transformation {
friend class SimpleInlinerCollectionVisitor;

public:

  SimpleInliner(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      TheCallExpr(NULL),
      TheCaller(NULL),
      CurrentFD(NULL),
      MaxNumStmts(10)
  { }

  ~SimpleInliner(void);

private:
  
  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void replaceCallExpr(void);

  void doAnalysis(void);

  bool isValidArgExpr(const clang::Expr *E);

  bool hasValidArgExprs(const clang::CallExpr *CE);

  SimpleInlinerCollectionVisitor *CollectionVisitor;

  llvm::DenseMap<clang::CallExpr *, clang::FunctionDecl *> CalleeToCallerMap;

  llvm::SmallVector<clang::CallExpr *, 10> AllCallExprs;

  llvm::SmallSet<clang::FunctionDecl *, 10> ValidFunctionDecls;

  clang::CallExpr *TheCallExpr;

  clang::FunctionDecl *TheCaller;

  clang::FunctionDecl *CurrentFD;

  const unsigned int MaxNumStmts;

  // Unimplemented
  SimpleInliner(void);

  SimpleInliner(const SimpleInliner &);

  void operator=(const SimpleInliner &);
};
#endif
