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

#include <sstream>
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

  bool VisitRecordTypeLoc(RecordTypeLoc RTLoc);

  bool VisitTemplateSpecializationTypeLoc(
         TemplateSpecializationTypeLoc TSPLoc);

  bool VisitNestedNameSpecifierLoc(NestedNameSpecifierLoc QualifierLoc);

  bool VisitUsingDecl(UsingDecl *D);
  
  bool VisitUsingDirectiveDecl(UsingDirectiveDecl *D);

  bool VisitUnresolvedUsingValueDecl(UnresolvedUsingValueDecl *D);

  bool VisitUnresolvedUsingTypenameDecl(UnresolvedUsingTypenameDecl *D);

private:
  RenameClass *ConsumerInstance;

};

bool RenameClassASTVisitor::VisitCXXRecordDecl(CXXRecordDecl *CXXRD)
{
  ConsumerInstance->analyzeOneRecordDecl(CXXRD);
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

  // Avoid duplicated Visit Dtor. 
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

bool RenameClassRewriteVisitor::VisitCXXMemberCallExpr(CXXMemberCallExpr *CE)
{
  const CXXMethodDecl *MD = CE->getMethodDecl();
  const DeclContext *Ctx = MD->getDeclContext();
  const CXXRecordDecl *CXXRD = dyn_cast<CXXRecordDecl>(Ctx);
  TransAssert(CXXRD && "Invalid CXXRecordDecl");

  std::string Name;
  if (!ConsumerInstance->getNewName(CXXRD, Name))
    return true;

  const CXXDestructorDecl *DtorDecl = dyn_cast<CXXDestructorDecl>(MD);
  if (!DtorDecl)
    return true;

  Name = "~" + Name;

  std::string ExprStr;
  ConsumerInstance->RewriteHelper->getExprString(CE, ExprStr);
  std::string OldDtorName = DtorDecl->getNameAsString();
  size_t Pos = ExprStr.find(OldDtorName);
  TransAssert((Pos != std::string::npos) && "Bad Name Position!");
  if (Pos == 0)
    return true;

  SourceLocation StartLoc = CE->getLocStart();
  StartLoc = StartLoc.getLocWithOffset(Pos);

  ConsumerInstance->TheRewriter.ReplaceText(StartLoc, OldDtorName.size(), Name);
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
    const IdentifierInfo *TypeId = RTLoc.getType().getBaseTypeIdentifier();
    SourceLocation LocStart = RTLoc.getLocStart();
    ConsumerInstance->TheRewriter.ReplaceText(
      LocStart, TypeId->getLength(), Name);
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
  TransAssert(ND && "Invalid NamedDecl!");

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

bool RenameClassRewriteVisitor::VisitNestedNameSpecifierLoc(
       NestedNameSpecifierLoc QualifierLoc)
{
  SmallVector<NestedNameSpecifierLoc, 8> QualifierLocs;
  for (; QualifierLoc; QualifierLoc = QualifierLoc.getPrefix())
    QualifierLocs.push_back(QualifierLoc);

  while (!QualifierLocs.empty()) {
    NestedNameSpecifierLoc Loc = QualifierLocs.pop_back_val();
    NestedNameSpecifier *NNS = Loc.getNestedNameSpecifier();
    NestedNameSpecifier::SpecifierKind Kind = NNS->getKind();
    switch (Kind) {
      case NestedNameSpecifier::TypeSpec: {
        const Type *Ty = NNS->getAsType();
        const CXXRecordDecl *CXXRD = Ty->getAsCXXRecordDecl();
        if (CXXRD) {
          ConsumerInstance->rewriteClassName(CXXRD, NNS, Loc);
        }
        break;
      }

      case NestedNameSpecifier::TypeSpecWithTemplate: {
        const Type *Ty = NNS->getAsType();
        if ( const TemplateSpecializationType *TST = 
             dyn_cast<TemplateSpecializationType>(Ty) ) {
          TemplateName TplName = TST->getTemplateName();
          const TemplateDecl *TplD = TplName.getAsTemplateDecl();
          TransAssert(TplD && "Invalid TemplateDecl!");
          NamedDecl *ND = TplD->getTemplatedDecl();
          TransAssert(ND && "Invalid NamedDecl!");
          const CXXRecordDecl *CXXRD = dyn_cast<CXXRecordDecl>(ND);
          if (CXXRD)
            ConsumerInstance->rewriteClassName(CXXRD, NNS, Loc);
        }
        break;
      }

      case NestedNameSpecifier::NamespaceAlias: // Fall-through
      case NestedNameSpecifier::Identifier: // Fall-through
      case NestedNameSpecifier::Global: // Fall-through
      case NestedNameSpecifier::Namespace:
        break;
 
      default:
        TransAssert(0 && "Unreachable code: invalid SpecifierKind!");
    }
  }
  return true;
}

// e.g., using namespace_XX::identifie_YY
bool RenameClassRewriteVisitor::VisitUsingDecl(UsingDecl *D)
{
  NestedNameSpecifierLoc QualifierLoc = D->getQualifierLoc();
  if (!QualifierLoc)
    return true;
  return VisitNestedNameSpecifierLoc(QualifierLoc);
}

// e.g., using namespace std
bool RenameClassRewriteVisitor::VisitUsingDirectiveDecl(UsingDirectiveDecl *D)
{
  NestedNameSpecifierLoc QualifierLoc = D->getQualifierLoc();
  if (!QualifierLoc)
    return true;
  return VisitNestedNameSpecifierLoc(QualifierLoc);
}

// e.g., class A : public Base<T> { using Base<T>::foo; };
bool RenameClassRewriteVisitor::VisitUnresolvedUsingValueDecl(
       UnresolvedUsingValueDecl *D)
{
  NestedNameSpecifierLoc QualifierLoc = D->getQualifierLoc();
  if (!QualifierLoc)
    return true;
  return VisitNestedNameSpecifierLoc(QualifierLoc);
}

// e.g., class A : public Base<T> { using typename Base<T>::foo; };
bool RenameClassRewriteVisitor::VisitUnresolvedUsingTypenameDecl(
       UnresolvedUsingTypenameDecl *D)
{
  NestedNameSpecifierLoc QualifierLoc = D->getQualifierLoc();
  if (!QualifierLoc)
    return true;
  return VisitNestedNameSpecifierLoc(QualifierLoc);
}

void RenameClass::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new RenameClassASTVisitor(this);
  RewriteVisitor = new RenameClassRewriteVisitor(this);
}

bool RenameClass::HandleTopLevelDecl(DeclGroupRef D) 
{
  if (TransformationManager::isCLangOpt()) {
    return true;
  }

  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    CollectionVisitor->TraverseDecl(*I);
  }
  return true;
}
 
