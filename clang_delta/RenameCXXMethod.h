//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2014 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef RENAME_CXX_METHOD_H
#define RENAME_CXX_METHOD_H

#include <string>
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class FunctionDecl;
  class CXXMethodDecl;
  class CXXRecordDecl;
  class Expr;
  class CallExpr;
  class OverloadExpr;
  class CXXDependentScopeMemberExpr;
  class DeclarationName;
  class FunctionTemplateDecl;
  class TemplateArgument;
  class ClassTemplateSpecializationDecl;
}

namespace llvm {
  class StringRef;
}

class RenameCXXMethodCollectionVisitor;
class RenameCXXMethodVisitor;

class RenameCXXMethod : public Transformation {
friend class RenameCXXMethodCollectionVisitor;
friend class RenameCXXMethodVisitor;

public:

  RenameCXXMethod(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      MethodCollectionVisitor(NULL),
      RenameVisitor(NULL),
      CurrentFD(NULL),
      ClassInstantiation(false),
      FunctionInstantiation(false),
      DoRenaming(false),
      MethodNamePrefix("m_fn"),
      NumRenamedMethods(0)
  { }

  ~RenameCXXMethod();

  virtual bool skipCounter() {
    return true;
  }

private:

  typedef llvm::DenseMap<const clang::CXXMethodDecl *, std::string> 
    CXXMethodDeclToNameMap;

  typedef llvm::DenseMap<const clang::CXXRecordDecl *, unsigned int> 
    CXXRecordDeclToNumMap;

  typedef llvm::SmallPtrSet<const clang::CXXRecordDecl *, 10> CXXRecordDeclSet;

  typedef llvm::SmallPtrSet<const clang::CXXMethodDecl *, 10> CXXMethodDeclSet;

  typedef llvm::SmallVector<const clang::ClassTemplateSpecializationDecl*, 5>
    ClassSpecDeclVector;

  virtual void Initialize(clang::ASTContext &context);

  virtual bool HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void handleOneCXXRecordDecl(const clang::CXXRecordDecl *RD);

  void addOneMethodName(const clang::CXXMethodDecl *MD, unsigned int Num);

  void addOneInheritedName(const clang::CXXMethodDecl *MD,
                           const clang::CXXMethodDecl *BaseMD);

  bool isSpecialCXXMethod(const clang::CXXMethodDecl *MD);

  unsigned int getNumInheritedFunctions(const clang::CXXRecordDecl *RD);

  void handleOneMemberTemplateFunction(const clang::CXXMethodDecl *MD);

  void rewriteDependentExpr(const clang::Expr *E);

  void rewriteOverloadExpr(const clang::OverloadExpr *OE);

  void rewriteFunctionTemplateExplicitInstantiation(
         const clang::FunctionDecl *FD);

  void rewriteCXXDependentScopeMemberExpr(
       const clang::CXXDependentScopeMemberExpr *DE);

  bool getMethodNewName(const clang::CXXMethodDecl *MD, std::string &NewName);

  bool hasValidMethods();

  bool isExplicit(const clang::CXXMethodDecl *MD);

  const clang::FunctionDecl* getFunctionDecl(
          const clang::CXXDependentScopeMemberExpr *DE);

  const clang::CXXMethodDecl* getCXXMethodFromMemberFunction(
       const clang::CXXMethodDecl *MD);

  const clang::FunctionDecl* getFunctionDeclFromType(
          const clang::Type *Ty, clang::DeclarationName &DName);

  const clang::FunctionDecl* getFunctionDeclFromOverloadExpr(
          const clang::OverloadExpr *OE);

  const clang::FunctionDecl* getFunctionDeclFromReturnType(
          const clang::CallExpr *CE, clang::DeclarationName &DName);

  const clang::FunctionDecl* getFunctionDeclFromOverloadTemplate(
          const clang::CallExpr *CE, const clang::OverloadExpr *OE, 
          clang::DeclarationName &DName);

  bool isValidName(const llvm::StringRef &Name);

  void setClassInstantiationFlag(const clang::RecordDecl *RD);

  void clearClassInstantiationFlag(void) {
    ClassInstantiation = false;
  }

  void setFunctionInstantiationFlag(const clang::FunctionDecl *FD);

  void clearFunctionInstantiationFlag(void) {
    FunctionInstantiation = false;
  }

  RenameCXXMethodCollectionVisitor *MethodCollectionVisitor;

  RenameCXXMethodVisitor *RenameVisitor;

  const clang::FunctionDecl *CurrentFD;

  bool ClassInstantiation;
  
  bool FunctionInstantiation;

  bool DoRenaming;

  const std::string MethodNamePrefix;

  int NumRenamedMethods;

  CXXRecordDeclSet VisitedCXXRecordDecls;

  CXXMethodDeclSet VisitedSpecializedMethods;

  CXXMethodDeclToNameMap NewMethodNames;

  CXXRecordDeclToNumMap NumMemberFunctions;

  ClassSpecDeclVector InstantiationQueue;

  // Unimplemented
  RenameCXXMethod();

  RenameCXXMethod(const RenameCXXMethod &);

  void operator=(const RenameCXXMethod &);
};
#endif
