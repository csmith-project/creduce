//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2015, 2016 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef EMPTY_STRUCT_TO_INT_H
#define EMPTY_STRUCT_TO_INT_H

#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class RecordDecl;
  class FieldDecl;
}

class EmptyStructToIntASTVisitor;
class EmptyStructToIntRewriteVisitor;

class EmptyStructToInt : public Transformation {
friend class EmptyStructToIntASTVisitor;
friend class EmptyStructToIntRewriteVisitor;

public:

  EmptyStructToInt(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      RewriteVisitor(NULL),
      TheRecordDecl(NULL)
  { }

  ~EmptyStructToInt(void);

private:

  typedef llvm::DenseMap<const clang::RecordDecl *, IndexVector *>
    RecordDeclToFieldIdxVectorMap;

  typedef llvm::SetVector<const clang::RecordDecl *> RecordDeclSet;

  typedef llvm::SmallPtrSet<const clang::CXXRecordDecl *, 20> CXXRecordDeclSet;

  typedef llvm::SmallPtrSet<void *, 20> LocPtrSet;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void handleOneRecordDecl(const clang::RecordDecl *RD,
                           const clang::RecordDecl *BaseRD,
                           const clang::FieldDecl *FD,
                           unsigned int Idx);

  void handleOneVarDecl(const clang::VarDecl *VD);

  bool isValidRecordDecl(const clang::RecordDecl *RD);

  bool pointToSelf(const clang::FieldDecl *FD);

  void removeRecordDecls(void);

  void doAnalysis(void);

  const clang::RecordDecl *getBaseRecordDef(const clang::Type *Ty);

  void getInitExprs(const clang::Type *Ty, 
                    const clang::Expr *E,
                    const IndexVector *IdxVec,
                    ExprVector &InitExprs);

  const clang::FieldDecl *getFieldDeclByIdx(const clang::RecordDecl *RD, unsigned int Idx);

  RecordDeclToFieldIdxVectorMap RecordDeclToField;

  RecordDeclSet VisitedRecordDecls;

  CXXRecordDeclSet BaseClassDecls;

  LocPtrSet VisitedLocs;

  EmptyStructToIntASTVisitor *CollectionVisitor;

  EmptyStructToIntRewriteVisitor *RewriteVisitor;

  const clang::RecordDecl *TheRecordDecl;

  // Unimplemented
  EmptyStructToInt(void);

  EmptyStructToInt(const EmptyStructToInt &);

  void operator=(const EmptyStructToInt &);
};

#endif