void RenameClass::HandleTranslationUnit(ASTContext &Ctx)
{
  if (TransformationManager::isCLangOpt()) {
    ValidInstanceNum = 0;
  }
  else {
    doAnalysis();
  }

  if (QueryInstanceOnly)
    return;

  if (NewName > 'Z') {
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
  bool RV;
  if (CanonicalRD == TheCXXRecordDecl) {
    NewName = NewNameStr;
    RV = true;
  }
  else if (CanonicalRD == ConflictingRD) {
    NewName = BackupName;
    RV = true;
  }
  else {
    NewName = "";
    RV = false;
  }
  return RV;
}

void RenameClass::rewriteClassName(const CXXRecordDecl *CXXRD, 
                                   NestedNameSpecifier *NNS,
                                   NestedNameSpecifierLoc Loc)
{
  std::string Name;
  if (!getNewName(CXXRD, Name))
    return;

  Name += "::";
  SourceRange LocRange = Loc.getLocalSourceRange();
  TheRewriter.ReplaceText(LocRange, Name);
}

bool RenameClass::matchCurrentName(const std::string &Name)
{
  if (!isValidName(Name))
    return false;

  return (Name[0] == CurrentName);
}

void RenameClass::incValidInstance(const CXXRecordDecl *CXXRD)
{
  ValidInstanceNum++;
  if (ValidInstanceNum != TransformationCounter)
    return;

  TheCXXRecordDecl = CXXRD;
  NewName = CurrentName;
  NewNameStr.assign(1, NewName);
}

bool RenameClass::isReservedName(char C)
{
  return ((C == 'E') || (C == 'T') || (C == 'U'));
}

void RenameClass::incCurrentName(void)
{
  if (CurrentName > 'Z')
    return;

  CurrentName++;
  if (CurrentName > 'Z')
    return;

  while (isReservedName(CurrentName)) {
    if (CurrentName > 'Z')
      return;
    CurrentName++;
  }
}

void RenameClass::setBackupName(ClassNameSet &AllClassNames)
{
  ConflictingRD = NameToRecord[NewName];
  if (!ConflictingRD || (NewName > 'Z'))
    return;

  for (unsigned I = 0; I < 23; ++I) {
    std::stringstream SS;
    SS << NewName << "_" << I;
    BackupName = SS.str();
    if (!AllClassNames.count(BackupName)) {
      return;
    }
  }
  TransAssert(0 && "Unreachable code");
}

bool RenameClass::doAnalysis(void)
{
  ClassNameSet AllClassNames;

  for (unsigned Level = 0; Level <= MaxInheritanceLevel; ++Level) {
    CXXRecordDeclSet *RDSet = LevelToRecords[Level];
    if (!RDSet)
      continue;

    for (CXXRecordDeclSet::const_iterator I = RDSet->begin(),
         E = RDSet->end(); I != E; ++I) {
      const CXXRecordDecl *CXXRD = (*I);
      std::string RDName = CXXRD->getNameAsString();
      AllClassNames.insert(RDName);

      if (!matchCurrentName(RDName)) {
        incValidInstance(CXXRD);
      }

      incCurrentName();
    }
  }

  setBackupName(AllClassNames);
  return true;
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

void RenameClass::analyzeOneRecordDecl(const CXXRecordDecl *CXXRD)
{
  if (isSpecialRecordDecl(CXXRD))
    return;

  const CXXRecordDecl *CanonicalRD = CXXRD->getCanonicalDecl();
  if (RecordToLevel.find(CanonicalRD) != RecordToLevel.end())
    return;

  unsigned NumBases = 0;

  // getNumBases dies on the case where CXXRD has no definition.
  if (CanonicalRD->hasDefinition())
    NumBases = CanonicalRD->getNumBases();

  unsigned Level = 0;
  if (NumBases > 0) {
    for (CXXRecordDecl::base_class_const_iterator I = 
         CanonicalRD->bases_begin(), E = CanonicalRD->bases_end(); I != E; ++I) {

      const CXXBaseSpecifier *BS = I;
      const Type *T = BS->getType().getTypePtr();
      const CXXRecordDecl *Base;
      if ( const TemplateSpecializationType *TST = 
           dyn_cast<TemplateSpecializationType>(T) ) {
        TemplateName TplName = TST->getTemplateName();
        const TemplateDecl *TplD = TplName.getAsTemplateDecl();
        TransAssert(TplD && "Invalid TemplateDecl!");
        NamedDecl *ND = TplD->getTemplatedDecl();
        TransAssert(ND && "Invalid NamedDecl!");
        Base = dyn_cast<CXXRecordDecl>(ND);
      }
      else if ( const DependentTemplateSpecializationType *DTST = 
                dyn_cast<DependentTemplateSpecializationType>(T) ) {
        (void)DTST;
        TransAssert(0 && "We cannot have DependentTemplateSpecializationType \
                         here!");
      }
      else {
        Base = T->getAsCXXRecordDecl();
      }

      TransAssert(Base && "Bad base class type!");

      RecordToInheritanceLevelMap::iterator LI = 
        RecordToLevel.find(Base->getCanonicalDecl());
      TransAssert((LI != RecordToLevel.end()) && "Unknown base class!");
      unsigned BaseLevel = (*LI).second;
      if (BaseLevel > Level)
        Level = BaseLevel;
    }
    Level++;
  }
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
    NameToRecord[C] = CanonicalRD;
  }
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

