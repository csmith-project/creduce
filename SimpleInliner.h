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
class SimpleInlinerFunctionVisitor;

class SimpleInliner : public Transformation {
friend class SimpleInlinerCollectionVisitor;
friend class SimpleInlinerFunctionVisitor;

public:

  SimpleInliner(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      FunctionVisitor(NULL),
      NameQueryWrap(NULL),
      TheCallExpr(NULL),
      TheCaller(NULL),
      CurrentFD(NULL),
      MaxNumStmts(10),
      TmpVarName(""),
      NamePostfix(0)
  { }

  ~SimpleInliner(void);

private:
  
  typedef llvm::SmallVector<clang::ReturnStmt *, 5> ReturnStmtsVector;

  typedef llvm::SmallVector<const clang::DeclRefExpr *, 5> ParmRefsVector;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void replaceCallExpr(void);

  void doAnalysis(void);

  bool isValidArgExpr(const clang::Expr *E);

  bool hasValidArgExprs(const clang::CallExpr *CE);

  void createReturnVar(void);

  void generateParamStrings(void);

  std::string getNewTmpName(void);

  SimpleInlinerCollectionVisitor *CollectionVisitor;

  SimpleInlinerFunctionVisitor *FunctionVisitor;

  llvm::DenseMap<clang::CallExpr *, clang::FunctionDecl *> CalleeToCallerMap;

  llvm::SmallVector<clang::CallExpr *, 10> AllCallExprs;

  llvm::SmallSet<clang::FunctionDecl *, 10> ValidFunctionDecls;

  ReturnStmtsVector ReturnStmts;

  ParmRefsVector ParmRefs;

  llvm::SmallVector<std::string, 10> ParmStrings;

  TransNameQueryWrap *NameQueryWrap;

  clang::CallExpr *TheCallExpr;

  clang::FunctionDecl *TheCaller;

  clang::FunctionDecl *CurrentFD;

  const unsigned int MaxNumStmts;

  std::string TmpVarName;

  unsigned int NamePostfix;

  // Unimplemented
  SimpleInliner(void);

  SimpleInliner(const SimpleInliner &);

  void operator=(const SimpleInliner &);
};
#endif
