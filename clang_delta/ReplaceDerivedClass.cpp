//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2015, 2017 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "ReplaceDerivedClass.h"

#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"
#include "CommonRenameClassRewriteVisitor.h"
#include "TransformationManager.h"

using namespace clang;
using namespace clang_delta_common_visitor;

static const char *DescriptionMsg =
"Replace a derived class with one of its base classes if \n\
  * the derived class is empty; \n\
  * and if the base class and the derived class are both class templates, \
they require the same number of arguments for instantiation. \n";

static RegisterTransformation<ReplaceDerivedClass>
         Trans("replace-derived-class", DescriptionMsg);

class ReplaceDerivedClassASTVisitor : public 
  RecursiveASTVisitor<ReplaceDerivedClassASTVisitor> {

public:
  explicit ReplaceDerivedClassASTVisitor(ReplaceDerivedClass *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitCXXRecordDecl(CXXRecordDecl *CXXRD);

private:
  ReplaceDerivedClass *ConsumerInstance;
};

class ReplaceDerivedClassRewriteVisitor : public 
  CommonRenameClassRewriteVisitor<ReplaceDerivedClassRewriteVisitor> 
{
public:
  ReplaceDerivedClassRewriteVisitor(Rewriter *RT, 
                                    RewriteUtils *Helper,
                                    const CXXRecordDecl *CXXRD,
                                    const std::string &Name)
    : CommonRenameClassRewriteVisitor<ReplaceDerivedClassRewriteVisitor>
      (RT, Helper, CXXRD, Name)
  { }
};

bool ReplaceDerivedClassASTVisitor::VisitCXXRecordDecl(CXXRecordDecl *CXXRD)
{
  ConsumerInstance->handleOneCXXRecordDecl(CXXRD);
  return true;
}

void ReplaceDerivedClass::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new ReplaceDerivedClassASTVisitor(this);
}

void ReplaceDerivedClass::HandleTranslationUnit(ASTContext &Ctx)
{
  if (TransformationManager::isCLangOpt() ||
      TransformationManager::isOpenCLLangOpt()) {
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

  TransAssert(TheDerivedClass && "TheDerivedClass is NULL!");
  TransAssert(TheBaseClass && "TheBaseClass is NULL!");
  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  RewriteVisitor = 
    new ReplaceDerivedClassRewriteVisitor(&TheRewriter, RewriteHelper, 
                                          TheDerivedClass->getCanonicalDecl(),
                                          TheBaseClass->getNameAsString());
  TransAssert(RewriteVisitor && "NULL RewriteVisitor!");
  RewriteVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
  doRewrite();
 
  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

bool ReplaceDerivedClass::isValidBaseDerivedPair(const CXXRecordDecl *Base,
                                                 const CXXRecordDecl *Derived)
{
  const ClassTemplateDecl *BaseTmplD = Base->getDescribedClassTemplate();
  const ClassTemplateDecl *DerivedTmplD = Derived->getDescribedClassTemplate();

  if (!BaseTmplD && !DerivedTmplD) {
    return true;
  }
  else if (BaseTmplD && DerivedTmplD) {
     return (BaseTmplD->getTemplateParameters()->getMinRequiredArguments() ==
             DerivedTmplD->getTemplateParameters()->getMinRequiredArguments());
  }

  return false;
}

bool ReplaceDerivedClass::isEmptyClass(const CXXRecordDecl *CXXDef)
{
  TransAssert(CXXDef->isThisDeclarationADefinition() && 
              "CXXDef must be a definition!");
  const DeclContext *Ctx = dyn_cast<DeclContext>(CXXDef);
  TransAssert(Ctx && "Invalid DeclContext!");
  for (DeclContext::decl_iterator I = Ctx->decls_begin(),
       E = Ctx->decls_end(); I != E; ++I) {
    if (!(*I)->isImplicit())
      return false;
  }
  return true;
}

void ReplaceDerivedClass::handleOneCXXRecordDecl(const CXXRecordDecl *CXXRD)
{
  if (isInIncludedFile(CXXRD))
    return;
  const CXXRecordDecl *CXXDef = CXXRD->getDefinition();
  if (!CXXDef)
    return;

  if (dyn_cast<ClassTemplateSpecializationDecl>(CXXRD))
    return;
  if ((CXXDef->getNumBases() == 0) || !isEmptyClass(CXXDef))
    return;

  const CXXRecordDecl *CanonicalRD = CXXRD->getCanonicalDecl();
  if (VisitedCXXRecordDecls.count(CanonicalRD))
    return;

  VisitedCXXRecordDecls.insert(CanonicalRD);
  for (CXXRecordDecl::base_class_const_iterator I = 
       CanonicalRD->bases_begin(), E = CanonicalRD->bases_end(); I != E; ++I) {
    const CXXBaseSpecifier *BS = I;
    const Type *Ty = BS->getType().getTypePtr();
    const CXXRecordDecl *Base = getBaseDeclFromType(Ty);
    if (!Base || !isValidBaseDerivedPair(Base, CanonicalRD))
      continue;

    ValidInstanceNum++;
    if (ValidInstanceNum == TransformationCounter) {
      TheBaseClass = Base;
      TheDerivedClass = CanonicalRD;
    }
  }
}

void ReplaceDerivedClass::doRewrite(void)
{
  if (const ClassTemplateDecl *TmplD = 
      TheDerivedClass->getDescribedClassTemplate()) {
    RewriteHelper->removeClassTemplateDecls(TmplD);
  }
  else {
    RewriteHelper->removeClassDecls(TheDerivedClass);
  }
}

ReplaceDerivedClass::~ReplaceDerivedClass(void)
{
  delete CollectionVisitor;
  delete RewriteVisitor;
}

