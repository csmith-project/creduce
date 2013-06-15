//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "InstantiateTemplateParam.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;
using namespace llvm;

// TODO: probably need to handle more cases where this transformation
// generates invalid code, e.g.:
// template <typename T> struct S {
//   typedef T type;
//   type &foo();
// };
// struct A {
//   void foo() {};
// };
// void bar() {
//   struct S<A> s;
//   struct A a = s.foo();
// }

static const char *DescriptionMsg = 
"This pass tries to instantiate a template type parameter of a class \
inside the class's definition. For example, \n\
  template <typename T> struct A { T foo(void); } \n\
=> \n\
  template <typename T> struct A { int foo(void); } \n\
\n\
Currently, just simply replace any reference of T with int.\n";

static RegisterTransformation<InstantiateTemplateParam>
         Trans("instantiate-template-param", DescriptionMsg);

namespace {

typedef llvm::SmallPtrSet<const NamedDecl *, 8> TemplateParameterSet;

class TemplateParameterVisitor : public 
  RecursiveASTVisitor<TemplateParameterVisitor> {

public:
  explicit TemplateParameterVisitor(TemplateParameterSet &Params)
             : UsedParameters(Params) 
  { }

  ~TemplateParameterVisitor() { };

  bool VisitTemplateTypeParmType(TemplateTypeParmType *Ty);

  bool isUsedParam(const NamedDecl *ND) {
    return UsedParameters.count(ND);
  }

private:

  TemplateParameterSet &UsedParameters;
};

bool TemplateParameterVisitor::VisitTemplateTypeParmType(
       TemplateTypeParmType *Ty)
{
  const TemplateTypeParmDecl *D = Ty->getDecl();
  UsedParameters.insert(D);
  return true;
}

} // end anonymous namespace

// In order to generate less uncompilable code, filter out cases such as
// template <typename T> struct S {
//   T::type foo(void);
// };
class TemplateParameterFilterVisitor : public 
  RecursiveASTVisitor<TemplateParameterFilterVisitor> {

public:
  TemplateParameterFilterVisitor(TemplateParameterSet &Params,
                                 InstantiateTemplateParam *Instance)
    : Parameters(Params),
      ConsumerInstance(Instance)
  { }

  ~TemplateParameterFilterVisitor() { };

  bool VisitTemplateTypeParmTypeLoc(TemplateTypeParmTypeLoc Loc);

private:
  TemplateParameterSet &Parameters;

  InstantiateTemplateParam *ConsumerInstance;
};

bool TemplateParameterFilterVisitor::VisitTemplateTypeParmTypeLoc(
       TemplateTypeParmTypeLoc Loc)
{
  const NamedDecl *ND = Loc.getDecl();
  if (!Parameters.count(ND))
    return true;

  if (ConsumerInstance->isBeforeColonColon(Loc))
    Parameters.erase(ND);

  return true;
}

