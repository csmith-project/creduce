//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "RemoveNamespace.h"

#include <sstream>
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;
using namespace llvm;

static const char *DescriptionMsg =
"Remove namespaces. This pass tries to remove namespace without \
introducing name conflicts. \n";

static RegisterTransformation<RemoveNamespace>
         Trans("remove-namespace", DescriptionMsg);

class RemoveNamespaceASTVisitor : public 
  RecursiveASTVisitor<RemoveNamespaceASTVisitor> {

public:
  explicit RemoveNamespaceASTVisitor(RemoveNamespace *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitNamespaceDecl(NamespaceDecl *ND);

private:
  RemoveNamespace *ConsumerInstance;

};

// A visitor for rewriting decls in the namespace being removed
// ISSUE: quite a lot of functionality could be provided by the
//        RenameClassRewriteVisitor from RenameClass.cpp. 
//        I have certain hesitation of factoring out 
//        RenameClassRewriteVisitor for common uses. 
//        A couple of reasons:
//        * RenameClassRewriteVisitor is only suitable for renaming
//          classes, but here we will be facing more types, e.g., enum.
//        * RenameClassRewriteVisitor handles one class, but here
//          we need to rename multiple conflicting classes;
//        * some processing logic is different here
//        * I don't want to make two transformations interference with
//          each other
//        Therefore, we will have some code duplications (but not much
//        since I put quite a few common utility functions into RewriteUtils)
class RemoveNamespaceRewriteVisitor : public 
  RecursiveASTVisitor<RemoveNamespaceRewriteVisitor> {

public:
  explicit RemoveNamespaceRewriteVisitor(RemoveNamespace *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitNamespaceDecl(NamespaceDecl *ND);

  bool VisitNamespaceAliasDecl(NamespaceAliasDecl *D);

  bool VisitUsingDecl(UsingDecl *D);

  bool VisitUsingDirectiveDecl(UsingDirectiveDecl *D);

  bool VisitCXXConstructorDecl(CXXConstructorDecl *CtorDecl);

  bool VisitCXXDestructorDecl(CXXDestructorDecl *DtorDecl);

  bool VisitCXXMemberCallExpr(CXXMemberCallExpr *CE);

  bool VisitDeclRefExpr(DeclRefExpr *DRE);

  bool VisitDeclaratorDecl(DeclaratorDecl *D);

  bool VisitUnresolvedUsingValueDecl(UnresolvedUsingValueDecl *D);

  bool VisitUnresolvedUsingTypenameDecl(UnresolvedUsingTypenameDecl *D);

  bool VisitTemplateArgumentLoc(const TemplateArgumentLoc &TAL);

  bool VisitRecordTypeLoc(RecordTypeLoc RTLoc);

  bool VisitElaboratedTypeLoc(ElaboratedTypeLoc ETLoc);

  bool VisitTemplateSpecializationTypeLoc(
         TemplateSpecializationTypeLoc TSPLoc);

  bool VisitClassTemplatePartialSpecializationDecl(
         ClassTemplatePartialSpecializationDecl *D);

  bool VisitDependentTemplateSpecializationTypeLoc(
         DependentTemplateSpecializationTypeLoc DTSLoc);

  bool VisitInjectedClassNameTypeLoc(InjectedClassNameTypeLoc TyLoc);

  bool VisitTypedefTypeLoc(TypedefTypeLoc TpLoc);

private:
  RemoveNamespace *ConsumerInstance;

};

bool RemoveNamespaceASTVisitor::VisitNamespaceDecl(NamespaceDecl *ND)
{
  ConsumerInstance->handleOneNamespaceDecl(ND);
  return true;
}

bool RemoveNamespaceRewriteVisitor::VisitNamespaceDecl(NamespaceDecl *ND)
{
  if (ConsumerInstance->isForUsingNamedDecls)
    return true;

  const NamespaceDecl *CanonicalND = ND->getCanonicalDecl();
  if (CanonicalND != ConsumerInstance->TheNamespaceDecl)
    return true;

  ConsumerInstance->removeNamespace(ND);
  return true;
}

bool RemoveNamespaceRewriteVisitor::VisitUsingDirectiveDecl(
       UsingDirectiveDecl *D)
{
  if (ConsumerInstance->isForUsingNamedDecls)
    return true;

  if (ConsumerInstance->UselessUsingDirectiveDecls.count(D)) {
    ConsumerInstance->RewriteHelper->removeDecl(D);
    return true;
  }

  NestedNameSpecifierLoc QualifierLoc = D->getQualifierLoc();
  if (QualifierLoc && 
      ConsumerInstance->replaceNestedNameSpecifier(QualifierLoc))
    return true;

  const NamespaceDecl *CanonicalND = 
    D->getNominatedNamespace()->getCanonicalDecl();
  
  if (CanonicalND == ConsumerInstance->TheNamespaceDecl) {
    // remove the entire Decl if it's in the following form:
    // * using namespace TheNameSpace; or
    // * using namespace ::TheNameSpace;

    if (!QualifierLoc || ConsumerInstance->isGlobalNamespace(QualifierLoc))
      ConsumerInstance->RewriteHelper->removeDecl(D);
    else
      ConsumerInstance->removeLastNamespaceFromUsingDecl(D, CanonicalND);
  }
  
  // handle cases like the following:
  // namespace NS1 { }
  // namespace NS2 {
  //   namespace NS1 {
  //     void foo() {}
  //   }
  //   namespace NS3 {
  //     using namespace NS1;
  //     void bar() {foo();}
  //   }
  // }
  // if we remove NS1, we need to update "using namespace NS1" because "NS1"
  // conflicts with the global namespace NS1. Note that it could only happen
  // if NS1 is the first name in a NestedNameSpecifier
  std::string Name;
  if (ConsumerInstance->getNewNamedDeclName(CanonicalND, Name)) {
    ConsumerInstance->replaceFirstNamespaceFromUsingDecl(D, Name);
  }

  return true;
}

bool RemoveNamespaceRewriteVisitor::VisitUsingDecl(UsingDecl *D)
{
  if (ConsumerInstance->isForUsingNamedDecls)
    return true;

  if (ConsumerInstance->UselessUsingDecls.count(D)) {
    ConsumerInstance->RewriteHelper->removeDecl(D);
    return true;
  }

  // check if this UsingDecl refers to the namespaced being removed
  NestedNameSpecifierLoc QualifierLoc = D->getQualifierLoc();
  TransAssert(QualifierLoc && "Bad QualifierLoc!");
  NestedNameSpecifierLoc PrefixLoc = QualifierLoc.getPrefix();

  const NestedNameSpecifier *NNS = D->getQualifier();
  TransAssert(NNS && "Bad NameSpecifier!");
  if (ConsumerInstance->isTheNamespaceSpecifier(NNS) && 
      (!PrefixLoc || ConsumerInstance->isGlobalNamespace(PrefixLoc))) {
    ConsumerInstance->RewriteHelper->removeDecl(D);
    return true;
  }

  ConsumerInstance->replaceNestedNameSpecifier(QualifierLoc);
  return true;
}

bool RemoveNamespaceRewriteVisitor::VisitNamespaceAliasDecl(
       NamespaceAliasDecl *D)
{
  if (ConsumerInstance->isForUsingNamedDecls)
    return true;

  const NamespaceDecl *CanonicalND = 
    D->getNamespace()->getCanonicalDecl();
  if (CanonicalND == ConsumerInstance->TheNamespaceDecl) {
    ConsumerInstance->RewriteHelper->removeDecl(D);
    return true;
  }

  if (NestedNameSpecifierLoc QualifierLoc = D->getQualifierLoc())
    ConsumerInstance->replaceNestedNameSpecifier(QualifierLoc);

  return true;
}

bool RemoveNamespaceRewriteVisitor::VisitCXXConstructorDecl
       (CXXConstructorDecl *CtorDecl)
{
  if (ConsumerInstance->isForUsingNamedDecls)
    return true;

  const DeclContext *Ctx = CtorDecl->getDeclContext();
  const CXXRecordDecl *CXXRD = dyn_cast<CXXRecordDecl>(Ctx);
  TransAssert(CXXRD && "Invalid CXXRecordDecl");

  std::string Name;
  if (ConsumerInstance->getNewNamedDeclName(CXXRD, Name))
    ConsumerInstance->RewriteHelper->replaceFunctionDeclName(CtorDecl, Name);

  return true;
}

// I didn't factor out the common part of this function
// into RewriteUtils, because the common part has implicit
// dependency on VisitTemplateSpecializationTypeLoc. If in another
// transformation we use this utility without implementing
// VisitTemplateSpecializationTypeLoc, we will be in trouble.
bool RemoveNamespaceRewriteVisitor::VisitCXXDestructorDecl(
       CXXDestructorDecl *DtorDecl)
{
  if (ConsumerInstance->isForUsingNamedDecls)
    return true;

  const DeclContext *Ctx = DtorDecl->getDeclContext();
  const CXXRecordDecl *CXXRD = dyn_cast<CXXRecordDecl>(Ctx);
  TransAssert(CXXRD && "Invalid CXXRecordDecl");

  std::string Name;
  if (!ConsumerInstance->getNewNamedDeclName(CXXRD, Name))
    return true;

  // Avoid duplicated VisitDtor. 
  // For example, in the code below:
  // template<typename T>
  // class SomeClass {
  // public:
  //   ~SomeClass<T>() {}
  // };
  // ~SomeClass<T>'s TypeLoc is represented as TemplateSpecializationTypeLoc
  // In this case, ~SomeClass will be renamed from 
  // VisitTemplateSpecializationTypeLoc.
  DeclarationNameInfo NameInfo = DtorDecl->getNameInfo();
  if ( TypeSourceInfo *TSInfo = NameInfo.getNamedTypeInfo()) {
    TypeLoc DtorLoc = TSInfo->getTypeLoc();
    if (!DtorLoc.isNull() && 
        (DtorLoc.getTypeLocClass() == TypeLoc::TemplateSpecialization))
      return true;
  }

  Name = "~" + Name;
  ConsumerInstance->RewriteHelper->replaceFunctionDeclName(DtorDecl, Name);

  return true;
}

bool RemoveNamespaceRewriteVisitor::VisitCXXMemberCallExpr(
       CXXMemberCallExpr *CXXCE)
{
  const CXXRecordDecl *CXXRD = CXXCE->getRecordDecl();
  // getRecordDEcl could return NULL if getImplicitObjectArgument() 
  // returns NULL
  if (!CXXRD || ConsumerInstance->isForUsingNamedDecls)
    return true;

  std::string Name;
  // Dtors from UsingNamedDecl can't have conflicts, so it's safe
  // to get new names from NamedDecl set
  if (ConsumerInstance->getNewNamedDeclName(CXXRD, Name))
    ConsumerInstance->RewriteHelper->replaceCXXDtorCallExpr(CXXCE, Name);
  return true;
}

bool RemoveNamespaceRewriteVisitor::VisitDeclRefExpr(DeclRefExpr *DRE)
{
  NestedNameSpecifierLoc QualifierLoc = DRE->getQualifierLoc();
  if (QualifierLoc && 
      (ConsumerInstance->replaceNestedNameSpecifier(QualifierLoc)))
    return true;

  const ValueDecl *OrigDecl = DRE->getDecl();
  if (isa<FunctionDecl>(OrigDecl) || isa<VarDecl>(OrigDecl)) {
    std::string Name;
    if (ConsumerInstance->getNewName(OrigDecl, Name)) {
      ConsumerInstance->TheRewriter.ReplaceText(DRE->getLocStart(),
        OrigDecl->getNameAsString().size(), Name);
    }
  }

  return true;
}

bool RemoveNamespaceRewriteVisitor::VisitDeclaratorDecl(DeclaratorDecl *D)
{
  if (ConsumerInstance->isForUsingNamedDecls)
    return true;

  if (NestedNameSpecifierLoc QualifierLoc = D->getQualifierLoc())
    ConsumerInstance->replaceNestedNameSpecifier(QualifierLoc);

  return true;
}

bool RemoveNamespaceRewriteVisitor::VisitUnresolvedUsingValueDecl(
       UnresolvedUsingValueDecl *D)
{
  if (ConsumerInstance->isForUsingNamedDecls)
    return true;

  if (NestedNameSpecifierLoc QualifierLoc = D->getQualifierLoc())
    ConsumerInstance->replaceNestedNameSpecifier(QualifierLoc);

  return true;
}

bool RemoveNamespaceRewriteVisitor::VisitUnresolvedUsingTypenameDecl(
       UnresolvedUsingTypenameDecl *D)
{
  if (ConsumerInstance->isForUsingNamedDecls)
    return true;

  if (NestedNameSpecifierLoc QualifierLoc = D->getQualifierLoc())
    ConsumerInstance->replaceNestedNameSpecifier(QualifierLoc);

  return true;
}

// FIXME: need to invoke this function from where TemplateArgument[s]
//        could appear
bool RemoveNamespaceRewriteVisitor::VisitTemplateArgumentLoc(
       const TemplateArgumentLoc &TAL)
{
  if (NestedNameSpecifierLoc QualifierLoc = TAL.getTemplateQualifierLoc())
    ConsumerInstance->replaceNestedNameSpecifier(QualifierLoc);
  return true;
}

bool RemoveNamespaceRewriteVisitor::VisitRecordTypeLoc(RecordTypeLoc RTLoc)
{
  const CXXRecordDecl *RD = dyn_cast<CXXRecordDecl>(RTLoc.getDecl());
  if (!RD)
    return true;

  std::string Name;
  if (ConsumerInstance->getNewName(RD, Name)) {
    ConsumerInstance->RewriteHelper->replaceRecordType(RTLoc, Name);
  }
  return true;
}

bool RemoveNamespaceRewriteVisitor::VisitElaboratedTypeLoc(
       ElaboratedTypeLoc ETLoc)
{
  if (ConsumerInstance->isForUsingNamedDecls)
    return true;

  if (NestedNameSpecifierLoc QualifierLoc = ETLoc.getQualifierLoc())
    ConsumerInstance->replaceNestedNameSpecifier(QualifierLoc);

  return true;
}

bool RemoveNamespaceRewriteVisitor::VisitTemplateSpecializationTypeLoc(
       TemplateSpecializationTypeLoc TSPLoc)
{
  const Type *Ty = TSPLoc.getTypePtr();
  const TemplateSpecializationType *TST = 
    dyn_cast<TemplateSpecializationType>(Ty);
  TransAssert(TST && "Bad TemplateSpecializationType!");

  TemplateName TplName = TST->getTemplateName();
  const TemplateDecl *TplD = TplName.getAsTemplateDecl();
  TransAssert(TplD && "Invalid TemplateDecl!");
  NamedDecl *ND = TplD->getTemplatedDecl();
  // in some cases, ND could be NULL, e.g., the 
  // template template parameter code below:
  // template<template<class> class BBB>
  // struct AAA {
  //   template <class T>
  //   struct CCC {
  //     static BBB<T> a;
  //   };
  // };
  // where we don't know BBB
  if (!ND)
    return true;

  const CXXRecordDecl *CXXRD = dyn_cast<CXXRecordDecl>(ND);
  if (!CXXRD)
    return true;

  std::string Name;
  if (ConsumerInstance->getNewName(CXXRD, Name)) {
    SourceLocation LocStart = TSPLoc.getTemplateNameLoc();
    ConsumerInstance->TheRewriter.ReplaceText(
      LocStart, CXXRD->getNameAsString().size(), Name);
  }

  return true;
}

bool RemoveNamespaceRewriteVisitor::VisitClassTemplatePartialSpecializationDecl(
       ClassTemplatePartialSpecializationDecl *D)
{
  const Type *Ty = D->getInjectedSpecializationType().getTypePtr();
  TransAssert(Ty && "Bad TypePtr!");
  const TemplateSpecializationType *TST = 
    dyn_cast<TemplateSpecializationType>(Ty);
  TransAssert(TST && "Bad TemplateSpecializationType!");

  TemplateName TplName = TST->getTemplateName();
  const TemplateDecl *TplD = TplName.getAsTemplateDecl();
  TransAssert(TplD && "Invalid TemplateDecl!");
  NamedDecl *ND = TplD->getTemplatedDecl();
  TransAssert(ND && "Invalid NamedDecl!");

  const CXXRecordDecl *CXXRD = dyn_cast<CXXRecordDecl>(ND);
  TransAssert(CXXRD && "Invalid CXXRecordDecl!");

  std::string Name;
  if (ConsumerInstance->getNewName(CXXRD, Name)) {
    const TypeSourceInfo *TyInfo = D->getTypeAsWritten();
    if (!TyInfo)
      return true;
    TypeLoc TyLoc = TyInfo->getTypeLoc();
    SourceLocation LocStart = TyLoc.getLocStart();
    TransAssert(LocStart.isValid() && "Invalid Location!");
    ConsumerInstance->TheRewriter.ReplaceText(
      LocStart, CXXRD->getNameAsString().size(), Name);
  }
  return true;
}

// handle the case where a template specialization type cannot be resolved, e.g.
// template <class T> struct Base {};
// template <class T> struct Derived: public Base<T> {
//  typename Derived::template Base<double>* p1;
// };
bool RemoveNamespaceRewriteVisitor::VisitDependentTemplateSpecializationTypeLoc(
       DependentTemplateSpecializationTypeLoc DTSLoc)
{
  const Type *Ty = DTSLoc.getTypePtr();
  const DependentTemplateSpecializationType *DTST = 
    dyn_cast<DependentTemplateSpecializationType>(Ty);
  TransAssert(DTST && "Bad DependentTemplateSpecializationType!");

  const IdentifierInfo *IdInfo = DTST->getIdentifier();
  std::string IdName = IdInfo->getName();
  std::string Name;

  // FIXME:
  // This isn't quite right, we will generate bad code for some cases, e.g.,
  // namespace NS1 {
  //   template <class T> struct Base {};
  //   template <class T> struct Derived: public Base<T> {
  //     typename Derived::template Base<double>* p1;
  //   };
  // }
  // template <class T> struct Base {};
  // template <class T> struct Derived: public Base<T> {
  //   typename Derived::template Base<double>* p1;
  // };
  // For the global Derived template class, we will end up with
  // typename Derived::template Tran_NS_NS1_Base ...,
  // which is obviously wrong.
  // Any way to avoid this bad transformation?
  if (ConsumerInstance->getNewNameByName(IdName, Name)) {
    SourceLocation LocStart = DTSLoc.getTemplateNameLoc();
    ConsumerInstance->TheRewriter.ReplaceText(
      LocStart, IdName.size(), Name);
  }

  if ( NestedNameSpecifierLoc QualifierLoc = DTSLoc.getQualifierLoc() )
    ConsumerInstance->replaceNestedNameSpecifier(QualifierLoc);
  return true;
}

bool RemoveNamespaceRewriteVisitor::VisitInjectedClassNameTypeLoc(
       InjectedClassNameTypeLoc TyLoc)
{
  const CXXRecordDecl *CXXRD = TyLoc.getDecl();
  TransAssert(CXXRD && "Invalid CXXRecordDecl!");

  std::string Name;
  if (ConsumerInstance->getNewName(CXXRD, Name)) {
    SourceLocation LocStart = TyLoc.getLocStart();
    TransAssert(LocStart.isValid() && "Invalid Location!");

    ConsumerInstance->TheRewriter.ReplaceText(
      LocStart, CXXRD->getNameAsString().size(), Name);
  }
  return true;
}

bool RemoveNamespaceRewriteVisitor::VisitTypedefTypeLoc(TypedefTypeLoc TyLoc)
{
  const TypedefNameDecl *D = TyLoc.getTypedefNameDecl();
  
  std::string Name;
  if (ConsumerInstance->getNewName(D, Name)) {
    SourceLocation LocStart = TyLoc.getLocStart();
    ConsumerInstance->TheRewriter.ReplaceText(
      LocStart, D->getNameAsString().size(), Name);
  }
  return true;
}

void RemoveNamespace::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new RemoveNamespaceASTVisitor(this);
  RewriteVisitor = new RemoveNamespaceRewriteVisitor(this);
}

bool RemoveNamespace::HandleTopLevelDecl(DeclGroupRef D) 
{
  // Nothing to do
  return true;
}
 
void RemoveNamespace::HandleTranslationUnit(ASTContext &Ctx)
{
  if (TransformationManager::isCLangOpt()) {
    ValidInstanceNum = 0;
  }
  else {
    // Invoke CollectionVisitor here because we need full DeclContext
    // to resolve name conflicts. Full ASTs has been built at this point.
    CollectionVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
  }

  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  TransAssert(RewriteVisitor && "NULL RewriteVisitor!");
  TransAssert(TheNamespaceDecl && "NULL TheNamespaceDecl!");
  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  // First rename UsingNamedDecls, i.e., conflicting names
  // from other namespaces. 
  // FIXME: isForUsingNamedDecls flag is quite ugly,
  //        need a way to remove it
  isForUsingNamedDecls = true;
  RewriteVisitor->TraverseDecl(TheNamespaceDecl);
  isForUsingNamedDecls = false;

  rewriteNamedDecls();
  RewriteVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void RemoveNamespace::rewriteNamedDecls(void)
{
  for (NamedDeclToNameMap::const_iterator I = NamedDeclToNewName.begin(),
       E = NamedDeclToNewName.end(); I != E; ++I) {
    const NamedDecl *D = (*I).first;
    std::string Name = (*I).second;

    Decl::Kind K = D->getKind();
    switch (K) {
    case Decl::Function: {
      // Check replaceFunctionDecl in RewriteUtils.cpp for the reason that
      // we need a special case for FunctionDecl
      const FunctionDecl *FD = dyn_cast<FunctionDecl>(D);
      RewriteHelper->replaceFunctionDeclName(FD, Name);
      break;
    }
    case Decl::ClassTemplateSpecialization: {
      // Skip this case, which will be handled by 
      // VisitTemplateSpecializationTypeLoc
      break;
    }
    default:
      RewriteHelper->replaceNamedDeclName(D, Name);
    }
  }
}

bool RemoveNamespace::hasNameConflict(const NamedDecl *ND, 
                                      const DeclContext *ParentCtx)
{
  DeclarationName Name = ND->getDeclName();
  DeclContextLookupConstResult Result = ParentCtx->lookup(Name);
  return (Result.first != Result.second);
}

// A using declaration in the removed namespace could cause
// name conflict, e.g.,
// namespace NS1 {
//   void foo(void) {}
// }
// namespace NS2 {
//   using NS1::foo;
//   void bar() { ... foo(); ... }
// }
// void foo() {...}
// void func() {... foo(); ...}
// if we remove NS2, then foo() in func() will become ambiguous.
// In this case, we need to replace the first invocation of foo()
// with NS1::foo()
void RemoveNamespace::handleOneUsingShadowDecl(const UsingShadowDecl *UD,
                                               const DeclContext *ParentCtx)
{
  const NamedDecl *ND = UD->getTargetDecl();
  if (!hasNameConflict(ND, ParentCtx))
    return;
  
  std::string NewName;
  const UsingDecl *D = UD->getUsingDecl();
  NestedNameSpecifierLoc PrefixLoc = D->getQualifierLoc().getPrefix();
  RewriteHelper->getQualifierAsString(PrefixLoc, NewName);

  NewName += "::";
  const IdentifierInfo *IdInfo = ND->getIdentifier();
  NewName += IdInfo->getName();
  UsingNamedDeclToNewName[ND] = NewName;
  
  // the tied UsingDecl becomes useless, and hence it's removable
  UselessUsingDecls.insert(D);
}

// For the similar reason as dealing with using declarations,
// we need to resolve the possible name conflicts introduced by
// using directives
void RemoveNamespace::handleOneUsingDirectiveDecl(const UsingDirectiveDecl *UD,
                                                  const DeclContext *ParentCtx)
{
  const NamespaceDecl *ND = UD->getNominatedNamespace();
  TransAssert(!ND->isAnonymousNamespace() && 
              "Cannot have anonymous namespaces!");
  std::string NamespaceName = ND->getNameAsString();

  for (DeclContext::decl_iterator I = ND->decls_begin(), E = ND->decls_end();
       I != E; ++I) {
    const NamedDecl *NamedD = dyn_cast<NamedDecl>(*I);
    if (!NamedD)
      continue;

    if (!isa<TemplateDecl>(NamedD) && !isa<TypeDecl>(NamedD) && 
        !isa<ValueDecl>(NamedD) && !isa<NamespaceDecl>(NamedD))
      continue;

    const IdentifierInfo *IdInfo = NamedD->getIdentifier();
    std::string NewName;
    if ( NestedNameSpecifierLoc QualifierLoc = UD->getQualifierLoc() ) {
      RewriteHelper->getQualifierAsString(QualifierLoc, NewName);
    }
    else {
      NewName = NamespaceName;
    }
    NewName += "::";
    NewName += IdInfo->getName();

    if ( const TemplateDecl *TD = dyn_cast<TemplateDecl>(NamedD) ) {
      NamedD = TD->getTemplatedDecl();
    }
    UsingNamedDeclToNewName[NamedD] = NewName;
  }

  // We always remove this using directive, and in ThenNamespaceDecl 
  // append the corresponding namespace specifier to all names from the 
  // used namespace. The reason is that the conlicted name in the outter 
  // namespace could appear after TheNamespaceDecl, e.g.,
  //   namespace NS1 {
  //     void foo() {}
  //     void fiz() {}
  //   }
  //   namespace NS2 {
  //     using namespace NS1;
  //     int bar() { foo(); }
  //   }
  //   void fiz() {}
  //   void f() { fiz(); }
  // If we keep "using namespace NS1" after removing NS2, then we will
  // end up with an ambiguous name fiz
  // Consequently, we will add a few unecessary namespace qualifiers to
  // those without conflicting names. It's a trade-off.
  UselessUsingDirectiveDecls.insert(UD);
}

void RemoveNamespace::handleOneNamedDecl(const NamedDecl *ND,
                                         const DeclContext *ParentCtx,
                                         const std::string &NamespaceName)
{
  Decl::Kind K = ND->getKind();
  switch (K) {
  case Decl::UsingShadow: {
    const UsingShadowDecl *D = dyn_cast<UsingShadowDecl>(ND);
    handleOneUsingShadowDecl(D, ParentCtx);
    break;
  }

  case Decl::UsingDirective: {
    const UsingDirectiveDecl *D = dyn_cast<UsingDirectiveDecl>(ND);
    handleOneUsingDirectiveDecl(D, ParentCtx);
    break;
  }

  default:
    if (isa<NamespaceAliasDecl>(ND) || isa<TemplateDecl>(ND) ||
        isa<TypeDecl>(ND) || isa<ValueDecl>(ND) || isa<NamespaceDecl>(ND)) {
      if (!hasNameConflict(ND, ParentCtx))
        break;

      std::string NewName = NamePrefix + NamespaceName;
      const IdentifierInfo *IdInfo = ND->getIdentifier();
      NewName += "_";
      NewName += IdInfo->getName();

      if ( const TemplateDecl *TD = dyn_cast<TemplateDecl>(ND) ) {
        ND = TD->getTemplatedDecl();
      }
      NamedDeclToNewName[ND] = NewName;
    }
  }
}

void RemoveNamespace::addNamedDeclsFromNamespace(const NamespaceDecl *ND)
{
  // We don't care about name-lookup for friend's functions, so just
  // retrive ParentContext rather than LookupParent
  const DeclContext *ParentCtx = ND->getParent();
  std::string NamespaceName;

  if (ND->isAnonymousNamespace()) {
    std::stringstream TmpSS;
    TmpSS << AnonNamePrefix << AnonNamespaceCounter;
    NamespaceName = TmpSS.str();
    AnonNamespaceCounter++;
  }
  else {
    NamespaceName = ND->getNameAsString();
  }

  for (DeclContext::decl_iterator I = ND->decls_begin(), E = ND->decls_end();
       I != E; ++I) {
    if ( const NamedDecl *D = dyn_cast<NamedDecl>(*I) )
      handleOneNamedDecl(D, ParentCtx, NamespaceName);
  }
}

bool RemoveNamespace::handleOneNamespaceDecl(NamespaceDecl *ND)
{
  NamespaceDecl *CanonicalND = ND->getCanonicalDecl();
  if (VisitedND.count(CanonicalND)) {
    if (TheNamespaceDecl == CanonicalND) {
      addNamedDeclsFromNamespace(ND);
    }
    return true;
  }

  VisitedND.insert(CanonicalND);
  ValidInstanceNum++;
  if (ValidInstanceNum == TransformationCounter) {
    TheNamespaceDecl = CanonicalND;
    addNamedDeclsFromNamespace(ND);
  }
  return true;
}

void RemoveNamespace::removeNamespace(const NamespaceDecl *ND)
{
  // Remove the right brace first
  SourceLocation StartLoc = ND->getRBraceLoc();
  TheRewriter.RemoveText(StartLoc, 1);

  // Then remove name and the left brace
  StartLoc = ND->getLocStart();
  TransAssert(StartLoc.isValid() && "Invalid Namespace LocStart!");

  const char *StartBuf = SrcManager->getCharacterData(StartLoc);
  SourceRange NDRange = ND->getSourceRange();
  int RangeSize = TheRewriter.getRangeSize(NDRange);
  TransAssert((RangeSize != -1) && "Bad Namespace Range!");

  std::string NDStr(StartBuf, RangeSize);
  size_t Pos = NDStr.find('{');
  TransAssert((Pos != std::string::npos) && "Cannot find LBrace!");
  SourceLocation EndLoc = StartLoc.getLocWithOffset(Pos);
  TheRewriter.RemoveText(SourceRange(StartLoc, EndLoc));
}

bool RemoveNamespace::getNewNameFromNameMap(const NamedDecl *ND, 
                                            std::string &Name,
                                            const NamedDeclToNameMap &NameMap)
{
  NamedDeclToNameMap::const_iterator Pos = 
    NameMap.find(ND);
  if (Pos == NameMap.end())
    return false;

  Name = (*Pos).second;
  return true;
}

bool RemoveNamespace::getNewNameByNameFromNameMap(const std::string &Name,
                                            std::string &NewName,
                                            const NamedDeclToNameMap &NameMap)
{
  for (NamedDeclToNameMap::const_iterator I = NameMap.begin(),
       E = NameMap.end(); I != E; ++I) {
    const NamedDecl *D = (*I).first;
    const CXXRecordDecl *CXXRD = dyn_cast<CXXRecordDecl>(D);
    if (!CXXRD)
      continue;
    if (Name == CXXRD->getNameAsString()) {
      NewName = (*I).second;
      return true;
    }
  }
  return false;
}

bool RemoveNamespace::getNewNamedDeclName(const NamedDecl *ND,
                                          std::string &Name)
{
  return getNewNameFromNameMap(ND, Name, NamedDeclToNewName);
}

bool RemoveNamespace::getNewUsingNamedDeclName(const NamedDecl *ND,
                                               std::string &Name)
{
  return getNewNameFromNameMap(ND, Name, UsingNamedDeclToNewName);
}

bool RemoveNamespace::getNewName(const NamedDecl *ND, 
                                 std::string &Name)
{
  if (isForUsingNamedDecls)
    return getNewUsingNamedDeclName(ND, Name);
  else
    return getNewNamedDeclName(ND, Name);
}

bool RemoveNamespace::getNewNameByName(const std::string &Name,
                                       std::string &NewName)
{
  if (isForUsingNamedDecls)
    return getNewNameByNameFromNameMap(Name, NewName, UsingNamedDeclToNewName);
  else
    return getNewNameByNameFromNameMap(Name, NewName, NamedDeclToNewName);
}

bool RemoveNamespace::isGlobalNamespace(NestedNameSpecifierLoc Loc)
{
  NestedNameSpecifier *NNS = Loc.getNestedNameSpecifier();
  return (NNS->getKind() == NestedNameSpecifier::Global);
}

bool RemoveNamespace::isTheNamespaceSpecifier(const NestedNameSpecifier *NNS)
{
  NestedNameSpecifier::SpecifierKind Kind = NNS->getKind();
  switch (Kind) {
  case NestedNameSpecifier::Namespace: {
    const NamespaceDecl *CanonicalND = 
      NNS->getAsNamespace()->getCanonicalDecl();
    return (CanonicalND == TheNamespaceDecl);
  }

  case NestedNameSpecifier::NamespaceAlias: {
    const NamespaceAliasDecl *NAD = NNS->getAsNamespaceAlias();
    const NamespaceDecl *CanonicalND = 
      NAD->getNamespace()->getCanonicalDecl();
    return (CanonicalND == TheNamespaceDecl);
  }

  default:
    return false;
  }
  TransAssert(0 && "Unreachable code!");
  return false;
}

// It handles two cases:
//   * remove the specifier if it refers to TheNamespaceDecl
//   * replace the specifier with a new name if the corresponding namespace
//     has a name conflicts, e.g.,
//   namespace NS1 { }
//   namespace NS2 {
//     namespace NS1 {
//       void foo() {}
//     }
//     namespace NS3 {
//       using NS1::foo;
//       void bar() { foo(); }
//     }
//   }
//   If we remove NS2, then the inner namespace NS1 conflicts with
//   the global NS1, but "using NS1::foo" refers to the conflicting NS1.
bool RemoveNamespace::replaceNestedNameSpecifier(
       NestedNameSpecifierLoc QualifierLoc)
{
  SmallVector<NestedNameSpecifierLoc, 8> QualifierLocs;
  for (; QualifierLoc; QualifierLoc = QualifierLoc.getPrefix())
    QualifierLocs.push_back(QualifierLoc);

  while (!QualifierLocs.empty()) {
    NestedNameSpecifierLoc Loc = QualifierLocs.pop_back_val();
    NestedNameSpecifier *NNS = Loc.getNestedNameSpecifier();
    NestedNameSpecifier::SpecifierKind Kind = NNS->getKind();
    const NamespaceDecl *ND = NULL;
    switch (Kind) {
      case NestedNameSpecifier::Namespace: {
        ND = NNS->getAsNamespace()->getCanonicalDecl();
        break;
      }
      case NestedNameSpecifier::NamespaceAlias: {
        const NamespaceAliasDecl *NAD = NNS->getAsNamespaceAlias();
        ND = NAD->getNamespace()->getCanonicalDecl();
        break;
      }
      default:
        break;
    }

    if (!ND)
      continue;

    if (ND == TheNamespaceDecl) {
      RewriteHelper->removeSpecifier(Loc);
      return true;
    }

    std::string SpecifierName;
    RewriteHelper->getSpecifierAsString(Loc, SpecifierName);
    std::string NDName = ND->getNameAsString();
    std::string Name = "";
    getNewName(ND, Name);

    // Skip it if this specifier is the same as ND's name. 
    // Note that the above case could only happen for UsingNamedDecls
    if (isForUsingNamedDecls && (SpecifierName == NDName)) {
      // It could happen for example:
      // namespace NS1 { }
      // namespace NS2 {
      //   using namespace NS1;
      //   void bar() { NS1::foo(); }
      // }
      // If we remove NS2, then the guard below avoids renaming 
      // NS1::foo to NS1::foo::foo. 
      if (Name.empty())
        return true;

      // another case to handle:
      // namespace NS1 {
      //   namespace NS2 {
      //     void foo() {}
      //   }
      // }
      // namespace NS3 {
      //   using namespace NS1;
      //   void bar() { NS2::foo(); }
      // } 
      // If we remove NS3, we do need to rename NS2::foo as NS1::NS2::foo
      if (!isSuffix(Name, SpecifierName))
        return true;
    }
    
    if (!Name.empty()) {
      RewriteHelper->replaceSpecifier(Loc, Name);
      return true;
    }
  }
  return false;
}

bool RemoveNamespace::isSuffix(std::string &Name, std::string &SpecifierName)
{
  size_t NameLen = Name.length();
  std::string Suffix = "::" + SpecifierName;
  size_t SuffixLen = Suffix.length();
  if (NameLen <= SuffixLen)
    return false;

  return !Suffix.compare(0, SuffixLen, Name, NameLen - SuffixLen, SuffixLen);
}

void RemoveNamespace::replaceFirstNamespaceFromUsingDecl(
       const UsingDirectiveDecl *D, const std::string &Name)
{
  const NamespaceDecl *ND = D->getNominatedNamespace();
  SourceLocation IdLocStart = D->getIdentLocation();
  TheRewriter.ReplaceText(IdLocStart, ND->getNameAsString().length(), Name);
}

void RemoveNamespace::removeLastNamespaceFromUsingDecl(
       const UsingDirectiveDecl *D, const NamespaceDecl *ND)
{
  SourceLocation IdLocStart = D->getIdentLocation();
  SourceRange DeclSourceRange = D->getSourceRange();
  SourceLocation DeclLocStart = DeclSourceRange.getBegin();

  const char *IdStartBuf = SrcManager->getCharacterData(IdLocStart);
  const char *DeclStartBuf = SrcManager->getCharacterData(DeclLocStart);

  unsigned Count = 0;
  int Offset = 0;
  while (IdStartBuf != DeclStartBuf) {
    if (*IdStartBuf != ':') {
      IdStartBuf--;
      Offset--;
      continue;
    }

    Count++;
    if (Count == 2) {
      break;
    }
    Offset--;
    IdStartBuf--;
  }
  TransAssert((Count == 2) && "Bad NestedNamespaceSpecifier!");
  TransAssert((Offset < 0) && "Bad Offset Value!");
  IdLocStart = IdLocStart.getLocWithOffset(Offset);

  TheRewriter.RemoveText(IdLocStart, 
                         ND->getNameAsString().length() - Offset);
}

RemoveNamespace::~RemoveNamespace(void)
{
  if (CollectionVisitor)
    delete CollectionVisitor;
  if (RewriteVisitor)
    delete RewriteVisitor;
}

