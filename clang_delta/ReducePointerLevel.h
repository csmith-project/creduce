//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

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
      TheDecl(NULL),
      TheRecordDecl(NULL)
  { }

  ~ReducePointerLevel(void);

private:
  
  typedef llvm::SmallPtrSet<const clang::DeclaratorDecl *, 20> DeclSet;

  typedef llvm::SmallPtrSet<const clang::DeclRefExpr *, 20> DeclRefExprSet;

  typedef llvm::SmallPtrSet<const clang::MemberExpr *, 20> MemberExprSet;

  typedef llvm::DenseMap<int, DeclSet *> LevelToDeclMap;

  typedef void (ReducePointerLevel::*InitListHandler)(const clang::Expr *Init,
                                                      std::string &InitStr);

  typedef llvm::SmallVector<unsigned int, 10> IndexVector;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void getInitListExprString(const clang::InitListExpr *ILE,
                             std::string &InitStr,
                             InitListHandler Handler);

  const clang::DeclaratorDecl *getRefDecl(const clang::Expr *Exp);

  const clang::DeclRefExpr *getDeclRefExpr(const clang::Expr *Exp);

  const clang::DeclaratorDecl *
          getCanonicalDeclaratorDecl(const clang::Expr *E);

  void addOneDecl(const clang::DeclaratorDecl *DD, int IndirectLevel);

  void doAnalysis(void);

  void setRecordDecl(void);

  void copyInitStr(const clang::Expr *Exp, std::string &InitStr);

  void getNewLocalInitStr(const clang::Expr *Init, std::string &InitStr);

  void getNewGlobalInitStr(const clang::Expr *Init, 
                           std::string &InitStr);

  const clang::Expr *getFirstInitListElem(const clang::InitListExpr *E);

  void rewriteVarDecl(const clang::VarDecl *VD);

  void rewriteFieldDecl(const clang::FieldDecl *FD);

  void rewriteRecordInit(const clang::RecordDecl *RD,
                         const clang::Expr *Init);

  void rewriteArrayInit(const clang::RecordDecl *RD,
                         const clang::Expr *Init);

  void rewriteDerefOp(const clang::UnaryOperator *UO);

  void rewriteDeclRefExpr(const clang::DeclRefExpr *DRE);

  void replaceArrowWithDot(const clang::Expr *E);

  bool isPointerToSelf(const clang::Type *Ty, const clang::DeclaratorDecl *DD);

  void checkPrefixAndPostfix(const clang::UnaryOperator *UO);

  DeclSet VisitedDecls;

  DeclSet ValidDecls;

  DeclSet AddrTakenDecls;

  DeclRefExprSet VisitedDeclRefExprs;

  MemberExprSet VisitedMemberExprs;

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
