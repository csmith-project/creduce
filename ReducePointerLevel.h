#ifndef REDUCE_POINTER_LEVEL_H
#define REDUCE_POINTER_LEVEL_H

#include <string>
#include "Transformation.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/DenseMap.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class VarDecl;
  class Expr;
  class DeclRefExpr;
}

class PointerLevelCollectionVisitor;
class PointerLevelRewriteVisitor;

class ReducePointerLevel : public Transformation {
friend class PointerLevelCollectionVisitor;
friend class PointerLevelRewriteVisitor;

public:

  ReducePointerLevel(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      RewriteVisitor(NULL),
      MaxIndirectLevel(0),
      TheVarDecl(NULL)
  { }

  ~ReducePointerLevel(void);

private:
  
  typedef llvm::SmallPtrSet<const clang::DeclaratorDecl *, 20> DeclSet;

  typedef llvm::DenseMap<unsigned int, DeclSet *> LevelToDeclMap;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  const clang::DeclRefExpr *getRefDecl(const clang::Expr *Exp);

  void addOneDecl(const clang::DeclaratorDecl *DD, unsigned int IndirectLevel);

  void doAnalysis(void);

  DeclSet VisitedDecls;

  DeclSet ValidDecls;

  DeclSet AddrTakenDecls;

  LevelToDeclMap AllPtrDecls;

  PointerLevelCollectionVisitor *CollectionVisitor;

  PointerLevelRewriteVisitor *RewriteVisitor;

  unsigned int MaxIndirectLevel;

  clang::DeclaratorDecl *TheDecl;

  // Unimplemented
  ReducePointerLevel(void);

  ReducePointerLevel(const ReducePointerLevel &);

  void operator=(const ReducePointerLevel &);
};

#endif
