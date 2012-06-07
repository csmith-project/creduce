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

#include "RemoveCtorInitializer.h"

#include "clang/Basic/SourceManager.h"
#include "clang/AST/RecursiveASTVisitor.h"

#include "TransformationManager.h"

using namespace clang;
using namespace llvm;

static const char *DescriptionMsg = 
"This pass tries to remove an initializer from a Ctor. \n";

static RegisterTransformation<RemoveCtorInitializer>
         Trans("remove-ctor-initializer", DescriptionMsg);

class RemoveCtorInitializerASTVisitor : public 
  RecursiveASTVisitor<RemoveCtorInitializerASTVisitor> {

public:
  explicit RemoveCtorInitializerASTVisitor(
             RemoveCtorInitializer *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitCXXConstructorDecl(CXXConstructorDecl *Ctor);

private:
  RemoveCtorInitializer *ConsumerInstance;
};

bool RemoveCtorInitializerASTVisitor::VisitCXXConstructorDecl(
       CXXConstructorDecl *Ctor)
{
  unsigned Idx = 0;
  for (CXXConstructorDecl::init_const_iterator I = Ctor->init_begin(),
       E = Ctor->init_end(); I != E; ++I) {
    const CXXCtorInitializer *Init = (*I);
    if (!Init->isWritten()) {
      continue;
    }
      
    if (!Init->isAnyMemberInitializer()) {
      Idx++;
      continue;
    }
    const FieldDecl *Field = Init->getAnyMember();
    TransAssert(Field && "Invalid Field for Init!");
    const Type *Ty = Field->getType().getTypePtr();
    if (!ConsumerInstance->isValidType(Ty)) {
      Idx++;
      continue;
    }

    ConsumerInstance->ValidInstanceNum++;
    if (ConsumerInstance->ValidInstanceNum == 
        ConsumerInstance->TransformationCounter) {
      ConsumerInstance->TheInitializer = Init;
      ConsumerInstance->TheCtorDecl = Ctor;
      ConsumerInstance->TheIndex = Idx;
    }
    Idx++;
  }
 
  return true;
}

void RemoveCtorInitializer::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new RemoveCtorInitializerASTVisitor(this);
}

void RemoveCtorInitializer::HandleTranslationUnit(ASTContext &Ctx)
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

  TransAssert(TheCtorDecl && "TheCtorDecl is NULL!");
  TransAssert(TheInitializer && "TheInitializer is NULL!");
  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  RewriteHelper->removeCXXCtorInitializer(TheInitializer, TheIndex,
                   getNumCtorWrittenInitializers(*TheCtorDecl));

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

bool RemoveCtorInitializer::isValidType(const Type *Ty)
{
  if (Ty->isReferenceType())
    return false;

  if (const RecordType *RTy = Ty->getAs<RecordType>()) {
    const CXXRecordDecl *CXXRD = dyn_cast<CXXRecordDecl>(RTy->getDecl());
    if (!CXXRD)
      return true;
    return CXXRD->hasDeclaredDefaultConstructor();
  }
  return true;
}

RemoveCtorInitializer::~RemoveCtorInitializer(void)
{
  if (CollectionVisitor)
    delete CollectionVisitor;
}