// ISSUE: maybe also try to instantiate a type parm with
// its default value, if it exists?
class InstantiateTemplateParamASTVisitor : public 
  RecursiveASTVisitor<InstantiateTemplateParamASTVisitor> {

public:
  explicit InstantiateTemplateParamASTVisitor(
             InstantiateTemplateParam *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitClassTemplateDecl(ClassTemplateDecl *D);

  bool VisitFunctionTemplateDecl(FunctionTemplateDecl *D);

private:
  InstantiateTemplateParam *ConsumerInstance;

};

class InstantiateTemplateParamRewriteVisitor : public 
  RecursiveASTVisitor<InstantiateTemplateParamRewriteVisitor> {

public:
  explicit InstantiateTemplateParamRewriteVisitor(
             InstantiateTemplateParam *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitTemplateTypeParmTypeLoc(TemplateTypeParmTypeLoc Loc);

private:
  InstantiateTemplateParam *ConsumerInstance;

};

bool InstantiateTemplateParamASTVisitor::VisitClassTemplateDecl(
       ClassTemplateDecl *D)
{
  // only care about declarations
  if (D->isThisDeclarationADefinition())
    ConsumerInstance->handleOneTemplateDecl(D);
  return true;
}

bool InstantiateTemplateParamASTVisitor::VisitFunctionTemplateDecl(
       FunctionTemplateDecl *D)
{
  // only care about declarations
  if (D->isThisDeclarationADefinition())
    ConsumerInstance->handleOneTemplateDecl(D);
  return true;
}

bool InstantiateTemplateParamRewriteVisitor::VisitTemplateTypeParmTypeLoc(
       TemplateTypeParmTypeLoc Loc)
{
  const TemplateTypeParmDecl *D = Loc.getDecl();
  if (D != ConsumerInstance->TheParameter)
    return true;

  SourceRange Range = Loc.getSourceRange();
  ConsumerInstance->TheRewriter.ReplaceText(Range, "int");
  return true;
}

void InstantiateTemplateParam::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new InstantiateTemplateParamASTVisitor(this);
  ParamRewriteVisitor = new InstantiateTemplateParamRewriteVisitor(this);
}

void InstantiateTemplateParam::HandleTranslationUnit(ASTContext &Ctx)
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

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  TransAssert(TheParameter && "NULL TheParameter!");
  TransAssert(ParamRewriteVisitor && "NULL ParamRewriteVisitor!");
  ParamRewriteVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void InstantiateTemplateParam::handleOneTemplateDecl(const TemplateDecl *D)
{
  // doesn't handle TypeAliasTemplateDecl
  TransAssert((!dyn_cast<TypeAliasTemplateDecl>(D)) && 
              "Doesn't support TypeAliasTemplateDecl!");
  const TemplateDecl *CanonicalD = 
    dyn_cast<TemplateDecl>(D->getCanonicalDecl());
  TransAssert(CanonicalD && "Invalid TemplateDecl!");
  TransAssert((VisitedTemplateDecls.count(CanonicalD) == 0) &&
              "Duplicate visitation to TemplateDecl!");
  VisitedTemplateDecls.insert(CanonicalD);

  NamedDecl *ND = D->getTemplatedDecl();
  TemplateParameterSet ParamsSet;
  TemplateParameterVisitor ParameterVisitor(ParamsSet);
  ParameterVisitor.TraverseDecl(ND);
  filterInvalidParams(D, ParamsSet);

  if (ParamsSet.size() == 0)
    return;

  TemplateParameterList *TPList = D->getTemplateParameters();
  for (TemplateParameterList::const_iterator I = TPList->begin(),
       E = TPList->end(); I != E; ++I) {
    const NamedDecl *ND = (*I);
    if (!ParameterVisitor.isUsedParam(ND))
      continue;
    ValidInstanceNum++;
    if (ValidInstanceNum == TransformationCounter)
      TheParameter = ND;
  }
}

void InstantiateTemplateParam::filterInvalidParams(const TemplateDecl *D,
                                                   TemplateParameterSet &Params)
{
  NamedDecl *ND = D->getTemplatedDecl();
  TemplateParameterFilterVisitor Filter(Params, this);
  Filter.TraverseDecl(ND);

  const ClassTemplateDecl *CD = dyn_cast<ClassTemplateDecl>(D);
  if (!CD)
    return;
  CXXRecordDecl *Def = CD->getTemplatedDecl()->getDefinition();
  TransAssert(Def && "No Definition?");
  if (!Def->isCompleteDefinition())
    return;
  for (CXXRecordDecl::base_class_const_iterator I = Def->bases_begin(),
       E = Def->bases_end(); I != E; ++I) {
    const CXXBaseSpecifier *BS = I;
    const Type *Ty = BS->getType().getTypePtr();
    const TemplateTypeParmType *ParmTy = dyn_cast<TemplateTypeParmType>(Ty);
    if (!ParmTy)
      continue;
    const TemplateTypeParmDecl *ParmD = ParmTy->getDecl();
    Params.erase(ParmD);
  }
}

InstantiateTemplateParam::~InstantiateTemplateParam()
{
  delete CollectionVisitor;
  delete ParamRewriteVisitor;
}

