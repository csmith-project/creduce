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

#include "TemplateArgToInt.h"

#include "clang/Lex/Lexer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"
#include "CommonTemplateArgumentVisitor.h"

using namespace clang;
using namespace clang_delta_common_visitor;

static const char *DescriptionMsg = 
"This pass replaces a template argument in an instantiation with \
int if the argument: \n\
   * is type of CXXRecord; \n\
   * the corresponding template parameter T is not used as T::x, \n\
nor template<typename T> class : T { ... };\n\
For example, from:\n\
   struct S {};\n\
   template <typename T> struct C {};\n\
   C<S> c;\n\
to:\n\
   struct S {};\n\
   template <typename T> struct C {};\n\
   C<int> c;\n\
";

static RegisterTransformation<TemplateArgToInt>
         Trans("template-arg-to-int", DescriptionMsg);

class TemplateArgToIntASTVisitor : public 
  RecursiveASTVisitor<TemplateArgToIntASTVisitor> {

public:
  explicit TemplateArgToIntASTVisitor(
             TemplateArgToInt *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitClassTemplateDecl(ClassTemplateDecl *D);

  bool VisitFunctionTemplateDecl(FunctionTemplateDecl *D);

private:
  TemplateArgToInt *ConsumerInstance;

};

class TemplateArgToIntArgCollector : public 
  CommonTemplateArgumentVisitor<TemplateArgToIntArgCollector, 
                                TemplateArgToInt> {

public:
  explicit TemplateArgToIntArgCollector(TemplateArgToInt *Instance)
    : CommonTemplateArgumentVisitor<TemplateArgToIntArgCollector,
                                    TemplateArgToInt>(Instance)
  { }
};

typedef llvm::SmallPtrSet<const NamedDecl *, 8> TemplateParameterSet;

// rule out cases like:
//template <typename T1> struct S { typedef T1 type; };
// struct S1 { void foo() {} };
// struct S2 {
//   typedef S<S1>::type type;
//   S<S1>::type s1;
//   type s2;
//   void bar() { s1.foo(); s2.foo(); }
// };
class TemplateGlobalInvalidParameterVisitor : public 
  RecursiveASTVisitor<TemplateGlobalInvalidParameterVisitor> {

public:
  explicit TemplateGlobalInvalidParameterVisitor(
             TemplateArgToInt *Instance)
    : ConsumerInstance(Instance)
  { }

  ~TemplateGlobalInvalidParameterVisitor() { };

  bool VisitMemberExpr(MemberExpr *ME);

  bool VisitCXXRecordDecl(CXXRecordDecl *D);

private:
  TemplateArgToInt *ConsumerInstance;
};

bool TemplateGlobalInvalidParameterVisitor::VisitMemberExpr(MemberExpr *ME)
{
  const Expr *Base = ME->getBase();
  if (dyn_cast<CXXThisExpr>(Base))
    return true;

  const Type *Ty = Base->getType().getTypePtr();
  ConsumerInstance->handleOneType(Ty);
  return true;
}

bool TemplateGlobalInvalidParameterVisitor::VisitCXXRecordDecl(CXXRecordDecl *D)
{
  if (!D->isCompleteDefinition())
    return true;
  for (CXXRecordDecl::base_class_const_iterator I = D->bases_begin(),
       E = D->bases_end(); I != E; ++I) {
    const CXXBaseSpecifier *BS = I;
    const Type *Ty = BS->getType().getTypePtr();
    ConsumerInstance->handleOneType(Ty);
  }
  return true;
}

class TemplateInvalidParameterVisitor : public 
  RecursiveASTVisitor<TemplateInvalidParameterVisitor> {

public:
  TemplateInvalidParameterVisitor(TemplateParameterSet &Params,
                                  TemplateArgToInt *Instance)
    : Parameters(Params),
      ConsumerInstance(Instance)
  { }

  ~TemplateInvalidParameterVisitor() { };

  bool VisitTemplateTypeParmTypeLoc(TemplateTypeParmTypeLoc Loc);

  bool VisitCXXRecordDecl(CXXRecordDecl *D);

private:
  TemplateParameterSet &Parameters;

  TemplateArgToInt *ConsumerInstance;
};

bool TemplateInvalidParameterVisitor::VisitTemplateTypeParmTypeLoc(
       TemplateTypeParmTypeLoc Loc)
{
  const NamedDecl *ND = Loc.getDecl();
  if (ConsumerInstance->isBeforeColonColon(Loc))
    Parameters.insert(ND);

  return true;
}

bool TemplateInvalidParameterVisitor::VisitCXXRecordDecl(CXXRecordDecl *D)
{
  if (!D->isCompleteDefinition())
    return true;
  for (CXXRecordDecl::base_class_const_iterator I = D->bases_begin(),
       E = D->bases_end(); I != E; ++I) {
    const CXXBaseSpecifier *BS = I;
    const Type *Ty = BS->getType().getTypePtr();
    const TemplateTypeParmType *ParmTy = dyn_cast<TemplateTypeParmType>(Ty);
    if (!ParmTy)
      continue;
    const TemplateTypeParmDecl *ParmD = ParmTy->getDecl();
    Parameters.insert(ParmD);
  }
  return true;
}

bool TemplateArgToIntASTVisitor::VisitClassTemplateDecl(
       ClassTemplateDecl *D)
{
  if (D->isThisDeclarationADefinition())
    ConsumerInstance->handleOneTemplateDecl(D);
  return true;
}

bool TemplateArgToIntASTVisitor::VisitFunctionTemplateDecl(
       FunctionTemplateDecl *D)
{
  if (D->isThisDeclarationADefinition())
    ConsumerInstance->handleOneTemplateDecl(D);
  return true;
}

void TemplateArgToInt::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new TemplateArgToIntASTVisitor(this);
  ArgCollector = new TemplateArgToIntArgCollector(this);
  GlobalParamFilter = new TemplateGlobalInvalidParameterVisitor(this);
}

