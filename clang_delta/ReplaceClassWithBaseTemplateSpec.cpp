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

#include "ReplaceClassWithBaseTemplateSpec.h"

#include "clang/Basic/SourceManager.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg = 
"This pass tries to replace a class with its base class if \n\
  * this class has only one base class, and \n\
  * this class doesn't have any explicit declaration, and \n\
  * the base class is a class template specialization \n\
";

static RegisterTransformation<ReplaceClassWithBaseTemplateSpec>
         Trans("replace-class-with-base-template-spec", DescriptionMsg);

class ReplaceClassWithBaseTemplateSpecVisitor : public 
  RecursiveASTVisitor<ReplaceClassWithBaseTemplateSpecVisitor> {

public:
  explicit ReplaceClassWithBaseTemplateSpecVisitor(
             ReplaceClassWithBaseTemplateSpec *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitCXXRecordDecl(CXXRecordDecl *CXXRD);

private:
  ReplaceClassWithBaseTemplateSpec *ConsumerInstance;
};

bool ReplaceClassWithBaseTemplateSpecVisitor::VisitCXXRecordDecl(
       CXXRecordDecl *CXXRD)
{
  if (ConsumerInstance->isInIncludedFile(CXXRD) ||
      ConsumerInstance->isSpecialRecordDecl(CXXRD) ||
      !CXXRD->hasDefinition())
    return true;
  ConsumerInstance->handleOneCXXRecordDecl(CXXRD->getDefinition());
  return true;
}

class ReplaceClassWithBaseTemplateSpecRewriteVisitor : public
  RecursiveASTVisitor<ReplaceClassWithBaseTemplateSpecRewriteVisitor> {
public:
  explicit ReplaceClassWithBaseTemplateSpecRewriteVisitor(
             ReplaceClassWithBaseTemplateSpec *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitRecordTypeLoc(RecordTypeLoc TLoc);

private:
  ReplaceClassWithBaseTemplateSpec *ConsumerInstance;
};

bool ReplaceClassWithBaseTemplateSpecRewriteVisitor::VisitRecordTypeLoc(
       RecordTypeLoc TLoc)
{
  const Type *Ty = TLoc.getTypePtr();
  if (Ty->isUnionType())
    return true;

  const CXXRecordDecl *RD = dyn_cast<CXXRecordDecl>(TLoc.getDecl());
  if (!RD || (RD->getCanonicalDecl() != ConsumerInstance->TheCXXRecord))
    return true;

  ConsumerInstance->RewriteHelper->replaceRecordType(
    TLoc, ConsumerInstance->TheBaseName + " ");
  return true;
}

void ReplaceClassWithBaseTemplateSpec::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new ReplaceClassWithBaseTemplateSpecVisitor(this);
  RewriteVisitor = new ReplaceClassWithBaseTemplateSpecRewriteVisitor(this);
}

void ReplaceClassWithBaseTemplateSpec::HandleTranslationUnit(ASTContext &Ctx)
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

  TransAssert(TheCXXRecord && "TheCXXRecord is NULL!");
  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);
  TransAssert(RewriteVisitor && "NULL RewriteVisitor!");
  RewriteVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
  removeBaseSpecifier();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void ReplaceClassWithBaseTemplateSpec::handleOneCXXRecordDecl(
       const CXXRecordDecl *CXXRD)
{
  TransAssert(CXXRD && "NULL CXXRD!");
  TransAssert(CXXRD->isThisDeclarationADefinition() &&  "Not a definition!");

  if (getNumExplicitDecls(CXXRD))
    return;
  if (CXXRD->getNumBases() != 1)
    return;

  CXXRecordDecl::base_class_const_iterator I = CXXRD->bases_begin();
  const CXXBaseSpecifier *BS = I;
  const Type *Ty = BS->getType().getTypePtr();
  const CXXRecordDecl *Base = getBaseDeclFromType(Ty);
  if (!Base || !Base->hasDefinition() || !Base->getDescribedClassTemplate())
    return;

  ValidInstanceNum++;
  if (ValidInstanceNum == TransformationCounter) {
    BS->getType().getAsStringInternal(TheBaseName, 
                                      Context->getPrintingPolicy());
    TheCXXRecord = CXXRD;
  }
}

void ReplaceClassWithBaseTemplateSpec::removeBaseSpecifier(void)
{
  unsigned NumBases = TheCXXRecord->getNumBases(); (void)NumBases;
  TransAssert((NumBases == 1) && "TheCXXRecord can have only one base!");
  SourceLocation StartLoc = TheCXXRecord->getLocation();
  StartLoc = RewriteHelper->getLocationUntil(StartLoc, ':');
  SourceLocation EndLoc = RewriteHelper->getLocationUntil(StartLoc, '{');
  EndLoc = EndLoc.getLocWithOffset(-1);

  TheRewriter.RemoveText(SourceRange(StartLoc, EndLoc));
}

ReplaceClassWithBaseTemplateSpec::~ReplaceClassWithBaseTemplateSpec(void)
{
  delete CollectionVisitor;
  delete RewriteVisitor;
}

