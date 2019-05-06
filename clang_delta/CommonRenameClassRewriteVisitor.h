//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2018, 2019 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef COMMON_RENAME_CLASS_REWRITE_VISITOR_H
#define COMMON_RENAME_CLASS_REWRITE_VISITOR_H

#include "llvm/ADT/SmallPtrSet.h"
#include "clang/AST/RecursiveASTVisitor.h"

class Transformation;

namespace clang_delta_common_visitor {

using namespace clang;

template<typename T>
class CommonRenameClassRewriteVisitor : public RecursiveASTVisitor<T> {
public:
  CommonRenameClassRewriteVisitor(Transformation *Instance,
                                  Rewriter *RT,
                                  RewriteUtils *Helper,
                                  const CXXRecordDecl *CXXRD,
                                  const std::string &Name)
    : ConsumerInstance(Instance),
      TheRewriter(RT),
      RewriteHelper(Helper),
      TheCXXRecordDecl(CXXRD),
      NewNameStr(Name)
  { }

  T &getDerived() { return *static_cast<T*>(this); };

  bool VisitCXXRecordDecl(CXXRecordDecl *CXXRD);

  bool VisitCXXConstructorDecl(CXXConstructorDecl *CtorDecl);

  bool VisitCXXDestructorDecl(CXXDestructorDecl *DtorDecl);

  bool VisitCXXMemberCallExpr(CXXMemberCallExpr *CE);

  bool VisitInjectedClassNameTypeLoc(InjectedClassNameTypeLoc TyLoc);

  bool VisitRecordTypeLoc(RecordTypeLoc RTLoc);

  bool VisitTemplateSpecializationTypeLoc(
         TemplateSpecializationTypeLoc TSPLoc);

  bool TraverseTemplateArgumentLoc(const TemplateArgumentLoc &ArgLoc);

  bool VisitDependentTemplateSpecializationTypeLoc(
         DependentTemplateSpecializationTypeLoc DTSLoc);

  bool VisitClassTemplatePartialSpecializationDecl(
         ClassTemplatePartialSpecializationDecl *D);

  bool VisitClassTemplateSpecializationDecl(
         ClassTemplateSpecializationDecl *TSD);

  bool TraverseConstructorInitializer(CXXCtorInitializer *Init);

  bool VisitUsingDecl(UsingDecl *D);

private:
  typedef llvm::SmallPtrSet<void *, 20> LocPtrSet;

  void renameTemplateName(TemplateName TmplName, SourceLocation LocStart);

  bool getNewName(const CXXRecordDecl *CXXRD, std::string &NewName);

  bool getNewNameByName(const std::string &Name, std::string &NewName);

  LocPtrSet VisitedLocs;

  Transformation *ConsumerInstance;

  Rewriter *TheRewriter;

  RewriteUtils *RewriteHelper;

  const CXXRecordDecl *TheCXXRecordDecl;

