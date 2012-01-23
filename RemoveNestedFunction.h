#ifndef REMOVE_NESTED_FUNCTION_H
#define REMOVE_NESTED_FUNCTION_H

#include <string>
#include "llvm/ADT/SmallVector.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class FunctionDecl;
  class Stmt;
  class CallExpr;
}

class RNFCollectionVisitor;

class RemoveNestedFunction : public Transformation {
friend class RNFCollectionVisitor;

public:

  RemoveNestedFunction(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      NestedInvocationVisitor(NULL),
      TheFuncDecl(NULL),
      TheStmt(NULL),
      TheCallExpr(NULL),
      TmpVarName(""),
      NeedParen(false)
  { }

  ~RemoveNestedFunction(void);

private:
  
  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  bool addNewTmpVariable(void);

  bool addNewAssignStmt(void);

  bool replaceCallExpr(void);

  void setTmpVarName(std::string &Name) {
    TmpVarName = Name;
  }

  std::string getTmpVarName(void) {
    return TmpVarName;
  }

  clang::SmallVector<clang::CallExpr *, 5> CallExprQueue;

  clang::SmallVector<clang::CallExpr *, 10> ValidCallExprs;

  RNFCollectionVisitor *NestedInvocationVisitor;

  clang::FunctionDecl *TheFuncDecl;

  clang::Stmt *TheStmt;

  clang::CallExpr *TheCallExpr;

  std::string TmpVarName;

  bool NeedParen;

  // Unimplemented
  RemoveNestedFunction(void);

  RemoveNestedFunction(const RemoveNestedFunction &);

  void operator=(const RemoveNestedFunction &);
};
#endif
