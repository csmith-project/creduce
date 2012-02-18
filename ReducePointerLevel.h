#ifndef REDUCE_POINTER_LEVEL_H
#define REDUCE_POINTER_LEVEL_H

#include <string>
#include "Transformation.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/DenseMap.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class Expr;
  class DeclaratorDecl;
  class RecordDecl;
  class FieldDecl;
  class VarDecl;
  class Type;
  class ArrayType;
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
      TheDecl(NULL),
      TheRecordDecl(NULL)
  { }

  ~ReducePointerLevel(void);

private:
  
  typedef llvm::SmallPtrSet<const clang::DeclaratorDecl *, 20> DeclSet;

  typedef llvm::DenseMap<int, DeclSet *> LevelToDeclMap;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  const clang::DeclaratorDecl *getRefDecl(const clang::Expr *Exp);

  const clang::DeclaratorDecl *
          getCanonicalDeclaratorDecl(const clang::Expr *E);

  const clang::Expr *
          ignoreSubscriptExprParenCasts(const clang::Expr *E);

  const clang::Type *getArrayBaseElemType(const clang::ArrayType *ArrayTy);

  void addOneDecl(const clang::DeclaratorDecl *DD, int IndirectLevel);

  void doAnalysis(void);

  void setRecordDecl(void);

  void rewriteVarDecl(const clang::VarDecl *VD);

  void rewriteFieldDecl(const clang::FieldDecl *FD);

  void rewriteRecordInit(const clang::RecordDecl *RD,
                         const clang::Expr *Init);

  void rewriteArrayInit(const clang::RecordDecl *RD,
                         const clang::Expr *Init);

  DeclSet VisitedDecls;

  DeclSet ValidDecls;

  DeclSet AddrTakenDecls;

  LevelToDeclMap AllPtrDecls;

  PointerLevelCollectionVisitor *CollectionVisitor;

  PointerLevelRewriteVisitor *RewriteVisitor;

  int MaxIndirectLevel;

  const clang::DeclaratorDecl *TheDecl;

  const clang::RecordDecl *TheRecordDecl ;

  // Unimplemented
  ReducePointerLevel(void);

  ReducePointerLevel(const ReducePointerLevel &);

  void operator=(const ReducePointerLevel &);
};

#endif
