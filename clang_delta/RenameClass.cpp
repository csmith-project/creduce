//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "RenameClass.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;
using namespace llvm;

static const char *DescriptionMsg =
"To increase readability, simplify class names to [A - Z] \
(except E, T and Z). Class names from the same hierarchy tree will \
have alphabetical order. For example, we could have a transformed \
code shown as below: \n\
  class A {}; \n\
  class B : public A {}; \n\
  class C : public B {}; \n";

static RegisterTransformation<RenameClass>
         Trans("rename-class", DescriptionMsg);

class RenameClassASTVisitor : public 
  RecursiveASTVisitor<RenameClassASTVisitor> {

public:
  explicit RenameClassASTVisitor(RenameClass *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitCXXRecordDecl(CXXRecordDecl *CXXRD);

private:
  RenameClass *ConsumerInstance;

};

class RenameClassRewriteVisitor : public 
  RecursiveASTVisitor<RenameClassRewriteVisitor> {

public:
  explicit RenameClassRewriteVisitor(RenameClass *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitCXXRecordDecl(CXXRecordDecl *CXXRD);

  bool VisitCXXConstructorDecl(CXXConstructorDecl *CtorDecl);

  bool VisitCXXDestructorDecl(CXXDestructorDecl *DtorDecl);

  bool VisitCXXMemberCallExpr(CXXMemberCallExpr *CE);

  bool VisitInjectedClassNameTypeLoc(InjectedClassNameTypeLoc TyLoc);

  bool VisitRecordTypeLoc(RecordTypeLoc RTLoc);

  bool VisitTemplateSpecializationTypeLoc(
         TemplateSpecializationTypeLoc TSPLoc);

  bool VisitDependentTemplateSpecializationTypeLoc(
         DependentTemplateSpecializationTypeLoc DTSLoc);

  bool VisitClassTemplatePartialSpecializationDecl(
         ClassTemplatePartialSpecializationDecl *D);

  bool VisitClassTemplateSpecializationDecl(
         ClassTemplateSpecializationDecl *TSD);

private:
  RenameClass *ConsumerInstance;

};

bool RenameClassASTVisitor::VisitCXXRecordDecl(CXXRecordDecl *CXXRD)
{
  ConsumerInstance->analyzeOneRecordDecl(CXXRD);
  return true;
}

bool RenameClassRewriteVisitor::VisitClassTemplatePartialSpecializationDecl(
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

// ISSUE: I am not sure why, but RecursiveASTVisitor doesn't recursively
// visit base classes from explicit template specialization, e.g.,
//   struct A { };
//   template<typename T> class B : public A<T> { };
//   template<> class B : public A<short> { };
// In the above case, A<short> won't be touched.
// So we have to do it manually
bool RenameClassRewriteVisitor::VisitClassTemplateSpecializationDecl(
       ClassTemplateSpecializationDecl *TSD)
{
  if (!TSD->isExplicitSpecialization() || !TSD->isCompleteDefinition())
    return true;

  for (CXXRecordDecl::base_class_const_iterator I = TSD->bases_begin(),
       E = TSD->bases_end(); I != E; ++I) {
    TypeSourceInfo *TSI = (*I).getTypeSourceInfo();
    TransAssert(TSI && "Bad TypeSourceInfo!");
    TraverseTypeLoc(TSI->getTypeLoc());
  }
  return true;
}

bool RenameClassRewriteVisitor::VisitCXXRecordDecl(CXXRecordDecl *CXXRD)
{
  std::string Name;
  if (ConsumerInstance->getNewName(CXXRD, Name)) {
    ConsumerInstance->RewriteHelper->replaceRecordDeclName(CXXRD, Name);
  }

  return true;
}

bool RenameClassRewriteVisitor::VisitCXXConstructorDecl
       (CXXConstructorDecl *CtorDecl)
{
  const DeclContext *Ctx = CtorDecl->getDeclContext();
  const CXXRecordDecl *CXXRD = dyn_cast<CXXRecordDecl>(Ctx);
  TransAssert(CXXRD && "Invalid CXXRecordDecl");

  std::string Name;
  if (ConsumerInstance->getNewName(CXXRD, Name))
    ConsumerInstance->RewriteHelper->replaceFunctionDeclName(CtorDecl, Name);

  return true;
}

bool RenameClassRewriteVisitor::VisitCXXDestructorDecl(
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

  std::string Name;
  if (ConsumerInstance->getNewName(CXXRD, Name)) {
    Name = "~" + Name;
    ConsumerInstance->RewriteHelper->replaceFunctionDeclName(DtorDecl, Name);
  }

  return true;
}

bool RenameClassRewriteVisitor::VisitInjectedClassNameTypeLoc(
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

bool RenameClassRewriteVisitor::VisitCXXMemberCallExpr(CXXMemberCallExpr *CE)
{
  const CXXRecordDecl *CXXRD = CE->getRecordDecl();
  // getRecordDEcl could return NULL if getImplicitObjectArgument() 
  // returns NULL
  if (!CXXRD)
    return true;

  std::string Name;
  if (ConsumerInstance->getNewName(CXXRD, Name)) {
    ConsumerInstance->RewriteHelper->replaceCXXDtorCallExpr(CE, Name);
  }
  return true;
}

bool RenameClassRewriteVisitor::VisitRecordTypeLoc(RecordTypeLoc RTLoc)
{
  const Type *Ty = RTLoc.getTypePtr();
  if (Ty->isUnionType())
    return true;

  const CXXRecordDecl *RD = dyn_cast<CXXRecordDecl>(RTLoc.getDecl());
  if (!RD)
    return true;

  std::string Name;
  if (ConsumerInstance->getNewName(RD, Name)) {
    ConsumerInstance->RewriteHelper->replaceRecordType(RTLoc, Name);
  }
  return true;
}

bool RenameClassRewriteVisitor::VisitDependentTemplateSpecializationTypeLoc(
       DependentTemplateSpecializationTypeLoc DTSLoc)
{
  const Type *Ty = DTSLoc.getTypePtr();
  const DependentTemplateSpecializationType *DTST = 
    dyn_cast<DependentTemplateSpecializationType>(Ty);
  TransAssert(DTST && "Bad DependentTemplateSpecializationType!");

  const IdentifierInfo *IdInfo = DTST->getIdentifier();
  std::string IdName = IdInfo->getName();
  std::string Name;
  if (ConsumerInstance->getNewNameByName(IdName, Name)) {
    SourceLocation LocStart = DTSLoc.getTemplateNameLoc();
    ConsumerInstance->TheRewriter.ReplaceText(
      LocStart, IdName.size(), Name);
  }

  return true;
}

bool RenameClassRewriteVisitor::VisitTemplateSpecializationTypeLoc(
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

void RenameClass::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new RenameClassASTVisitor(this);
  RewriteVisitor = new RenameClassRewriteVisitor(this);
}

void RenameClass::HandleTranslationUnit(ASTContext &Ctx)
{
  if (TransformationManager::isCLangOpt()) {
    ValidInstanceNum = 0;
  }
  else {
    CollectionVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
    doAnalysis();
  }

  if (QueryInstanceOnly)
    return;

  if ((ValidInstanceNum + UsedNames.size() > 23)) {
    TransError = TransMaxClassesError;
    return;
  }
  else if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  TransAssert(RewriteVisitor && "NULL RewriteVisitor!");
  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  RewriteVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

bool RenameClass::getNewName(const CXXRecordDecl *CXXRD,
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

bool RenameClass::getNewNameByName(const std::string &Name,
                             std::string &NewName)
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

bool RenameClass::isReservedName(char C)
{
  return ((C == 'E') || (C == 'T') || (C == 'U'));
}

void RenameClass::incValidInstance(const CXXRecordDecl *CXXRD)
{
  ValidInstanceNum++;
  if (ValidInstanceNum != TransformationCounter)
    return;

  while (isReservedName(CurrentName) || UsedNames.count(CurrentName)) {
    if (CurrentName > 'Z')
      return;
    CurrentName++;
  }

  TheCXXRecordDecl = CXXRD;
  NewNameStr.assign(1, CurrentName);
}

void RenameClass::doAnalysis(void)
{
  for (unsigned Level = 0; Level <= MaxInheritanceLevel; ++Level) {
    CXXRecordDeclSet *RDSet = LevelToRecords[Level];
    if (!RDSet)
      continue;

    for (CXXRecordDeclSet::const_iterator I = RDSet->begin(),
         E = RDSet->end(); I != E; ++I) {
      const CXXRecordDecl *CXXRD = (*I);
      if (UsedNameDecls.count(CXXRD->getCanonicalDecl()))
        continue;

      incValidInstance(CXXRD);
    }
  }
}

bool RenameClass::isSpecialRecordDecl(const CXXRecordDecl *CXXRD)
{
  std::string Name = CXXRD->getNameAsString();
  return (Name == "__va_list_tag");
}

bool RenameClass::isValidName(const std::string &Name)
{
  if (Name.size() != 1)
    return false;

  char C = Name[0];
  return (((C >= 'A') || (C <= 'Z')) && !isReservedName(C));
}

const CXXRecordDecl *RenameClass::getBaseDeclFromTemplateSpecializationType(
        const TemplateSpecializationType *TSTy)
{
  TemplateName TplName = TSTy->getTemplateName();
  const TemplateDecl *TplD = TplName.getAsTemplateDecl();
  TransAssert(TplD && "Invalid TemplateDecl!");
  NamedDecl *ND = TplD->getTemplatedDecl();
  TransAssert(ND && "Invalid NamedDecl!");
  return dyn_cast<CXXRecordDecl>(ND);
}

// This function could return NULL
const CXXRecordDecl *RenameClass::getBaseDeclFromType(const Type *Ty)
{
  const CXXRecordDecl *Base = NULL;
  Type::TypeClass TyClass = Ty->getTypeClass();

  switch (TyClass) {
  case Type::TemplateSpecialization: {
    const TemplateSpecializationType *TSTy = 
      dyn_cast<TemplateSpecializationType>(Ty);
    Base = getBaseDeclFromTemplateSpecializationType(TSTy);
    TransAssert(Base && "Bad base class type!");
    return Base;
  }

  case Type::DependentTemplateSpecialization: {
    return NULL;
  }

  case Type::Elaborated: {
    const ElaboratedType *ETy = dyn_cast<ElaboratedType>(Ty);
    const Type *NamedT = ETy->getNamedType().getTypePtr();
    if ( const TemplateSpecializationType *TSTy = 
         dyn_cast<TemplateSpecializationType>(NamedT) ) {
      Base = getBaseDeclFromTemplateSpecializationType(TSTy);
    }
    else if ( const TypedefType * Ty = dyn_cast<TypedefType>(NamedT) ){
      Base = getBaseDeclFromType(Ty);
    }
    else {
      Base = ETy->getAsCXXRecordDecl();
    }
    TransAssert(Base && "Bad base class type from ElaboratedType!");
    return Base;
  }

  case Type::DependentName: {
    // It's not always the case that we could resolve a dependent name type.
    // For example, 
    //   template<typename T1, typename T2>
    //   struct AAA { typedef T2 new_type; };
    //   template<typename T3>
    //   struct BBB : public AAA<int, T3>::new_type { };
    // In the above code, we can't figure out what new_type refers to 
    // until BBB is instantiated
    // Due to this reason, simply return NULL from here.
    return NULL;
  }

  case Type::Typedef: {
    const TypedefType *TdefTy = dyn_cast<TypedefType>(Ty);
    const TypedefNameDecl *TdefD = TdefTy->getDecl();
    const Type *UnderlyingTy = TdefD->getUnderlyingType().getTypePtr();
    if ( const TemplateSpecializationType *TSTy = 
         dyn_cast<TemplateSpecializationType>(UnderlyingTy) ) {
      Base = getBaseDeclFromTemplateSpecializationType(TSTy);
    }
    else if (dyn_cast<DependentNameType>(UnderlyingTy)) {
      return NULL;
    }
    else {
      Base = UnderlyingTy->getAsCXXRecordDecl();
    }
    TransAssert(Base && "Bad base class type from Typedef!");
    return Base;
  }

  case Type::TemplateTypeParm: {
    // Yet another case we might not know the base class, e.g.,
    // template<typename T1> 
    // class AAA {
    //   struct BBB : T1 {};
    // };
    return NULL;
  }
  default:
    Base = Ty->getAsCXXRecordDecl();
    TransAssert(Base && "Bad base class type!");

    // getAsCXXRecordDecl could return a ClassTemplateSpecializationDecl.
    // For example:
    //   template <class T> class AAA { };
    //   typedef AAA<int> BBB;
    //   class CCC : BBB { };
    // In the above code, BBB is of type ClassTemplateSpecializationDecl
    if (const ClassTemplateSpecializationDecl *CTSDecl =
        dyn_cast<ClassTemplateSpecializationDecl>(Base)) {
      Base = CTSDecl->getSpecializedTemplate()->getTemplatedDecl();
      TransAssert(Base && 
                  "Bad base decl from ClassTemplateSpecializationDecl!");
    }
  }

  return Base;
}

void RenameClass::addOneRecordDecl(const CXXRecordDecl *CanonicalRD,
                                   unsigned Level)
{
  RecordToLevel[CanonicalRD] = Level;
  if (Level > MaxInheritanceLevel)
    MaxInheritanceLevel = Level;

  CXXRecordDeclSet *RDSet = LevelToRecords[Level];
  if (!RDSet) {
    RDSet = new CXXRecordDeclSet();
    TransAssert(RDSet && "Cannot new a CXXRecordDeclSet!");
    LevelToRecords[Level] = RDSet;
  }
  RDSet->insert(CanonicalRD);

  std::string RDName = CanonicalRD->getNameAsString();
  if (isValidName(RDName)) {
    char C = RDName[0];
    UsedNames.insert(C);
    UsedNameDecls.insert(CanonicalRD);
  }
}

void RenameClass::analyzeOneRecordDecl(const CXXRecordDecl *CXXRD)
{
  if (isSpecialRecordDecl(CXXRD))
    return;

  // Avoid duplication
  if (dyn_cast<ClassTemplateSpecializationDecl>(CXXRD))
    return;

  if (CXXRD->getNameAsString().empty())
    return;

  const CXXRecordDecl *CanonicalRD = CXXRD->getCanonicalDecl();
  if (RecordToLevel.find(CanonicalRD) != RecordToLevel.end())
    return;

  if (!CXXRD->hasDefinition()) {
      addOneRecordDecl(CanonicalRD, 0);
      return;
  }

  // getNumBases dies on the case where CXXRD has no definition.
  unsigned NumBases = CanonicalRD->getNumBases();

  // in some cases, we will encounter a derived classs before
  // its base class,
  // namespace NS1 {
  //   class Derived;
  //   class Base {};
  //   class Derived: public Base { };
  // }
  if (NumBases == 0) {
    addOneRecordDecl(CanonicalRD, 0);
    return;
  }

  unsigned Level = 0;
  for (CXXRecordDecl::base_class_const_iterator I = 
       CanonicalRD->bases_begin(), E = CanonicalRD->bases_end(); I != E; ++I) {

    const CXXBaseSpecifier *BS = I;
    const Type *Ty = BS->getType().getTypePtr();
    const CXXRecordDecl *Base = getBaseDeclFromType(Ty);
    // If cannot find CXXRD's base class, then we assume its base class
    // has the level of 0
    if (!Base)
      continue;

    const CXXRecordDecl *CanonicalBase = Base->getCanonicalDecl();
    if (CanonicalBase == CanonicalRD);
      continue;

    unsigned BaseLevel;
    RecordToInheritanceLevelMap::iterator LI = 
      RecordToLevel.find(CanonicalBase);

    if (LI == RecordToLevel.end()) {
      continue;
    }

    BaseLevel = (*LI).second;
    if (BaseLevel > Level)
      Level = BaseLevel;
  }
  Level++;

  addOneRecordDecl(CanonicalRD, Level);
}

RenameClass::~RenameClass(void)
{
  for (InheritanceLevelToRecordsMap::iterator I = LevelToRecords.begin(),
       E = LevelToRecords.end(); I != E; ++I) {
    delete (*I).second;
  }

  if (CollectionVisitor)
    delete CollectionVisitor;
  if (RewriteVisitor)
    delete RewriteVisitor;
}

