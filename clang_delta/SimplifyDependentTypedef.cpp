//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2014, 2015, 2017 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "SimplifyDependentTypedef.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Sometimes the underlying type of a typedef declaration \
is a complex dependent type which prevents further reduction. \
This pass tries to replace this complex dependent type with a \
simple one. For example, from \n\
  template<typename T> class { \n\
    typedef typename X< typename Y<T> >::type type; \n\
  }; \n\
to \n\
  template<typename T> class { \n\
    typedef T type; \n\
  };\n";

static RegisterTransformation<SimplifyDependentTypedef>
         Trans("simplify-dependent-typedef", DescriptionMsg);

class DependentTypedefCollectionVisitor : public
  RecursiveASTVisitor<DependentTypedefCollectionVisitor> {

public:
  explicit DependentTypedefCollectionVisitor(SimplifyDependentTypedef *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitTypedefDecl(TypedefDecl *D);

private:
  SimplifyDependentTypedef *ConsumerInstance;

};

class DependentTypedefTemplateTypeParmTypeVisitor : public
  RecursiveASTVisitor<DependentTypedefTemplateTypeParmTypeVisitor> {

  typedef llvm::SmallPtrSet<const clang::Type *, 5> TemplateTypeParmTypeSet;

public:
  explicit DependentTypedefTemplateTypeParmTypeVisitor(
             SimplifyDependentTypedef *Instance)
    : IsValidType(false)
  { }

  bool VisitTemplateTypeParmType(TemplateTypeParmType *Ty);

  void setTypeSet(TemplateTypeParmTypeSet *Set) {
    TypeSet = Set;
  }

  void setValidType(bool Valid) {
    IsValidType = Valid;
  }

  bool isValidType(void) { 
    return IsValidType; 
  }

private:

  TemplateTypeParmTypeSet *TypeSet;

  bool IsValidType;
};

bool DependentTypedefCollectionVisitor::VisitTypedefDecl(TypedefDecl *D)
{
  ConsumerInstance->handleOneTypedefDecl(D);
  return true;
}

bool DependentTypedefTemplateTypeParmTypeVisitor::VisitTemplateTypeParmType(
       TemplateTypeParmType *Ty)
{
  const Type *CanonicalTy = 
      Ty->getCanonicalTypeInternal().getTypePtr();
  if (TypeSet->count(CanonicalTy)) {
    IsValidType = true;
    return false;
  }
  return true;
}

void SimplifyDependentTypedef::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new DependentTypedefCollectionVisitor(this);
  TemplateTypeParmTypeVisitor = 
    new DependentTypedefTemplateTypeParmTypeVisitor(this);
}

void SimplifyDependentTypedef::HandleTranslationUnit(ASTContext &Ctx)
{
  if (TransformationManager::isCLangOpt() ||
      TransformationManager::isOpenCLLangOpt()) {
    ValidInstanceNum = 0;
  }

  CollectionVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);
  TransAssert(TheTypedefDecl && "NULL TheTypedefDecl!");
  TransAssert(FirstTmplTypeParmD && "NULL FirstTmplTypeParmD!");
  rewriteTypedefDecl();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void SimplifyDependentTypedef::rewriteTypedefDecl(void)
{
  SourceLocation LocStart = TheTypedefDecl->getLocStart();

  // skip "typedef "
  LocStart = LocStart.getLocWithOffset(8);
  SourceLocation LocEnd = TheTypedefDecl->getLocation();
  LocEnd = LocEnd.getLocWithOffset(-1);

  std::string ParmName = FirstTmplTypeParmD->getNameAsString(); 
  TransAssert(!ParmName.empty() && "Invalid TypeParmType Name!");
  // make an explicit blank after the type name in case we
  // have typedef XXX<T>type;
  TheRewriter.ReplaceText(SourceRange(LocStart, LocEnd), ParmName+" ");
}

void SimplifyDependentTypedef::handleOneTypedefDecl(const TypedefDecl *D)
{
  if (isInIncludedFile(D))
    return;

  const TypedefDecl *CanonicalD = dyn_cast<TypedefDecl>(D->getCanonicalDecl());
  TransAssert(CanonicalD && "Bad TypedefDecl!");
  if (VisitedTypedefDecls.count(CanonicalD))
    return;
  VisitedTypedefDecls.insert(CanonicalD);

  const CXXRecordDecl *CXXRD = dyn_cast<CXXRecordDecl>(D->getDeclContext());
  if (!CXXRD)
    return;

  const ClassTemplateDecl *TmplD = CXXRD->getDescribedClassTemplate();
  if (!TmplD)
    return;
  
  TemplateParameterList *TmplParmList = TmplD->getTemplateParameters();
  if (TmplParmList->size() == 0)
    return;

  TemplateTypeParmTypeSet TypeSet;
  const TemplateTypeParmDecl *FirstParmD = NULL;
  for (TemplateParameterList::iterator I = TmplParmList->begin(),
       E = TmplParmList->end(); I != E; ++I) {
    if (const TemplateTypeParmDecl *TmplTypeParmD = 
        dyn_cast<TemplateTypeParmDecl>(*I)) {
      if (!FirstParmD && !TmplTypeParmD->getNameAsString().empty())
        FirstParmD = TmplTypeParmD;
      const TemplateTypeParmType *TmplParmTy = 
        dyn_cast<TemplateTypeParmType>(TmplTypeParmD->getTypeForDecl());
      TransAssert(TmplParmTy && "Bad TemplateTypeParmType!");
      TypeSet.insert(TmplParmTy->getCanonicalTypeInternal().getTypePtr());
    }
  }

  if (!FirstParmD)
    return;

  QualType QT = CanonicalD->getUnderlyingType();
  const Type *Ty = QT.getTypePtr();
  Type::TypeClass TC = Ty->getTypeClass();
  if ((TC != Type::DependentName) &&
      (TC != Type::DependentTemplateSpecialization) &&
      (TC != Type::TemplateSpecialization) &&
      (TC != Type::Elaborated))
    return;

  TemplateTypeParmTypeVisitor->setTypeSet(&TypeSet);
  TemplateTypeParmTypeVisitor->setValidType(false);
  TemplateTypeParmTypeVisitor->TraverseType(QT);

  if (!TemplateTypeParmTypeVisitor->isValidType())
    return;

  ValidInstanceNum++;
  if (ValidInstanceNum != TransformationCounter)
    return;
    
  FirstTmplTypeParmD = FirstParmD;
  TheTypedefDecl = CanonicalD;
}

SimplifyDependentTypedef::~SimplifyDependentTypedef(void)
{
  delete CollectionVisitor;
  delete TemplateTypeParmTypeVisitor;
}

