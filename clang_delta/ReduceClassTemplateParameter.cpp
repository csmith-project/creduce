//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ReduceClassTemplateParameter.h"

#include "llvm/ADT/DenseMap.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;
using namespace llvm;

static const char *DescriptionMsg = 
"This pass tries to remove one unused parameter from a class template \
declaration and also erase the corresponding template argument \
from template instantiations/specializations. Note that this pass \
does not target those templates with single argument, and skips \
variadic templates as well. ";

static RegisterTransformation<ReduceClassTemplateParameter>
         Trans("reduce-class-template-param", DescriptionMsg);

class ReduceClassTemplateParameterASTVisitor : public 
  RecursiveASTVisitor<ReduceClassTemplateParameterASTVisitor> {

public:
  explicit ReduceClassTemplateParameterASTVisitor(
             ReduceClassTemplateParameter *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitClassTemplateDecl(ClassTemplateDecl *D);

private:
  ReduceClassTemplateParameter *ConsumerInstance;

};

namespace {

class TemplateParameterVisitor : public 
  RecursiveASTVisitor<TemplateParameterVisitor> {

public:
  TemplateParameterVisitor(void)
             : UsedParameters(NULL) { 
    UsedParameters = new TemplateParameterSet();
  }

  ~TemplateParameterVisitor() {
    delete UsedParameters;
  };

  bool VisitTemplateTypeParmType(TemplateTypeParmType *Ty);

  bool isUsedParam(const NamedDecl *ND) {
    return UsedParameters->count(ND);
  }

private:

  typedef SmallPtrSet<const NamedDecl *, 8> TemplateParameterSet;

  TemplateParameterSet *UsedParameters;
};

bool TemplateParameterVisitor::VisitTemplateTypeParmType(
       TemplateTypeParmType *Ty)
{
  const TemplateTypeParmDecl *D = Ty->getDecl();
  UsedParameters->insert(D);
  return true;
}

class ArgumentDependencyVisitor : public 
  RecursiveASTVisitor<ArgumentDependencyVisitor> {

public:
  typedef llvm::DenseMap<const Type *, unsigned> TypeToVisitsCountSet;

  explicit ArgumentDependencyVisitor(TypeToVisitsCountSet &CounterSet)
             : VisitsCountSet(CounterSet)
  { }

  bool VisitTemplateTypeParmType(TemplateTypeParmType *Ty);

private:

  TypeToVisitsCountSet &VisitsCountSet;
};

bool ArgumentDependencyVisitor::VisitTemplateTypeParmType(
       TemplateTypeParmType *Ty)
{
  TypeToVisitsCountSet::iterator I = VisitsCountSet.find(Ty);
  if (I != VisitsCountSet.end()) {
    unsigned Count = (*I).second + 1;
    VisitsCountSet[(*I).first] = Count;
  }
  return true;
}

}

class ReduceClassTemplateParameterRewriteVisitor : public 
  RecursiveASTVisitor<ReduceClassTemplateParameterRewriteVisitor> {

public:
  explicit ReduceClassTemplateParameterRewriteVisitor(
             ReduceClassTemplateParameter *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitTemplateSpecializationTypeLoc(TemplateSpecializationTypeLoc Loc);

private:

  ReduceClassTemplateParameter *ConsumerInstance;
};

bool ReduceClassTemplateParameterRewriteVisitor::
       VisitTemplateSpecializationTypeLoc(TemplateSpecializationTypeLoc Loc)
{
  const TemplateSpecializationType *Ty = 
    dyn_cast<TemplateSpecializationType>(Loc.getTypePtr());
  TransAssert(Ty && "Invalid TemplateSpecializationType!");

  TemplateName TmplName = Ty->getTemplateName();
  if (!ConsumerInstance->referToTheTemplateDecl(TmplName))
    return true;

  unsigned NumArgs = Loc.getNumArgs();
  if ((ConsumerInstance->TheParameterIndex >= NumArgs) && 
      ConsumerInstance->hasDefaultArg)
    return true;

  TransAssert((ConsumerInstance->TheParameterIndex < NumArgs) &&
              "TheParameterIndex cannot be greater than NumArgs!");
  TemplateArgumentLoc ArgLoc = Loc.getArgLoc(ConsumerInstance->TheParameterIndex);
  SourceRange Range = ArgLoc.getSourceRange();

  if (NumArgs == 1) {
    ConsumerInstance->TheRewriter.ReplaceText(SourceRange(Loc.getLAngleLoc(),
                                                          Loc.getRAngleLoc()),
                                              "<>");
  }
  else if ((ConsumerInstance->TheParameterIndex + 1) == NumArgs) {
    SourceLocation EndLoc = Loc.getRAngleLoc();
    EndLoc = EndLoc.getLocWithOffset(-1);
    ConsumerInstance->RewriteHelper->removeTextFromLeftAt(
                                       Range, ',', EndLoc);
  }
  else {
    ConsumerInstance->RewriteHelper->removeTextUntil(Range, ',');
  }
  return true;
}

bool ReduceClassTemplateParameterASTVisitor::VisitClassTemplateDecl(
       ClassTemplateDecl *D)
{
  ClassTemplateDecl *CanonicalD = D->getCanonicalDecl();
  if (ConsumerInstance->VisitedDecls.count(CanonicalD))
    return true;

  ConsumerInstance->VisitedDecls.insert(CanonicalD);
  if (!ConsumerInstance->isValidClassTemplateDecl(D))
    return true;

  TemplateParameterVisitor ParameterVisitor;
  CXXRecordDecl *CXXRD = D->getTemplatedDecl();
  if (CXXRecordDecl *Def = CXXRD->getDefinition())
    ParameterVisitor.TraverseDecl(Def);

  // ISSUE: we should also check the parameter usage for partial template
  //        specializations. For example:
  //   template<typename T1, typename T2> struct S{};
  //   template<typename T1, typename T2> struct<T1 *, T2 *> S{...};
  //   T1 or T2 could be used in "..."
  // Also, we could have another bad transformation, for example,
  //   template<bool, typename T> struct S{};
  //   template<typename T> struct<true, T> S{};
  // if we remove bool and true, we will have two definitions for S
  unsigned Index = 0;
  TemplateParameterList *TPList = CanonicalD->getTemplateParameters();
  for (TemplateParameterList::const_iterator I = TPList->begin(),
       E = TPList->end(); I != E; ++I) {
    const NamedDecl *ND = (*I);
    if (ParameterVisitor.isUsedParam(ND)) {
      Index++;
      continue;
    }

    ConsumerInstance->ValidInstanceNum++;
    if (ConsumerInstance->ValidInstanceNum == 
        ConsumerInstance->TransformationCounter) {
      ConsumerInstance->TheClassTemplateDecl = CanonicalD;
      ConsumerInstance->TheParameterIndex = Index;
      ConsumerInstance->TheTemplateName = new TemplateName(CanonicalD);
      ConsumerInstance->setDefaultArgFlag(ND);
    }
    Index++;
  }

  return true;
}

void ReduceClassTemplateParameter::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new ReduceClassTemplateParameterASTVisitor(this);
  ArgRewriteVisitor = new ReduceClassTemplateParameterRewriteVisitor(this);
}

void ReduceClassTemplateParameter::HandleTranslationUnit(ASTContext &Ctx)
{
  if (TransformationManager::isCLangOpt()) {
    ValidInstanceNum = 0;
  }
  else {
    CollectionVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
  }

  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  TransAssert(TheClassTemplateDecl && "NULL TheClassTemplateDecl!");
  TransAssert(ArgRewriteVisitor && "NULL ArgRewriteVisitor!");
  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  removeParameterFromDecl();
  removeParameterFromPartialSpecs();
  ArgRewriteVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void ReduceClassTemplateParameter::removeParameterByRange(SourceRange Range,
                                     const TemplateParameterList *TPList, 
                                     unsigned Index)
{
  unsigned NumParams = TPList->size();

  // if the parameter is the last one
  if (NumParams == 1) {
    TheRewriter.ReplaceText(SourceRange(TPList->getLAngleLoc(),
                                       TPList->getRAngleLoc()),
                            "<>");
  }
  else if ((Index + 1) == NumParams) {
    SourceLocation EndLoc = TPList->getRAngleLoc();
    EndLoc = EndLoc.getLocWithOffset(-1);
    RewriteHelper->removeTextFromLeftAt(Range, ',', EndLoc);
  }
  else {
    RewriteHelper->removeTextUntil(Range, ',');
  }
}

void ReduceClassTemplateParameter::removeParameterFromDecl(void)
{
  unsigned NumParams = TheClassTemplateDecl->getTemplateParameters()->size();
  
  TransAssert((NumParams > 1) && "Bad size of TheClassTemplateDecl!");

  for (ClassTemplateDecl::redecl_iterator 
         I = TheClassTemplateDecl->redecls_begin(), 
         E = TheClassTemplateDecl->redecls_end();
       I != E; ++I) {
    const TemplateParameterList *TPList = (*I)->getTemplateParameters();
    const NamedDecl *Param = TPList->getParam(TheParameterIndex);
    SourceRange Range = Param->getSourceRange();
    removeParameterByRange(Range, TPList, TheParameterIndex);
  }
}

void ReduceClassTemplateParameter::removeOneParameterByArgExpression(
       const ClassTemplatePartialSpecializationDecl *PartialD,
       const TemplateArgument &Arg)
{
  TransAssert((Arg.getKind() == TemplateArgument::Expression) && 
              "Arg is not TemplateArgument::Expression!");

  const Expr *E = Arg.getAsExpr();
  TransAssert(E && "Bad Expression!");
  const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E);
  TransAssert(DRE && "Bad DeclRefExpr!");
  const NonTypeTemplateParmDecl *ParmD = 
    dyn_cast<NonTypeTemplateParmDecl>(DRE->getDecl());
  TransAssert(ParmD && "Invalid NonTypeTemplateParmDecl!");
  
  const TemplateParameterList *TPList = PartialD->getTemplateParameters();
  unsigned Idx = 0;
  for (TemplateParameterList::const_iterator I = TPList->begin(),
       E = TPList->end(); I != E; ++I) {
    if ((*I) == ParmD)
      break;
    Idx++;
  }

  unsigned NumParams = TPList->size();
  TransAssert((Idx < NumParams) && "Cannot find valid TemplateParameter!");
  SourceRange Range = ParmD->getSourceRange();
  removeParameterByRange(Range, TPList, Idx);
}

void ReduceClassTemplateParameter::removeOneParameterByArgType(
       const ClassTemplatePartialSpecializationDecl *PartialD,
       const TemplateArgument &Arg)
{
  TransAssert((Arg.getKind() == TemplateArgument::Type) && 
              "Arg is not TemplateArgument::Type!");
  llvm::DenseMap<const Type *, unsigned> TypeToVisitsCount;
  llvm::DenseMap<const Type *, const NamedDecl *> TypeToNamedDecl;
  llvm::DenseMap<const Type *, unsigned> TypeToIndex;

  // retrieve all TemplateTypeParmType
  const TemplateParameterList *TPList = PartialD->getTemplateParameters();
  unsigned Idx = 0;
  for (TemplateParameterList::const_iterator I = TPList->begin(),
       E = TPList->end(); I != E; ++I) {
    const NamedDecl *ND = (*I);
    const TemplateTypeParmDecl *TypeD = dyn_cast<TemplateTypeParmDecl>(ND);
    if (!TypeD) {
      Idx++;
      continue;
    }
    const Type *ParmTy = TypeD->getTypeForDecl();
    TypeToVisitsCount[ParmTy] = 0;
    TypeToNamedDecl[ParmTy] = ND;
    TypeToIndex[ParmTy] = Idx;
  }

  QualType QTy = Arg.getAsType();
  ArgumentDependencyVisitor V(TypeToVisitsCount);
  // collect TemplateTypeParmType being used by Arg
  V.TraverseType(QTy);

  llvm::DenseMap<const Type *, unsigned> DependentTypeToVisitsCount;
  for (llvm::DenseMap<const Type *, unsigned>::iterator 
         I = TypeToVisitsCount.begin(), E = TypeToVisitsCount.end();
       I != E; ++I) {
    if ((*I).second > 0)
      DependentTypeToVisitsCount[(*I).first] = 1;
  }

  // check if the used TemplateTypeParmType[s] have dependencies
  // on other Args. If yes, we cannot remove it from the parameter list.
  // For example:
  //   template <typename T>
  //   struct S <T*, T&> {};
  // removing either of the arguments needs to keep the template 
  // parameter
  ArgumentDependencyVisitor AccumV(DependentTypeToVisitsCount);
  TemplateArgumentLoc *ArgLocs = PartialD->getTemplateArgsAsWritten();
  unsigned NumArgs = PartialD->getNumTemplateArgsAsWritten();
  TransAssert((TheParameterIndex < NumArgs) && 
               "Bad NumArgs from partial template decl!");
  for (unsigned I = 0; I < NumArgs; ++I) {
    if (I == TheParameterIndex)
      continue;
    
    TemplateArgumentLoc ArgLoc = ArgLocs[I];
    TemplateArgument OtherArg = ArgLoc.getArgument();
    if (OtherArg.isInstantiationDependent() && 
        (OtherArg.getKind() == TemplateArgument::Type)) {
      QualType QTy = OtherArg.getAsType();
      AccumV.TraverseType(QTy);
    }
  }

  for (llvm::DenseMap<const Type *, unsigned>::iterator 
         I = DependentTypeToVisitsCount.begin(), 
         E = DependentTypeToVisitsCount.end();
       I != E; ++I) {
    if ((*I).second != 1)
      continue;

    const NamedDecl *Param = TypeToNamedDecl[(*I).first];
    TransAssert(Param && "NULL Parameter!");
    SourceRange Range = Param->getSourceRange();
    removeParameterByRange(Range, TPList, TypeToIndex[(*I).first]);
  }
}

void ReduceClassTemplateParameter::removeOneParameterByArgTemplate(
       const ClassTemplatePartialSpecializationDecl *PartialD,
       const TemplateArgument &Arg)
{
  TransAssert((Arg.getKind() == TemplateArgument::Template) && 
              "Arg is not TemplateArgument::Template!");
  TemplateName TmplName = Arg.getAsTemplate();
  TransAssert((TmplName.getKind() == TemplateName::Template) &&
              "Invalid TemplateName Kind!");
  const TemplateDecl *TmplD = TmplName.getAsTemplateDecl();

  const TemplateParameterList *TPList = PartialD->getTemplateParameters();
  unsigned Idx = 0;
  for (TemplateParameterList::const_iterator I = TPList->begin(),
       E = TPList->end(); I != E; ++I) {
    if ((*I) == TmplD)
      break;
    Idx++;
  }

  unsigned NumParams = TPList->size();
  TransAssert((Idx < NumParams) && "Cannot find valid TemplateParameter!");
  SourceRange Range = TmplD->getSourceRange();
  removeParameterByRange(Range, TPList, Idx);
  
  return;
}

void ReduceClassTemplateParameter::removeOneParameterFromPartialDecl(
       const ClassTemplatePartialSpecializationDecl *PartialD,
       const TemplateArgument &Arg)
{
  if (!Arg.isInstantiationDependent())
    return;

  TemplateArgument::ArgKind K = Arg.getKind();
  switch (K) {
  case TemplateArgument::Expression:
    removeOneParameterByArgExpression(PartialD, Arg);
    return;

  case TemplateArgument::Template:
    removeOneParameterByArgTemplate(PartialD, Arg);
    return;

  case TemplateArgument::Type:
    removeOneParameterByArgType(PartialD, Arg);
    return;
  
  default:
    TransAssert(0 && "Uncatched ArgKind!");
  }
  TransAssert(0 && "Unreachable code!");
}

const NamedDecl *ReduceClassTemplateParameter::getNamedDecl(
        const TemplateArgument &Arg)
{
  if (!Arg.isInstantiationDependent())
    return NULL;

  TemplateArgument::ArgKind K = Arg.getKind();
  switch (K) {
  case TemplateArgument::Expression: {
    const Expr *E = Arg.getAsExpr();
    if (const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E)) {
      return dyn_cast<NonTypeTemplateParmDecl>(DRE->getDecl());
    }
    else {
      return NULL;
    }
  }

  case TemplateArgument::Template: {
    TemplateName TmplName = Arg.getAsTemplate();
    TransAssert((TmplName.getKind() == TemplateName::Template) &&
                "Invalid TemplateName Kind!");
    return TmplName.getAsTemplateDecl();
  }

  case TemplateArgument::Type: {
    const Type *Ty = Arg.getAsType().getTypePtr();
    if (const TemplateTypeParmType *TmplTy = 
        dyn_cast<TemplateTypeParmType>(Ty)) {
      return TmplTy->getDecl();
    }
    else {
      return NULL;
    }
  }
  
  default:
    return NULL;
  }

  TransAssert(0 && "Unreachable code!");
  return NULL;
}

