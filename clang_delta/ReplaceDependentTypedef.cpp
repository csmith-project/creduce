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

#include "ReplaceDependentTypedef.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;
using namespace llvm;

static const char *DescriptionMsg =
"This pass replaces typedef names with the underlying type if the \
underlying type is dependent, e.g.: \n\
\n\
  template <class T> struct S { typedef T type; }; \n\
  struct A { }; \n\
  struct B { typedef S<A>::type type; }; \n\
==> \n\
  template <class T> struct S { typedef T type; }; \n\
  struct A { }; \n\
  struct B { typedef A type; }; \n\
\n\
It also tries to reduce the typedef chain, e.g. \n\
  typedef long xx_t; \n\
  typedef xx_t xx; \n\
==> \n\
  typedef long xx_t; \n\
  typedef long xx; \n\
";

static RegisterTransformation<ReplaceDependentTypedef>
         Trans("replace-dependent-typedef", DescriptionMsg);

class ReplaceDependentTypedefCollectionVisitor : public
  RecursiveASTVisitor<ReplaceDependentTypedefCollectionVisitor> {

public:
  explicit 
  ReplaceDependentTypedefCollectionVisitor(ReplaceDependentTypedef *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitTypedefDecl(TypedefDecl *D);

private:
  ReplaceDependentTypedef *ConsumerInstance;

};

bool 
ReplaceDependentTypedefCollectionVisitor::VisitTypedefDecl(TypedefDecl *D)
{
  ConsumerInstance->handleOneTypedefDecl(D);
  return true;
}

void ReplaceDependentTypedef::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new ReplaceDependentTypedefCollectionVisitor(this);
}

void ReplaceDependentTypedef::HandleTranslationUnit(ASTContext &Ctx)
{
  CollectionVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);
  rewriteTypedefDecl();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

bool ReplaceDependentTypedef::getDependentNameString(
       const Type *Ty,
       const TemplateArgument *Args,
       unsigned NumArgs,
       std::string &Str,
       bool &Typename)
{
  TransAssert((Ty->getTypeClass() == Type::DependentName) &&
              "Not DependentNameType!");

  const DependentNameType *DNT = dyn_cast<DependentNameType>(Ty);
  const IdentifierInfo *IdInfo = DNT->getIdentifier();
  if (!IdInfo)
    return false;
  const NestedNameSpecifier *Specifier = DNT->getQualifier();
  if (!Specifier)
    return false;
  const Type *DependentTy = Specifier->getAsType();
  if (!DependentTy)
    return false;

  const TemplateTypeParmType *ParmTy = 
    DependentTy->getAs<TemplateTypeParmType>();
  if (!ParmTy)
    return false;

  unsigned Idx = ParmTy->getIndex();
  TransAssert((Idx < NumArgs) && "Bad Parm Index!");
  const TemplateArgument Arg = Args[Idx];
  if (Arg.getKind() != TemplateArgument::Type)
    return false;
  QualType ArgQT = Arg.getAsType();
  ArgQT.getAsStringInternal(Str, Context->getPrintingPolicy());
  Str += "::";
  Str += IdInfo->getName();
  Typename = true;
  return true;
}

bool ReplaceDependentTypedef::getTemplateTypeParmString(
       const TemplateTypeParmType *ParmTy,
       const TemplateArgument *Args,
       unsigned NumArgs,
       std::string &Str)
{
  unsigned Idx = ParmTy->getIndex();
  // we could have default template args, skip this case for now
  if (Idx >= NumArgs)
    return false;
  const TemplateArgument Arg = Args[Idx];
  if (Arg.getKind() != TemplateArgument::Type)
    return false;
  QualType ArgQT = Arg.getAsType();
  ArgQT.getAsStringInternal(Str, Context->getPrintingPolicy());
  return true;
}

bool ReplaceDependentTypedef::getTypedefString(const StringRef &Name,
                                               const CXXRecordDecl *CXXRD,
                                               const TemplateArgument *Args,
                                               unsigned NumArgs,
                                               std::string &Str,
                                               bool &Typename)
{
  Str = "";
  // really simplified lookup process, maybe need to check parent decl context? 
  for (DeclContext::decl_iterator I = CXXRD->decls_begin(),
       E = CXXRD->decls_end(); I != E; ++I) {
    const TypedefDecl *D = dyn_cast<TypedefDecl>(*I);
    if (!D || (D->getNameAsString() != Name))
      continue;
    const Type *UnderlyingTy = D->getUnderlyingType().getTypePtr();
    Type::TypeClass TC = UnderlyingTy->getTypeClass();
    if (TC == Type::DependentName) {
      if (getDependentNameString(UnderlyingTy, Args, NumArgs, Str, Typename))
        return true;
    }
    else if (const TemplateTypeParmType *ParmTy = 
             UnderlyingTy->getAs<TemplateTypeParmType>()) {
      if (getTemplateTypeParmString(ParmTy, Args, NumArgs, Str))
        return true;
    }
  }
  return false;
}

