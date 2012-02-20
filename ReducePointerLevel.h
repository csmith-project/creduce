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
  class RecordType;
  class InitListExpr;
  class ArraySubscriptExpr;
  class MemberExpr;
  class UnaryOperator;
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

  typedef void (ReducePointerLevel::*InitListHandler)(const clang::Expr *Init,
                                                      std::string &InitStr);

  typedef llvm::SmallVector<unsigned int, 10> IndexVector;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void getInitListExprString(const clang::InitListExpr *ILE,
                             std::string &InitStr,
                             InitListHandler Handler);

  const clang::DeclaratorDecl *getRefDecl(const clang::Expr *Exp);

  const clang::DeclaratorDecl *
          getCanonicalDeclaratorDecl(const clang::Expr *E);

  const clang::Expr *
          ignoreSubscriptExprParenCasts(const clang::Expr *E);

  const clang::Type *getArrayBaseElemType(const clang::ArrayType *ArrayTy);

  const clang::Expr *getArrayBaseExprAndIdxs(
          const clang::ArraySubscriptExpr *ASE, IndexVector &Idxs);

  const clang::Expr *getArraySubscriptElem(
          const clang::ArraySubscriptExpr *ASE);

  const clang::Expr *getInitExprByIndex(IndexVector &Idxs,
                                        const clang::InitListExpr *ILE);

  const clang::Expr *getMemberExprBaseExprAndIdxs(const clang::MemberExpr *ME,
                                                  IndexVector &Idx);

  const clang::Expr *getInitExprFromBase(const clang::Expr *BaseE,
                                         IndexVector &Idxs);

  const clang::Expr *getMemberExprElem(const clang::MemberExpr *ME);

  unsigned int getArrayDimension(const clang::ArrayType *ArrayTy);

  void addOneDecl(const clang::DeclaratorDecl *DD, int IndirectLevel);

  void doAnalysis(void);

  void setRecordDecl(void);

  const clang::RecordType *getRecordType(const clang::Type *T);

  void getNewLocalInitStr(const clang::Expr *Init, 
                          std::string &InitStr);

  void getNewGlobalInitStr(const clang::Expr *Init, 
                           std::string &InitStr);

  const clang::Expr *getFirstInitListElem(const clang::InitListExpr *E);

  void copyInitStr(const clang::Expr *Exp, std::string &InitStr);

  void rewriteVarDecl(const clang::VarDecl *VD);

  void rewriteFieldDecl(const clang::FieldDecl *FD);

  void rewriteRecordInit(const clang::RecordDecl *RD,
                         const clang::Expr *Init);

  void rewriteArrayInit(const clang::RecordDecl *RD,
                         const clang::Expr *Init);

  void rewriteDerefOp(const clang::UnaryOperator *UO);

  void rewriteDeclRefExpr(const clang::DeclaratorDecl *DD);

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