void TemplateArgToInt::HandleTranslationUnit(ASTContext &Ctx)
{
  if (TransformationManager::isCLangOpt() ||
      TransformationManager::isOpenCLLangOpt()) {
    ValidInstanceNum = 0;
  }
  else {
    CollectionVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
    GlobalParamFilter->TraverseDecl(Ctx.getTranslationUnitDecl());
    ArgCollector->TraverseDecl(Ctx.getTranslationUnitDecl());
  }

  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);
  rewriteTemplateArgument();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void TemplateArgToInt::collectInvalidParamIdx(
       const TemplateDecl *D,
       TemplateParameterIdxSet &InvalidParamIdx)
{
  TemplateParameterSet InvalidParams;
  NamedDecl *ND = D->getTemplatedDecl();
  TemplateInvalidParameterVisitor ParameterVisitor(InvalidParams, this);
  ParameterVisitor.TraverseDecl(ND);

  TemplateParameterList *TPList = D->getTemplateParameters();
  unsigned Idx = 0;
  for (TemplateParameterList::const_iterator I = TPList->begin(),
       E = TPList->end(); I != E; ++I) {
    const NamedDecl *ParamND = (*I);
    ParamToTemplateDecl[ParamND] = D;
    if (InvalidParams.count(ParamND)) {
      TransAssert(!InvalidParamIdx.count(Idx) && "Duplicate Index!");
      InvalidParamIdx.insert(Idx);
    }
    Idx++;
  }
}

void TemplateArgToInt::handleOneTemplateDecl(const TemplateDecl *D)
{
  TemplateParameterIdxSet *InvalidIdx = new TemplateParameterIdxSet();
  collectInvalidParamIdx(D, *InvalidIdx);
  TransAssert(!DeclToParamIdx[D] && "Duplicate TemplateDecl!");
  DeclToParamIdx[dyn_cast<TemplateDecl>(D->getCanonicalDecl())] = InvalidIdx;
}

void TemplateArgToInt::handleOneTemplateArgumentLoc(
       const TemplateArgumentLoc &ArgLoc)
{
  if (ArgLoc.getLocation().isInvalid() ||
      isInIncludedFile(ArgLoc.getLocation()))
    return;
  const TemplateArgument &Arg = ArgLoc.getArgument();

  if (Arg.getKind() != TemplateArgument::Type)
    return;
  const Type *Ty = Arg.getAsType().getTypePtr();
  if (!Ty->getAsCXXRecordDecl() && !Ty->getPointeeCXXRecordDecl())
    return;

  ValidInstanceNum++;
  if (ValidInstanceNum == TransformationCounter)
    TheTypeSourceInfo = ArgLoc.getTypeSourceInfo();
}