bool ReduceClassTemplateParameter::referToAParameter(
       const ClassTemplatePartialSpecializationDecl *PartialD,
       const TemplateArgument &Arg)
{
  const NamedDecl *ArgND = getNamedDecl(Arg);
  if (!ArgND)
    return false;

  const TemplateParameterList *TPList = PartialD->getTemplateParameters();
  for (TemplateParameterList::const_iterator PI = TPList->begin(),
       PE = TPList->end(); PI != PE; ++PI) {
    if (ArgND != (*PI))
      return false;
  }
  return true;
}

bool ReduceClassTemplateParameter::isValidForReduction(
       const ClassTemplatePartialSpecializationDecl *PartialD)
{
  unsigned NumArgsAsWritten = PartialD->getNumTemplateArgsAsWritten();
  unsigned NumArgs = PartialD->getTemplateInstantiationArgs().size();

  if ((NumArgsAsWritten > 0) && 
      (TheParameterIndex >= NumArgsAsWritten) && 
      hasDefaultArg &&
      ((NumArgsAsWritten + 1) == NumArgs))  {

    return true;
  }

  if (NumArgsAsWritten != NumArgs)
    return false;

  TemplateArgumentLoc *ArgLocs = PartialD->getTemplateArgsAsWritten();
  for (unsigned AI = 0; AI < NumArgsAsWritten; ++AI) {
    if (AI == TheParameterIndex)
      continue;
    TemplateArgumentLoc ArgLoc = ArgLocs[AI];
    TemplateArgument Arg = ArgLoc.getArgument();
    if (!referToAParameter(PartialD, Arg))
      return false;
  }

  return true;
}

