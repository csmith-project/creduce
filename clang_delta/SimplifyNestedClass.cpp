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

#include "SimplifyNestedClass.h"

#include "clang/Basic/SourceManager.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Lex/Lexer.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg = 
"This pass tries to simplify nested classes by replacing the \
outer class with the inner class, if \n\
  * the outer class doesn't have any base class, and \n\
  * the outer class has only one inner class definition, and \n\
  * the outer class does not have any described template, and \n\
  * the outer class does not have any other declarations except \
the inner class \n\
";

static RegisterTransformation<SimplifyNestedClass>
         Trans("simplify-nested-class", DescriptionMsg);

class SimplifyNestedClassVisitor : public 
  RecursiveASTVisitor<SimplifyNestedClassVisitor> {

public:
  explicit SimplifyNestedClassVisitor(
             SimplifyNestedClass *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitCXXRecordDecl(CXXRecordDecl *CXXRD);

private:
  SimplifyNestedClass *ConsumerInstance;
};

bool SimplifyNestedClassVisitor::VisitCXXRecordDecl(
       CXXRecordDecl *CXXRD)
{
  if (ConsumerInstance->isInIncludedFile(CXXRD) ||
      ConsumerInstance->isSpecialRecordDecl(CXXRD) || !CXXRD->hasDefinition())
    return true;
  ConsumerInstance->handleOneCXXRecordDecl(CXXRD->getDefinition());
  return true;
}

class SimplifyNestedClassRewriteVisitor : public
  RecursiveASTVisitor<SimplifyNestedClassRewriteVisitor> {
public:
  explicit SimplifyNestedClassRewriteVisitor(
             SimplifyNestedClass *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitRecordTypeLoc(RecordTypeLoc TLoc);

private:
  SimplifyNestedClass *ConsumerInstance;
};

bool SimplifyNestedClassRewriteVisitor::VisitRecordTypeLoc(RecordTypeLoc TLoc)
{
  const CXXRecordDecl *RD = dyn_cast<CXXRecordDecl>(TLoc.getDecl());
  if (!RD || (RD->getCanonicalDecl() != 
              ConsumerInstance->TheBaseCXXRD->getCanonicalDecl()))
    return true;

  if (ConsumerInstance->isBeforeColonColon(TLoc)) {
    SourceLocation LocEnd = 
      ConsumerInstance->RewriteHelper->getLocationAfter(
        TLoc.getEndLoc(), ':');
    ConsumerInstance->TheRewriter.RemoveText(
                        SourceRange(TLoc.getBeginLoc(), LocEnd));
  }
  else {
    ConsumerInstance->RewriteHelper->replaceRecordType(TLoc,
      ConsumerInstance->TheBaseCXXRD->getNameAsString() + " ");
  }
  return true;
}

void SimplifyNestedClass::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new SimplifyNestedClassVisitor(this);
  RewriteVisitor = new SimplifyNestedClassRewriteVisitor(this);
}

void SimplifyNestedClass::HandleTranslationUnit(ASTContext &Ctx)
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

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);
  TransAssert(RewriteVisitor && "NULL RewriteVisitor!");
  RewriteVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
  removeOuterClass();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void SimplifyNestedClass::removeOuterClass()
{
  TransAssert(TheBaseCXXRD && "NULL Base CXXRD!");
  SourceLocation LocStart = TheBaseCXXRD->getLocStart();
  SourceLocation LocEnd = TheInnerDecl->getLocStart();
  LocEnd = LocEnd.getLocWithOffset(-1);
  TheRewriter.RemoveText(SourceRange(LocStart, LocEnd));

  LocStart = TheBaseCXXRD->getBraceRange().getEnd();
  LocEnd = RewriteHelper->getLocationUntil(LocStart, ';');
  if (LocStart.isInvalid() || LocEnd.isInvalid())
    return;

  TheRewriter.RemoveText(SourceRange(LocStart, LocEnd));
}

void SimplifyNestedClass::handleOneCXXRecordDecl(const CXXRecordDecl *CXXRD)
{
  TransAssert(CXXRD && "NULL CXXRD!");
  TransAssert(CXXRD->isThisDeclarationADefinition() &&  "Not a definition!");
  if (CXXRD->getDescribedClassTemplate() || 
      CXXRD->getNumBases() ||
      dyn_cast<ClassTemplateSpecializationDecl>(CXXRD))
    return;
  // anon class
  if (CXXRD->getNameAsString() == "")
    return;

  const Decl *InnerDecl = NULL;
  const DeclContext *Ctx = dyn_cast<DeclContext>(CXXRD);
  for (DeclContext::decl_iterator I = Ctx->decls_begin(),
       E = Ctx->decls_end(); I != E; ++I) {
    if ((*I)->isImplicit() || dyn_cast<AccessSpecDecl>(*I))
      continue;
    if (dyn_cast<CXXRecordDecl>(*I) || dyn_cast<ClassTemplateDecl>(*I)) {
      if (InnerDecl)
        return;
      InnerDecl = (*I);
    }
    else {
      return;
    }
  }
  if (!InnerDecl)
    return;
  ValidInstanceNum++;
  if (ValidInstanceNum == TransformationCounter) {
    TheBaseCXXRD = CXXRD;
    TheInnerDecl = InnerDecl;
  }
}

SimplifyNestedClass::~SimplifyNestedClass(void)
{
  delete CollectionVisitor;
  delete RewriteVisitor;
}

