//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2014, 2016, 2017 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef REMOVE_UNUSED_FUNCTION_H
#define REMOVE_UNUSED_FUNCTION_H

#include <string>
#include <map>
#include <set>
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallSet.h"
#include "Transformation.h"
#include "clang/Basic/SourceLocation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class FunctionDecl;
  class UsingDecl;
  class DeclarationName;
  class DeclContext;
  class CXXDependentScopeMemberExpr;
  class FunctionTemplateDecl;
  class UnresolvedLookupExpr;
  class NestedNameSpecifier;
  class MemberExpr;
  class CXXOperatorCallExpr;
}

class RUFAnalysisVisitor;
class ExtraReferenceVisitorWrapper;

class RemoveUnusedFunction : public Transformation {
friend class RUFAnalysisVisitor;
friend class ExtraReferenceVisitorWrapper;

public:

  RemoveUnusedFunction(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc, /*MultipleRewrites*/true),
      AnalysisVisitor(NULL),
      VisitorWrapper(NULL),
      TheFunctionDecl(NULL)
  { }

  ~RemoveUnusedFunction();

  void handleOneUsingDecl(const clang::FunctionDecl *CurrentFD, 
                          const clang::UsingDecl *D);

  void handleOneCXXDependentScopeMemberExpr(
         const clang::FunctionDecl *CurrentFD, 
         const clang::CXXDependentScopeMemberExpr *E);

  void handleOneUnresolvedLookupExpr(
         const clang::FunctionDecl *CurrentFD,
         const clang::UnresolvedLookupExpr *E);

  void handleOneMemberExpr(const clang::MemberExpr *ME);

  void handleOneCXXOperatorCallExpr(const clang::CXXOperatorCallExpr *E);

  void handleOneCallExpr(const clang::CallExpr *E);

  void handleOneFunctionDecl(const clang::FunctionDecl *FD);

private:

  typedef llvm::SmallVector<const clang::FunctionDecl *, 1000>
            FunctionDeclVector;

  typedef llvm::DenseMap<const clang::UsingDecl *, 
                         const clang::FunctionDecl *>
            UsingFunctionDeclsMap;

  typedef llvm::SmallPtrSet<const clang::FunctionDecl *, 32>
            FunctionDeclsSet;

  typedef llvm::SmallPtrSet<const clang::FunctionDecl *, 5>
            MemberSpecializationSet;

  typedef llvm::DenseMap<const clang::FunctionDecl *, 
                         MemberSpecializationSet *>
            MemberToSpecializationMap;

  typedef llvm::SmallPtrSet<const clang::UsingDecl *, 32>
            UsingDeclsSet;
  
  typedef std::map<std::string, std::string>
            InlinedSystemFunctionsMap;

  typedef std::set<std::string> SystemFunctionsSet;

  typedef llvm::SmallSet<clang::SourceLocation, 5> LocSet;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void initializeInlinedSystemFunctions();

  void doRewriting();

  bool hasReferencedSpecialization(const clang::FunctionDecl *FD);

  clang::SourceLocation getExtensionLocStart(clang::SourceLocation Loc);

  void removeOneFunctionDecl(const clang::FunctionDecl *FD);

  void removeMemberSpecializations(const clang::FunctionDecl *FD);

  void removeRemainingExplicitSpecs(MemberSpecializationSet *ExplicitSpecs);

  clang::SourceLocation getFunctionOuterLocStart(const clang::FunctionDecl *FD);

  bool hasValidOuterLocStart(const clang::FunctionTemplateDecl *FTD, 
                             const clang::FunctionDecl *FD);

  void removeOneFunctionDeclGroup(const clang::FunctionDecl *FD);

  bool isInReferencedSet(const clang::FunctionDecl *FD);

  bool hasAtLeastOneValidLocation(const clang::FunctionDecl *FD);

  void addOneFunctionDecl(const clang::FunctionDecl *CanonicalFD);

  void addOneMemberSpecialization(const clang::FunctionDecl *FD, 
                                  const clang::FunctionDecl *Member);

  void createFuncToExplicitSpecs(const clang::FunctionDecl *FD);

  void addFuncToExplicitSpecs(const clang::FunctionDecl *FD);

  const clang::FunctionDecl *lookupFunctionDeclShallow(
          const clang::DeclarationName &DName,
          const clang::DeclContext *Ctx);

  const clang::FunctionDecl *getFunctionDeclFromSpecifier(
          const clang::DeclarationName &Name, 
          const clang::NestedNameSpecifier *NNS);

  void addOneReferencedFunction(const clang::FunctionDecl *FD);

  clang::SourceLocation getFunctionLocEnd(clang::SourceLocation LocStart,
                                          clang::SourceLocation LocEnd,
                                          const clang::FunctionDecl *FD);

  bool isTokenOperator(clang::SourceLocation Loc);

  void removeFunctionExplicitInstantiations(const clang::FunctionDecl *FD);

  void removeOneExplicitInstantiation(const clang::FunctionDecl *Spec);

  const clang::FunctionTemplateDecl *getTopDescribedTemplate(
          const clang::FunctionDecl *FD);

  const clang::FunctionDecl *getSourceFunctionDecl(
          const clang::FunctionDecl *TheFD);

  void setInlinedSystemFunctions(const clang::FunctionDecl *FD);

  bool isInlinedSystemFunction(const clang::FunctionDecl *FD);

  UsingFunctionDeclsMap UsingFDs;

  UsingFunctionDeclsMap UsingParentFDs;

  FunctionDeclsSet ReferencedFDs;

  FunctionDeclsSet VisitedFDs;

  FunctionDeclsSet RemovedFDs;

  UsingDeclsSet VisitedUsingDecls;

  MemberToSpecializationMap MemberToSpecs;

  MemberToSpecializationMap FuncToExplicitSpecs;

  MemberToSpecializationMap MemberToInstantiations;

  InlinedSystemFunctionsMap InlinedSystemFunctions;

  SystemFunctionsSet ExistingSystemFunctions;

  LocSet VisitedLocations;

  FunctionDeclVector AllValidFunctionDecls;

  RUFAnalysisVisitor *AnalysisVisitor;

  ExtraReferenceVisitorWrapper *VisitorWrapper;

  const clang::FunctionDecl *TheFunctionDecl;

  // Unimplemented
  RemoveUnusedFunction();

  RemoveUnusedFunction(const RemoveUnusedFunction &);

  void operator=(const RemoveUnusedFunction &);
};
#endif