bool ReplaceDependentTypedef::getTypeString(const QualType &QT, 
                                            std::string &Str,
                                            bool &Typename)
{
  const Type *Ty = QT.getTypePtr();
  Type::TypeClass TC = Ty->getTypeClass();

  switch (TC) {
  case Type::SubstTemplateTypeParm: {
    const SubstTemplateTypeParmType *TP = 
      dyn_cast<SubstTemplateTypeParmType>(Ty);
    return getTypeString(TP->getReplacementType(), Str, Typename);
  }

  case Type::Elaborated: {
    const ElaboratedType *ETy = dyn_cast<ElaboratedType>(Ty);
    return getTypeString(ETy->getNamedType(), Str, Typename);
  }
  
  case Type::Typedef: {
    const TypedefType *TdefTy = dyn_cast<TypedefType>(Ty);
    const TypedefNameDecl *TdefD = TdefTy->getDecl();
    return getTypeString(TdefD->getUnderlyingType(), Str, Typename);
  }

  case Type::DependentName: {
    const DependentNameType *DNT = dyn_cast<DependentNameType>(Ty);
    const IdentifierInfo *IdInfo = DNT->getIdentifier();
    if (!IdInfo)
      return false;
    const NestedNameSpecifier *Specifier = DNT->getQualifier();
    if (!Specifier)
      return false;
    const Type *Ty = Specifier->getAsType();
    if (!Ty)
      return false;
    const CXXRecordDecl *CXXRD = getBaseDeclFromType(Ty);
    if (!CXXRD)
      return false;

    unsigned NumArgs = 0;
    const TemplateArgument *Args = NULL;
    if (const TemplateSpecializationType *TST = 
        Ty->getAs<TemplateSpecializationType>()) {
      NumArgs = TST->getNumArgs();
      Args = TST->getArgs();
    }
    return 
      getTypedefString(IdInfo->getName(), CXXRD, Args, NumArgs, Str, Typename);
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

bool ReplaceDependentTypedef::isValidType(const QualType &QT)
{
  const Type *Ty = QT.getTypePtr();
  Type::TypeClass TC = Ty->getTypeClass();

  switch (TC) {
  case Type::SubstTemplateTypeParm:
  case Type::Typedef:
  case Type::DependentName: // fall-through
    return true;

  case Type::Elaborated: {
    const ElaboratedType *ETy = dyn_cast<ElaboratedType>(Ty);
    ElaboratedTypeKeyword Keyword = ETy->getKeyword();
    return ((Keyword == ETK_Typename) || (Keyword == ETK_None));
  }
  
  default:
    return false;
  }

  TransAssert(0 && "Unreachable code!");
  return false;
}

void ReplaceDependentTypedef::handleOneTypedefDecl(const TypedefDecl *D)
{
  if (D->getLocStart().isInvalid())
    return;

  if (!isValidType(D->getUnderlyingType()))
    return;

  std::string Str = "";
  bool Typename = false;
  if (!getTypeString(D->getUnderlyingType(), Str, Typename))
    return;
  std::string TdefTyStr = "";
  D->getUnderlyingType().getAsStringInternal(
    TdefTyStr, Context->getPrintingPolicy());
  if (Str == TdefTyStr)
    return;

  ValidInstanceNum++;
  if (ValidInstanceNum == TransformationCounter) {
    TheTypedefDecl = D;
    TheTyName = Str;
    NeedTypenameKeyword = Typename;
  }
}

void ReplaceDependentTypedef::rewriteTypedefDecl()
{
  std::string NewStr = "typedef ";
  if (NeedTypenameKeyword)
    NewStr += "typename ";
  NewStr = NewStr + TheTyName + " " + TheTypedefDecl->getNameAsString();
  SourceRange Range = TheTypedefDecl->getSourceRange();
  TheRewriter.ReplaceText(Range, NewStr);
}

ReplaceDependentTypedef::~ReplaceDependentTypedef()
{
  delete CollectionVisitor;
}

