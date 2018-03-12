//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2015, 2016, 2017 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "RenameClass.h"

#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "CommonRenameClassRewriteVisitor.h"
#include "TransformationManager.h"

using namespace clang;
using namespace clang_delta_common_visitor;

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
  CommonRenameClassRewriteVisitor<RenameClassRewriteVisitor> 
{
public:
  RenameClassRewriteVisitor(Rewriter *RT, 
                            RewriteUtils *Helper,
                            const CXXRecordDecl *CXXRD,
                            const std::string &Name)
    : CommonRenameClassRewriteVisitor<RenameClassRewriteVisitor>
      (RT, Helper, CXXRD, Name)
  { }
};

bool RenameClassASTVisitor::VisitCXXRecordDecl(CXXRecordDecl *CXXRD)
{
  ConsumerInstance->analyzeOneRecordDecl(CXXRD);
  return true;
}

void RenameClass::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new RenameClassASTVisitor(this);
}

void RenameClass::HandleTranslationUnit(ASTContext &Ctx)
{
  if (TransformationManager::isCLangOpt() ||
      TransformationManager::isOpenCLLangOpt()) {
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

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  RewriteVisitor = 
    new RenameClassRewriteVisitor(&TheRewriter, RewriteHelper, 
                                  TheCXXRecordDecl, NewNameStr);

  TransAssert(RewriteVisitor && "NULL RewriteVisitor!");
  RewriteVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
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
      if (isInIncludedFile(CXXRD))
        continue;

      incValidInstance(CXXRD);
    }
  }
}

bool RenameClass::isValidName(const std::string &Name)
{
  if (Name.size() != 1)
    return false;

  char C = Name[0];
  return (((C >= 'A') && (C <= 'Z')) && !isReservedName(C));
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
    if (CanonicalBase == CanonicalRD)
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

  delete CollectionVisitor;
  delete RewriteVisitor;
}

