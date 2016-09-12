//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2016 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef UNION_TO_STRUCT_H
#define UNION_TO_STRUCT_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class DeclaratorDecl;
  class VarDecl;
  class RecordDecl;
  class DeclStmt;
  class FieldDecl;
}

class UnionToStructCollectionVisitor;

class UnionToStruct : public Transformation {
friend class UnionToStructCollectionVisitor;

public:

  UnionToStruct(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      TheRecordDecl(NULL),
      TheDeclaratorSet(NULL)
  { }

  ~UnionToStruct(void);

private:
  
  typedef llvm::SmallPtrSet<const clang::DeclaratorDecl *, 10> 
    DeclaratorDeclSet;

  typedef llvm::DenseMap<const clang::VarDecl *, const clang::DeclStmt *> 
            VarToDeclStmtMap;

  typedef llvm::DenseMap<const clang::VarDecl *, clang::DeclGroupRef> 
            VarToDeclGroupMap;

  typedef llvm::MapVector<const clang::RecordDecl *, DeclaratorDeclSet *> 
            RecordDeclToDeclaratorDeclMap;

  virtual void Initialize(clang::ASTContext &context);

  virtual bool HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void addOneDeclarator(const clang::DeclaratorDecl *VD, const clang::Type *T);

  void addOneRecord(const clang::RecordDecl *RD);

  void doAnalysis(void);

  bool isValidRecordDecl(const clang::RecordDecl *RD);

  void rewriteRecordDecls(void);

  void rewriteDeclarators(void);

  void rewriteOneRecordDecl(const clang::RecordDecl *RD);

  void rewriteOneVarDecl(const clang::VarDecl *VD);

  void rewriteOneFieldDecl(const clang::FieldDecl *FD);

  void rewriteOneFunctionDecl(const clang::FunctionDecl *FD);

  void getInitStrWithPointerType(const clang::Expr *E, std::string &Str);

  void getInitStrWithNonPointerType(const clang::Expr *E, std::string &Str);

  bool isTheFirstDecl(const clang::VarDecl *VD);

  VarToDeclStmtMap VarToDeclStmt;

  VarToDeclGroupMap VarToDeclGroup;

  RecordDeclToDeclaratorDeclMap RecordToDeclarator;

  UnionToStructCollectionVisitor *CollectionVisitor;

  const clang::RecordDecl *TheRecordDecl;

  DeclaratorDeclSet *TheDeclaratorSet;

  // Unimplemented
  UnionToStruct(void);

  UnionToStruct(const UnionToStruct &);

  void operator=(const UnionToStruct &);
};
#endif