bool ReduceClassTemplateParameter::reducePartialSpec(
       const ClassTemplatePartialSpecializationDecl *PartialD)
{
  const CXXRecordDecl *CXXRD = TheClassTemplateDecl->getTemplatedDecl();
  // it CXXRD has definition, skip it to avoid duplication
  if (CXXRD->hasDefinition())
    return false;

  if (!isValidForReduction(PartialD))
    return false;

  TemplateArgumentLoc *ArgLocs = PartialD->getTemplateArgsAsWritten();
  unsigned NumArgsAsWritten = PartialD->getNumTemplateArgsAsWritten();
  TemplateArgumentLoc FirstArgLoc = ArgLocs[0];
  SourceRange FirstRange = FirstArgLoc.getSourceRange();
  SourceLocation StartLoc = FirstRange.getBegin();

  TemplateArgumentLoc LastArgLoc = ArgLocs[NumArgsAsWritten - 1];
  SourceRange LastRange = LastArgLoc.getSourceRange();
  SourceLocation EndLoc = 
    RewriteHelper->getEndLocationUntil(LastRange, '>');

  RewriteHelper->removeTextFromLeftAt(SourceRange(StartLoc, EndLoc), '<', EndLoc);
  return true;
}

// ISSUE: The transformation is known to go wrong in the following case:
// template<typename T1, typename T2> struct S;
// template<typename T1, typename T2> struct S<T2, T1>;
void ReduceClassTemplateParameter::removeParameterFromPartialSpecs(void)
{
  SmallVector<ClassTemplatePartialSpecializationDecl *, 10> PartialDecls;
  TheClassTemplateDecl->getPartialSpecializations(PartialDecls);

  for (SmallVector<ClassTemplatePartialSpecializationDecl *, 10>::iterator 
         I = PartialDecls.begin(), E = PartialDecls.end(); I != E; ++I) {
    const ClassTemplatePartialSpecializationDecl *PartialD = (*I);
    TemplateArgumentLoc *ArgLocs = PartialD->getTemplateArgsAsWritten();
    if (!ArgLocs)
      continue;

    // handle a special case where we could reduce a partial specialization
    // to a class template definition, e.g.:
    //   template<typename T1, typename T2> struct A;
    //   template<typename T1> struct A<T1, int> { };
    // ==>
    //   template<typename T1> struct A;
    //   template<typename T1> struct A { };
    if (reducePartialSpec(PartialD))
      continue;

    unsigned NumArgs = PartialD->getNumTemplateArgsAsWritten();
    if ((TheParameterIndex >= NumArgs) && hasDefaultArg)
      return;

    TransAssert((TheParameterIndex < NumArgs) && 
                 "Bad NumArgs from partial template decl!");
    TemplateArgumentLoc ArgLoc = ArgLocs[TheParameterIndex];

    TemplateArgument Arg = ArgLoc.getArgument();
    removeOneParameterFromPartialDecl(PartialD, Arg);
     
    SourceRange Range = ArgLoc.getSourceRange();

    if (NumArgs == 1) {
      SourceLocation StartLoc = Range.getBegin();
      SourceLocation EndLoc = 
        RewriteHelper->getEndLocationUntil(Range, '>');
      EndLoc = EndLoc.getLocWithOffset(-1);
      TheRewriter.RemoveText(SourceRange(StartLoc, EndLoc));
    }
    else if ((TheParameterIndex + 1) == NumArgs) {
      // Seems there is no getRAngleLoc() utility for 
      // template arguments from a partial specialization
      SourceLocation EndLoc = 
        RewriteHelper->getEndLocationUntil(Range, '>');
      EndLoc = EndLoc.getLocWithOffset(-1);
      RewriteHelper->removeTextFromLeftAt(Range, ',', EndLoc);
    }
    else {
      RewriteHelper->removeTextUntil(Range, ',');
    }
  }
}

