//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef TRANSFORMATION_H
#define TRANSFORMATION_H

#include <string>
#include <cstdlib>
#include <cassert>
#include "clang/AST/ASTConsumer.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "RewriteUtils.h"

namespace clang {
  class CompilerInstance;
  class ASTContext;
  class SourceManager;
  class Decl;
  class Expr;
  class ArrayType;
  class InitListExpr;
  class ArraySubscriptExpr;
  class MemberExpr;
  class Type;
  class ConstantArrayType;
  class DeclContext;
  class DeclarationName;
  class NestedNameSpecifier;
  class TemplateSpecializationType;
  class NamedDecl;
  class CXXConstructorDecl;
}

typedef enum {
  TransSuccess = 0,
  TransInternalError,
  TransMaxInstanceError,
  TransMaxVarsError,
  TransMaxClassesError,
  TransNoValidVarsError,
  TransNoValidFunsError,
  TransNoValidParamsError
} TransformationError;

class Transformation : public clang::ASTConsumer {

public:

  Transformation(const char *TransName, const char *Desc)
    : Name(TransName),
      TransformationCounter(-1),
      ValidInstanceNum(0),
      QueryInstanceOnly(false),
      Context(NULL),
      SrcManager(NULL),
      TransError(TransSuccess),
      DescriptionString(Desc),
      RewriteHelper(NULL)
  {
    // Nothing to do
  }

  virtual ~Transformation(void);

  void outputOriginalSource(llvm::raw_ostream &OutStream);

  void outputTransformedSource(llvm::raw_ostream &OutStream);

  void setTransformationCounter(int Counter) {
    TransformationCounter = Counter;
  }

  void setQueryInstanceFlag(bool Flag) {
    QueryInstanceOnly = Flag;
  }

  bool transSuccess(void) {
    return (TransError == TransSuccess);
  }
  
  bool transMaxInstanceError(void) {
    return (TransError == TransMaxInstanceError);
  }

  bool transInternalError(void) {
    return (TransError == TransInternalError);
  }

  std::string &getDescription(void) {
    return DescriptionString;
  }

  void getTransErrorMsg(std::string &ErrorMsg);

  int getNumTransformationInstances(void) {
    return ValidInstanceNum;
  }

  virtual bool skipCounter(void) {
    return false;
  }

protected:

  typedef llvm::SmallVector<unsigned int, 10> IndexVector;

  typedef llvm::SmallVector<const clang::ArrayType *, 10> ArraySubTypeVector;

  typedef llvm::SmallVector<const clang::Expr *, 10> ExprVector;

  unsigned int getArrayDimension(const clang::ArrayType *ArrayTy);

  unsigned int getArrayDimensionAndTypes(const clang::ArrayType *ArrayTy,
                                         ArraySubTypeVector &TyVec);

  virtual void Initialize(clang::ASTContext &context);

  const clang::Type *
    getArrayBaseElemType(const clang::ArrayType *ArrayTy);

  const clang::Expr *
    getArraySubscriptElem(const clang::ArraySubscriptExpr *ASE);

  const clang::Expr *
    ignoreSubscriptExprParenCasts(const clang::Expr *E);

  const clang::Expr *getMemberExprElem(const clang::MemberExpr *ME);

  const clang::Expr *
    getArrayBaseExprAndIdxs(const clang::ArraySubscriptExpr *ASE, 
                            IndexVector &Idxs);

  const clang::Expr *getBaseExprAndIdxExprs(
          const clang::ArraySubscriptExpr *ASE, ExprVector &IdxExprs);

  const clang::Expr *getInitExprByIndex(IndexVector &Idxs,
                                        const clang::InitListExpr *ILE);

  unsigned int getConstArraySize(const clang::ConstantArrayType *CstArrayTy);

  const clang::Expr *getMemberExprBaseExprAndIdxs(const clang::MemberExpr *ME,
                                                  IndexVector &Idx);

  const clang::Expr *getInitExprFromBase(const clang::Expr *BaseE,
                                         IndexVector &Idxs);

  const clang::Expr *getBaseExprAndIdxs(const clang::Expr *E,
                                        IndexVector &Idxs);

  const clang::Type *getBasePointerElemType(const clang::Type *Ty);

  const clang::Type* getBaseType(const clang::Type *T);

  int getIndexAsInteger(const clang::Expr *E);

  bool isCXXMemberExpr(const clang::MemberExpr *ME);

  const clang::FunctionDecl *lookupFunctionDecl(
          clang::DeclarationName &DName, const clang::DeclContext *Ctx);

  const clang::FunctionDecl *lookupFunctionDeclInGlobal(
          clang::DeclarationName &DName, const clang::DeclContext *Ctx);

  const clang::DeclContext *getDeclContextFromSpecifier(
          const clang::NestedNameSpecifier *NNS);

  bool isSpecialRecordDecl(const clang::RecordDecl *RD);

  const clang::CXXRecordDecl *getBaseDeclFromType(const clang::Type *Ty);

  const clang::CXXRecordDecl *getBaseDeclFromTemplateSpecializationType(
        const clang::TemplateSpecializationType *TSTy);

  bool isParameterPack(const clang::NamedDecl *ND);

  unsigned getNumCtorWrittenInitializers(const clang::CXXConstructorDecl &Ctor);

  const std::string Name;

  int TransformationCounter;

  int ValidInstanceNum;

  bool QueryInstanceOnly;

  clang::ASTContext *Context;

  clang::SourceManager *SrcManager;

  clang::Rewriter TheRewriter;

  TransformationError TransError;
  
  std::string DescriptionString;

  RewriteUtils *RewriteHelper;
};

class TransNameQueryVisitor;

class TransNameQueryWrap {
friend class TransNameQueryVisitor;

public:
  explicit TransNameQueryWrap(const std::string &Prefix);

  ~TransNameQueryWrap(void);

  unsigned int getMaxNamePostfix(void) {
    return MaxPostfix;
  }

  bool TraverseDecl(clang::Decl *D);

private:

  std::string NamePrefix;

  unsigned int MaxPostfix;

  TransNameQueryVisitor *NameQueryVisitor;

  // Unimplemented
  TransNameQueryWrap(void);

  TransNameQueryWrap(const TransNameQueryWrap &);

  void operator=(const TransNameQueryWrap &);
};

#endif
