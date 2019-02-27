//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2018 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef REMOVE_UNUSED_STRUCT_FIELD_H
#define REMOVE_UNUSED_STRUCT_FIELD_H

#include "Transformation.h"
#include "llvm/ADT/DenseMap.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class FieldDecl;
  class RecordDecl;
  class Type;
  class VarDecl;
}

class RemoveUnusedStructFieldVisitor;
class RemoveUnusedStructFieldRewriteVisitor;

class RemoveUnusedStructField : public Transformation {
friend class RemoveUnusedStructFieldVisitor;
friend class RemoveUnusedStructFieldRewriteVisitor;

public:

  RemoveUnusedStructField(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      RewriteVisitor(NULL),
      TheRecordDecl(NULL),
      TheFieldDecl(NULL),
      NumFields(0),
      IsFirstField(false)
  { }

  ~RemoveUnusedStructField(void);

private:
  
  typedef llvm::DenseMap<const clang::RecordDecl *, IndexVector *>
    RecordDeclToFieldIdxVectorMap;

  typedef llvm::DenseMap<const clang::FieldDecl *, IndexVector *>
    FieldDeclToIdxVectorMap;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  const clang::RecordDecl *getBaseRecordDef(const clang::Type *Ty);

  void handleOneRecordDecl(const clang::RecordDecl *RD, 
                           const clang::RecordDecl *BaseRD, 
                           const clang::FieldDecl *FD, unsigned int Idx);

  void handleOneVarDecl(const clang::VarDecl *VD);

  void setBaseLine(const clang::RecordDecl *RD, const clang::FieldDecl *FD);

  const clang::Expr *getInitExprFromDesignatedInitExpr(
                       const clang::InitListExpr *ILE, int InitListIdx,
                       const clang::FieldDecl *FD);
  void getInitExprs(const clang::Type *Ty, const clang::Expr *E, 
                    const IndexVector *IdxVec, ExprVector &InitExprs);

  const clang::FieldDecl *getFieldDeclByIdx(const clang::RecordDecl *RD, 
                                            unsigned int Idx);

  void removeOneInitExpr(const clang::Expr *E);

  void removeFieldDecl(void);

  RecordDeclToFieldIdxVectorMap RecordDeclToField;

  FieldDeclToIdxVectorMap FieldToIdxVector;

  RemoveUnusedStructFieldVisitor *CollectionVisitor;

  RemoveUnusedStructFieldRewriteVisitor *RewriteVisitor;

  const clang::RecordDecl *TheRecordDecl;

  const clang::FieldDecl *TheFieldDecl;

  unsigned int NumFields;

  bool IsFirstField;

  // Unimplemented
  RemoveUnusedStructField(void);

  RemoveUnusedStructField(const RemoveUnusedStructField &);

  void operator=(const RemoveUnusedStructField &);
};
#endif
