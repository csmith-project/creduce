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

#include "RemoveUnusedOuterClass.h"

#include "clang/Basic/SourceManager.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Lex/Lexer.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg = 
"This pass an unused outer class if \n\
  * the outer class doesn't have any base class, and \n\
  * the outer class does not have any described template, and \n\
  * the outer class is not been referenced \n\
";

static RegisterTransformation<RemoveUnusedOuterClass>
         Trans("remove-unused-outer-class", DescriptionMsg);

class RemoveUnusedOuterClassVisitor : public 
  RecursiveASTVisitor<RemoveUnusedOuterClassVisitor> {

public:
  explicit RemoveUnusedOuterClassVisitor(
             RemoveUnusedOuterClass *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitCXXRecordDecl(CXXRecordDecl *CXXRD);

  bool VisitRecordTypeLoc(RecordTypeLoc TLoc);

private:
  RemoveUnusedOuterClass *ConsumerInstance;
};

bool RemoveUnusedOuterClassVisitor::VisitRecordTypeLoc(RecordTypeLoc TLoc)
{
  const CXXRecordDecl *RD = dyn_cast<CXXRecordDecl>(TLoc.getDecl());
  ConsumerInstance->UsedCXXRDSet.insert(RD->getCanonicalDecl());
  return true;
}

bool RemoveUnusedOuterClassVisitor::VisitCXXRecordDecl(
       CXXRecordDecl *CXXRD)
{
  if (ConsumerInstance->isInIncludedFile(CXXRD) ||
      ConsumerInstance->isSpecialRecordDecl(CXXRD) ||
      !CXXRD->hasDefinition() ||
      dyn_cast<ClassTemplateSpecializationDecl>(CXXRD) ||
      CXXRD->hasUserDeclaredConstructor() ||
      CXXRD->hasUserDeclaredDestructor() ||
      CXXRD->getDescribedClassTemplate() || 
      CXXRD->getNumBases())
    return true;
  ConsumerInstance->CXXRDDefSet.insert(CXXRD->getDefinition());
  return true;
}

void RemoveUnusedOuterClass::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new RemoveUnusedOuterClassVisitor(this);
}

void RemoveUnusedOuterClass::HandleTranslationUnit(ASTContext &Ctx)
{
  if (TransformationManager::isCLangOpt() ||
      TransformationManager::isOpenCLLangOpt()) {
    ValidInstanceNum = 0;
  }
  else {
    CollectionVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
    analyzeCXXRDSet();
  }

  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);
  removeOuterClass();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void RemoveUnusedOuterClass::analyzeCXXRDSet()
{
  for (CXXRecordDeclSetVector::iterator I = CXXRDDefSet.begin(), 
       E = CXXRDDefSet.end(); I != E; ++I) {
    const CXXRecordDecl *Def = (*I);
    if (UsedCXXRDSet.count(Def->getCanonicalDecl()))
      continue;
    ValidInstanceNum++;
    if (ValidInstanceNum == TransformationCounter)
      TheCXXRDDef = Def;
  }
}

void RemoveUnusedOuterClass::removeOuterClass()
{
  TransAssert(TheCXXRDDef && "NULL Base CXXRD!");
  SourceLocation LocStart = TheCXXRDDef->getLocStart();
  SourceLocation LocEnd = 
    RewriteHelper->getEndLocationUntil(LocStart, '{');
  TransAssert(LocEnd.isValid() && "Invalid Location!");
  TheRewriter.RemoveText(SourceRange(LocStart, LocEnd));

  const DeclContext *Ctx = dyn_cast<DeclContext>(TheCXXRDDef);
  for (DeclContext::decl_iterator I = Ctx->decls_begin(),
       E = Ctx->decls_end(); I != E; ++I) {
    if ((*I)->isImplicit())
      continue;
    const AccessSpecDecl *AS = dyn_cast<AccessSpecDecl>(*I);
    if (!AS)
      continue;
    TheRewriter.RemoveText(AS->getSourceRange());
  }
  
  LocStart = TheCXXRDDef->getBraceRange().getEnd();
  LocEnd = RewriteHelper->getLocationUntil(LocStart, ';');
  if (LocStart.isInvalid() || LocEnd.isInvalid())
    return;

  TheRewriter.RemoveText(SourceRange(LocStart, LocEnd));
}

RemoveUnusedOuterClass::~RemoveUnusedOuterClass(void)
{
  delete CollectionVisitor;
}