bool ReduceClassTemplateParameter::isValidClassTemplateDecl(
                                     const ClassTemplateDecl *D)
{
  const TemplateParameterList *TPList = D->getTemplateParameters();
  if (TPList->size() <= 1)
    return false;

  // FIXME: need to handle parameter pack later
  for (TemplateParameterList::const_iterator I = TPList->begin(),
       E = TPList->end(); I != E; ++I) {
    if (isParameterPack(*I))
      return false;
  }
  return true;
}

void ReduceClassTemplateParameter::setDefaultArgFlag(const NamedDecl *ND)
{
  if (const NonTypeTemplateParmDecl *D = 
      dyn_cast<NonTypeTemplateParmDecl>(ND)) {
    hasDefaultArg = D->hasDefaultArgument();
  }
  else if (const TemplateTypeParmDecl *D = 
             dyn_cast<TemplateTypeParmDecl>(ND)) {
    hasDefaultArg = D->hasDefaultArgument();
  }
  else if (const TemplateTemplateParmDecl *D = 
             dyn_cast<TemplateTemplateParmDecl>(ND)) {
    hasDefaultArg = D->hasDefaultArgument();
  }
  else {
    TransAssert(0 && "Unknown template parameter type!");
  }
}

bool ReduceClassTemplateParameter::referToTheTemplateDecl(
                                     TemplateName TmplName)
{
  return Context->hasSameTemplateName(*TheTemplateName, TmplName);
}

ReduceClassTemplateParameter::~ReduceClassTemplateParameter(void)
{
  if (TheTemplateName)
    delete TheTemplateName;
  if (CollectionVisitor)
    delete CollectionVisitor;
  if (ArgRewriteVisitor)
    delete ArgRewriteVisitor;
}