void TemplateArgToInt::handleTemplateArgumentLocs(
       const TemplateDecl *D, const TemplateArgumentLoc *TAL, unsigned NumArgs)
{
  TransAssert(D && "NULL TemplateDecl!");
  if (!TAL)
    return;
  TemplateParameterIdxSet *InvalidIdx = 
    DeclToParamIdx[dyn_cast<TemplateDecl>(D->getCanonicalDecl())];
  if (!InvalidIdx)
    return;
  for (unsigned I = 0; I < NumArgs; ++I) {
    if (!InvalidIdx->count(I))
      handleOneTemplateArgumentLoc(TAL[I]);
  }
}

void TemplateArgToInt::handleTemplateSpecializationTypeLoc(
       const TemplateSpecializationTypeLoc &TLoc)
{
  const Type *Ty = TLoc.getTypePtr();
  const TemplateSpecializationType *TST = 
    Ty->getAs<TemplateSpecializationType>();
  TemplateName TplName = TST->getTemplateName();
  const TemplateDecl *TplD = TplName.getAsTemplateDecl();

  TemplateParameterIdxSet *InvalidIdx = 
    DeclToParamIdx[dyn_cast<TemplateDecl>(TplD->getCanonicalDecl())];
  if (!InvalidIdx)
    return;
  for (unsigned I = 0; I < TLoc.getNumArgs(); ++I) {
    if (!InvalidIdx->count(I))
      handleOneTemplateArgumentLoc(TLoc.getArgLoc(I));
  }
}

void TemplateArgToInt::rewriteTemplateArgument()
{
  TransAssert(TheTypeSourceInfo && "NULL TheTypeSourceInfo");
  SourceRange Range = TheTypeSourceInfo->getTypeLoc().getSourceRange();
  TheRewriter.ReplaceText(Range, "int");
}

const SubstTemplateTypeParmType *
TemplateArgToInt::getSubstTemplateTypeParmType(const Type *Ty)
{
  Type::TypeClass TC = Ty->getTypeClass();
  switch (TC) {
  case Type::Elaborated: {
    const ElaboratedType *ETy = dyn_cast<ElaboratedType>(Ty);
    const Type *NamedT = ETy->getNamedType().getTypePtr();
    return getSubstTemplateTypeParmType(NamedT);
  }

  case Type::Typedef: {
    const TypedefType *TdefTy = dyn_cast<TypedefType>(Ty);
    const TypedefNameDecl *TdefD = TdefTy->getDecl();
    const Type *UnderlyingTy = TdefD->getUnderlyingType().getTypePtr();
    return getSubstTemplateTypeParmType(UnderlyingTy);
  }

  case Type::SubstTemplateTypeParm: {
    return dyn_cast<SubstTemplateTypeParmType>(Ty);
  }

  default:
    return NULL;
  }

  return NULL;
}

void TemplateArgToInt::handleOneType(const Type *Ty)
{
  if (Ty->isPointerType() || Ty->isReferenceType())
    Ty = getBasePointerElemType(Ty);
  const SubstTemplateTypeParmType *SubstType = getSubstTemplateTypeParmType(Ty);
  if (!SubstType)
    return;

  const TemplateTypeParmType *ParmType = SubstType->getReplacedParameter();
  TemplateTypeParmDecl *ParmDecl = ParmType->getDecl();
  TransAssert(ParmDecl && "Invalid ParmDecl!");
  const TemplateDecl *TmplD = ParamToTemplateDecl[ParmDecl];
  if (TmplD == NULL) {
    const DeclContext *Ctx = ParmDecl->getDeclContext();
    TransAssert(Ctx && "NULL Ctx!");
    const ClassTemplateSpecializationDecl *Spec = 
      dyn_cast<ClassTemplateSpecializationDecl>(Ctx);
    TransAssert(Spec && "Not a ClassTemplateSpecializationDecl!");
    TmplD = Spec->getSpecializedTemplate();
  }
  TransAssert(TmplD && "NULL TemplateDecl!");
  TemplateParameterIdxSet *InvalidIdx = 
    DeclToParamIdx[dyn_cast<TemplateDecl>(TmplD->getCanonicalDecl())];
  TransAssert(InvalidIdx && "NULL InvalidIdx!");
  InvalidIdx->insert(ParmType->getIndex());
}

TemplateArgToInt::~TemplateArgToInt()
{
  for (TemplateDeclToParamIdxMap::iterator I = DeclToParamIdx.begin(),
       E = DeclToParamIdx.end(); I != E; ++I) {
    delete (*I).second;
  }
  delete CollectionVisitor;
  delete ArgCollector;
  delete GlobalParamFilter;
}

