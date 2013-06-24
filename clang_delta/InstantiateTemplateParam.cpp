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

static const char *DescriptionMsg = 
"This pass tries to instantiate a template parameter with  \
its actual argument if this parameter has been instantiated \n\
only once. \n";

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

bool InstantiateTemplateParamASTVisitor::VisitClassTemplateDecl(
       ClassTemplateDecl *D)
{
  if (D->isThisDeclarationADefinition())
    ConsumerInstance->handleOneClassTemplateDecl(D);
  return true;
}

bool InstantiateTemplateParamASTVisitor::VisitFunctionTemplateDecl(
       FunctionTemplateDecl *D)
{
  if (D->isThisDeclarationADefinition())
    ConsumerInstance->handleOneFunctionTemplateDecl(D);
  return true;
}

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

bool 
InstantiateTemplateParamRewriteVisitor::VisitTemplateTypeParmTypeLoc(
       TemplateTypeParmTypeLoc Loc)
{
  const TemplateTypeParmDecl *D = Loc.getDecl();
  if (D != ConsumerInstance->TheParameter)
    return true;

  SourceRange Range = Loc.getSourceRange();
  ConsumerInstance->TheRewriter.ReplaceText(Range, 
                       ConsumerInstance->TheInstantiationString);
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
  TransAssert((TheInstantiationString != "") && "Invalid InstantiationString!");
  TransAssert(ParamRewriteVisitor && "NULL ParamRewriteVisitor!");
  ParamRewriteVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

bool InstantiateTemplateParam::getTypeString(
       const QualType &QT, std::string &Str)
{
  const Type *Ty = QT.getTypePtr();
  Type::TypeClass TC = Ty->getTypeClass();

  switch (TC) {
  case Type::Elaborated: {
    const ElaboratedType *ETy = dyn_cast<ElaboratedType>(Ty);
    return getTypeString(ETy->getNamedType(), Str);
  }

  case Type::Typedef: {
    const TypedefType *TdefTy = dyn_cast<TypedefType>(Ty);
    const TypedefNameDecl *TdefD = TdefTy->getDecl();
    return getTypeString(TdefD->getUnderlyingType(), Str);
  }

  case Type::Record:
  case Type::Builtin: { // fall-through
    QT.getAsStringInternal(Str, Context->getPrintingPolicy());
    return true;
  }

  default:
    return false;
  }

  TransAssert(0 && "Unreachable code!");
  return false;
}

bool InstantiateTemplateParam::getTemplateArgumentString(
       const TemplateArgument &Arg, std::string &ArgStr)
{
  ArgStr = "";
  if (Arg.getKind() != TemplateArgument::Type)
    return false;
  QualType QT = Arg.getAsType();
  return getTypeString(QT, ArgStr);
}

void InstantiateTemplateParam::handleOneTemplateSpecialization(
       const TemplateDecl *D, const TemplateArgumentList & ArgList)
{
  NamedDecl *ND = D->getTemplatedDecl();
  TemplateParameterSet ParamsSet;
  TemplateParameterVisitor ParameterVisitor(ParamsSet);
  ParameterVisitor.TraverseDecl(ND);

  if (ParamsSet.size() == 0)
    return;

  unsigned NumArgs = ArgList.size(); (void)NumArgs;
  unsigned Idx = 0;
  TemplateParameterList *TPList = D->getTemplateParameters();
  for (TemplateParameterList::const_iterator I = TPList->begin(),
       E = TPList->end(); I != E; ++I) {
    const NamedDecl *ND = (*I);
    // make it simple, skip NonTypeTemplateParmDecl and 
    // TemplateTemplateParmDecl for now
    const TemplateTypeParmDecl *TyParmDecl = 
      dyn_cast<TemplateTypeParmDecl>(ND);
    if (!TyParmDecl || TyParmDecl->isParameterPack() || !ParamsSet.count(ND)) {
      Idx++;
      continue;
    }

    TransAssert((Idx < NumArgs) && "Invalid Idx!");
    const TemplateArgument &Arg = ArgList.get(Idx);
    std::string ArgStr;
    if (!getTemplateArgumentString(Arg, ArgStr))
      continue;
    ValidInstanceNum++;
    if (ValidInstanceNum == TransformationCounter) {
      TheInstantiationString = ArgStr;
      TheParameter = ND;
    }
  }
}
       
// TODO: handle partial specialization
void InstantiateTemplateParam::handleOneClassTemplateDecl(
       const ClassTemplateDecl *D)
{
  ClassTemplateDecl::spec_iterator I = D->spec_begin();
  ClassTemplateDecl::spec_iterator E = D->spec_end();
  if (I == E)
    return;
  ClassTemplateSpecializationDecl *SpecD = (*I);
  ++I;
  if (I != D->spec_end())
    return;
  handleOneTemplateSpecialization(D, SpecD->getTemplateArgs());
}

void InstantiateTemplateParam::handleOneFunctionTemplateDecl(
       const FunctionTemplateDecl *D)
{
  FunctionTemplateDecl::spec_iterator I = D->spec_begin();
  FunctionTemplateDecl::spec_iterator E = D->spec_end();
  if (I == E)
    return;
  const FunctionDecl *FD = (*I);
  ++I;
  if (I != D->spec_end())
    return;
  if (const FunctionTemplateSpecializationInfo *Info =
      FD->getTemplateSpecializationInfo()) {
    handleOneTemplateSpecialization(D, *(Info->TemplateArguments));
  }
}

InstantiateTemplateParam::~InstantiateTemplateParam()
{
  delete CollectionVisitor;
  delete ParamRewriteVisitor;
}

