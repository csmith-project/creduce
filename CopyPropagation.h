#ifndef COPY_PROPAGATION_H
#define COPY_PROPAGATION_H

#include <string>
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/DenseMap.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class Expr;
  class VarDecl;
  class MemberExpr;
  class ArraySubscriptExpr;
}

class CopyPropCollectionVisitor;

class CopyPropagation : public Transformation {
friend class CopyPropCollectionVisitor;

public:

  CopyPropagation(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL)
  { }

  ~CopyPropagation(void);

private:
  
  typedef llvm::DenseMap<const clang::VarDecl *, const clang::Expr *> 
            VarToExprMap;

  typedef llvm::DenseMap<const clang::MemberExpr *, const clang::Expr *>
            MemberExprToExprMap;

  typedef llvm::DenseMap<const clang::ArraySubscriptExpr *, 
                         const clang::Expr *> ArraySubToExprMap;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  bool isValidExpr(const clang::Expr *E);

  void updateExpr(const clang::Expr *E, const clang::Expr *CopyE);

  void invalidateExpr(const clang::Expr *E);
  
  void doAnalysis(void);

  // A mapping from a var to its value at the current processing point
  VarToExprMap VarToExpr;

  // A mapping from a member expr to its value at the current processing point
  MemberExprToExprMap MemberExprToExpr;

  // A mapping from a arraysubscript expr to its value at
  // the current processing point
  ArraySubToExprMap ArraySubToExpr;

  CopyPropCollectionVisitor *CollectionVisitor;

  // Unimplemented
  CopyPropagation(void);

  CopyPropagation(const CopyPropagation &);

  void operator=(const CopyPropagation &);
};
#endif