  std::string NewNameStr;
};

template<typename T>
bool CommonRenameClassRewriteVisitor<T>::VisitUsingDecl(UsingDecl *D)
{
  DeclarationNameInfo NameInfo = D->getNameInfo();
  DeclarationName DeclName = NameInfo.getName();
  if (DeclName.getNameKind() != DeclarationName::Identifier)
    return true;

  IdentifierInfo *IdInfo = DeclName.getAsIdentifierInfo();
  std::string IdName = IdInfo->getName();
  std::string Name;
  if (getNewNameByName(IdName, Name)) {
    SourceLocation LocStart = NameInfo.getBeginLoc();
    TheRewriter->ReplaceText(LocStart, IdName.size(), Name);
  }
  return true;
}

template<typename T>
bool CommonRenameClassRewriteVisitor<T>::TraverseConstructorInitializer(
       CXXCtorInitializer *Init) 
{
  if (Init->isBaseInitializer() && !Init->isWritten())
    return true;

  if (TypeSourceInfo *TInfo = Init->getTypeSourceInfo())
    getDerived().TraverseTypeLoc(TInfo->getTypeLoc());

  if (Init->isWritten())
    getDerived().TraverseStmt(Init->getInit());
  return true;
}

template<typename T> 
bool CommonRenameClassRewriteVisitor<T>::
     VisitClassTemplatePartialSpecializationDecl(
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
  if (getNewName(CXXRD, Name)) {
    const TypeSourceInfo *TyInfo = D->getTypeAsWritten();
    if (!TyInfo)
      return true;
    TypeLoc TyLoc = TyInfo->getTypeLoc();
    SourceLocation LocStart = TyLoc.getBeginLoc();
    TransAssert(LocStart.isValid() && "Invalid Location!");
    TheRewriter->ReplaceText(LocStart, CXXRD->getNameAsString().size(), Name);
  }
  return true;
}

// ISSUE: I am not sure why, but RecursiveASTVisitor doesn't recursively
// visit base classes from explicit template specialization, e.g.,
//   struct A { };
//   template<typename T> class B : public A<T> { };
//   template<> class B : public A<short> { };
// In the above case, A<short> won't be touched.
// So we have to do it manually
template<typename T>
bool CommonRenameClassRewriteVisitor<T>::VisitClassTemplateSpecializationDecl(
       ClassTemplateSpecializationDecl *TSD)
{
  if (!TSD->isExplicitSpecialization() || !TSD->isCompleteDefinition())
    return true;

  for (CXXRecordDecl::base_class_const_iterator I = TSD->bases_begin(),
       E = TSD->bases_end(); I != E; ++I) {
    TypeSourceInfo *TSI = (*I).getTypeSourceInfo();
    TransAssert(TSI && "Bad TypeSourceInfo!");
    getDerived().TraverseTypeLoc(TSI->getTypeLoc());
  }
  return true;
}

template<typename T>
bool CommonRenameClassRewriteVisitor<T>::VisitCXXRecordDecl(
       CXXRecordDecl *CXXRD)
{
  std::string Name;
  if (!getNewName(CXXRD, Name))
    return true;

  void *LocPtr = CXXRD->getLocation().getPtrEncoding();
  if (VisitedLocs.count(LocPtr))
    return true;
  VisitedLocs.insert(LocPtr);

  if (ConsumerInstance->isDeclaringRecordDecl(CXXRD)) {
    RewriteHelper->replaceRecordDeclDef(CXXRD, Name);
  }
  else {
    RewriteHelper->replaceRecordDeclName(CXXRD, Name);
  }

  return true;
}

template<typename T>
bool CommonRenameClassRewriteVisitor<T>::VisitCXXConstructorDecl
       (CXXConstructorDecl *CtorDecl)
{
  const DeclContext *Ctx = CtorDecl->getDeclContext();
  const CXXRecordDecl *CXXRD = dyn_cast<CXXRecordDecl>(Ctx);
  TransAssert(CXXRD && "Invalid CXXRecordDecl");

  std::string Name;
  if (getNewName(CXXRD, Name))
    RewriteHelper->replaceFunctionDeclName(CtorDecl, Name);

  return true;
}

template<typename T>
bool CommonRenameClassRewriteVisitor<T>::VisitCXXDestructorDecl(
       CXXDestructorDecl *DtorDecl)
{
  const DeclContext *Ctx = DtorDecl->getDeclContext();
  const CXXRecordDecl *CXXRD = dyn_cast<CXXRecordDecl>(Ctx);
  TransAssert(CXXRD && "Invalid CXXRecordDecl");

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

  SourceLocation StartLoc = DtorDecl->getLocation();
  // skip '~'
  StartLoc = StartLoc.getLocWithOffset(1);
  void *LocPtr = StartLoc.getPtrEncoding();
  if (VisitedLocs.count(LocPtr))
    return true;
  VisitedLocs.insert(LocPtr);

  std::string Name;
  if (getNewName(CXXRD, Name)) {
    RewriteHelper->replaceCXXDestructorDeclName(DtorDecl, Name);
  }

  return true;
}

template<typename T>
bool CommonRenameClassRewriteVisitor<T>::VisitInjectedClassNameTypeLoc(
       InjectedClassNameTypeLoc TyLoc)
{
  const CXXRecordDecl *CXXRD = TyLoc.getDecl();
  TransAssert(CXXRD && "Invalid CXXRecordDecl!");

  std::string Name;
  if (getNewName(CXXRD, Name)) {
    SourceLocation LocStart = TyLoc.getBeginLoc();
    TransAssert(LocStart.isValid() && "Invalid Location!");

    TheRewriter->ReplaceText(LocStart, CXXRD->getNameAsString().size(), Name);
  }
  return true;
}

template<typename T>
bool CommonRenameClassRewriteVisitor<T>::VisitCXXMemberCallExpr(
       CXXMemberCallExpr *CE)
{
  const CXXRecordDecl *CXXRD = CE->getRecordDecl();
  // getRecordDEcl could return NULL if getImplicitObjectArgument() 
  // returns NULL
  if (!CXXRD)
    return true;

  std::string Name;
  if (getNewName(CXXRD, Name)) {
    RewriteHelper->replaceCXXDtorCallExpr(CE, Name);
  }
  return true;
}

template<typename T>
bool CommonRenameClassRewriteVisitor<T>::VisitRecordTypeLoc(RecordTypeLoc RTLoc)
{
  const Type *Ty = RTLoc.getTypePtr();
  if (Ty->isUnionType())
    return true;

  const CXXRecordDecl *RD = dyn_cast<CXXRecordDecl>(RTLoc.getDecl());
  if (!RD)
    return true;

  std::string Name;
  if (!getNewName(RD, Name))
    return true;

  // Let VisitCXXRecordDecl handle this case.
  if (ConsumerInstance->isDeclaringRecordDecl(RD))
    return true;

  // Avoid duplicated rewrites to Decls from the same DeclGroup, e.g.,
  // struct S s1, s2
  SourceLocation LocStart = RTLoc.getBeginLoc();
  void *LocPtr = LocStart.getPtrEncoding();
  if (VisitedLocs.count(LocPtr))
    return true;
  VisitedLocs.insert(LocPtr);
  RewriteHelper->replaceRecordType(RTLoc, Name);
  return true;
}

template<typename T> bool CommonRenameClassRewriteVisitor<T>::
  VisitDependentTemplateSpecializationTypeLoc(
    DependentTemplateSpecializationTypeLoc DTSLoc)
{
  const Type *Ty = DTSLoc.getTypePtr();
  const DependentTemplateSpecializationType *DTST = 
    dyn_cast<DependentTemplateSpecializationType>(Ty);
  TransAssert(DTST && "Bad DependentTemplateSpecializationType!");

  const IdentifierInfo *IdInfo = DTST->getIdentifier();
  std::string IdName = IdInfo->getName();
  std::string Name;
  if (getNewNameByName(IdName, Name)) {
    SourceLocation LocStart = DTSLoc.getTemplateNameLoc();
    TheRewriter->ReplaceText(LocStart, IdName.size(), Name);
  }

  return true;
}

template<typename T>
void CommonRenameClassRewriteVisitor<T>::renameTemplateName(
       TemplateName TmplName, SourceLocation LocStart)
{
  const TemplateDecl *TmplD = TmplName.getAsTemplateDecl();
  TransAssert(TmplD && "Invalid TemplateDecl!");
  NamedDecl *ND = TmplD->getTemplatedDecl();
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
    return;

  const CXXRecordDecl *CXXRD = dyn_cast<CXXRecordDecl>(ND);
  if (!CXXRD)
    return;

  std::string Name;
  if (getNewName(CXXRD, Name)) {
    TheRewriter->ReplaceText(LocStart, CXXRD->getNameAsString().size(), Name);
  }
}

// ISSUE: we don't have loc info for TemplateName, so we have to
//        overload RecursiveASTVisitor TraverseTemplateArgumentLoc
//        function, or else we will omit arguments for template template
//        parameters
template<typename T>
bool CommonRenameClassRewriteVisitor<T>::TraverseTemplateArgumentLoc(
       const TemplateArgumentLoc &ArgLoc) 
{
  const TemplateArgument &Arg = ArgLoc.getArgument();

  switch (Arg.getKind()) {
  case TemplateArgument::NullPtr:
  case TemplateArgument::Null:
  case TemplateArgument::Declaration:
  case TemplateArgument::Integral:
    return true;

  case TemplateArgument::Type: {
    if (TypeSourceInfo *TSI = ArgLoc.getTypeSourceInfo())
      return getDerived().TraverseTypeLoc(TSI->getTypeLoc());
    else
      return getDerived().TraverseType(Arg.getAsType());
  }

  case TemplateArgument::Template:
  case TemplateArgument::TemplateExpansion: {
    if (ArgLoc.getTemplateQualifierLoc()) {
      getDerived().TraverseNestedNameSpecifierLoc(
                                            ArgLoc.getTemplateQualifierLoc());
    }

    TemplateName TmplName = Arg.getAsTemplateOrTemplatePattern();
    getDerived().TraverseTemplateName(Arg.getAsTemplateOrTemplatePattern());
    renameTemplateName(TmplName, ArgLoc.getLocation());
    return true;
  }

  case TemplateArgument::Expression:
    return getDerived().TraverseStmt(ArgLoc.getSourceExpression());

  case TemplateArgument::Pack:
    return getDerived().TraverseTemplateArguments(Arg.pack_begin(),
                                                  Arg.pack_size());
  }

  return true;
}

template<typename T>
bool CommonRenameClassRewriteVisitor<T>::VisitTemplateSpecializationTypeLoc(
       TemplateSpecializationTypeLoc TSPLoc)
{
  const Type *Ty = TSPLoc.getTypePtr();
  const TemplateSpecializationType *TST = 
    dyn_cast<TemplateSpecializationType>(Ty);
  TransAssert(TST && "Bad TemplateSpecializationType!");

  TemplateName TmplName = TST->getTemplateName();
  renameTemplateName(TmplName, TSPLoc.getTemplateNameLoc());
  return true;
}

template<typename T>
bool CommonRenameClassRewriteVisitor<T>::getNewName(const CXXRecordDecl *CXXRD,
                             std::string &NewName)
{
  const CXXRecordDecl *CanonicalRD = CXXRD->getCanonicalDecl();
  if (CanonicalRD == TheCXXRecordDecl) {
    NewName = NewNameStr;
    return true;
  }
  else {
    NewName = "";
    return false;
  }
}

template<typename T>
bool CommonRenameClassRewriteVisitor<T>::getNewNameByName(
       const std::string &Name, std::string &NewName)
{
  if (TheCXXRecordDecl && (Name == TheCXXRecordDecl->getNameAsString())) {
    NewName = NewNameStr;
    return true;
  }
  else {
    NewName = "";
    return false;
  }
}

} // end namespace clang_delta_common_visitor

#endif

