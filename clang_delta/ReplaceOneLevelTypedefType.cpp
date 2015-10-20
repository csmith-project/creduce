//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "ReplaceOneLevelTypedefType.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;
using namespace llvm;

static const char *DescriptionMsg =
"This pass literally replaces a typedef type with the underlying type \
if typedef type is only been used once. It doesn't recursively resolve \
the underlying type.";

static RegisterTransformation<ReplaceOneLevelTypedefType>
         Trans("replace-one-level-typedef-type", DescriptionMsg);

class ReplaceOneLevelTypedefTypeCollectionVisitor : public
  RecursiveASTVisitor<ReplaceOneLevelTypedefTypeCollectionVisitor> {

public:
  explicit ReplaceOneLevelTypedefTypeCollectionVisitor(
             ReplaceOneLevelTypedefType *Instance)
    : ConsumerInstance(Instance)
  { }

  // Visits all locations where a typedef'd type is used
  bool VisitTypedefTypeLoc(TypedefTypeLoc TLoc);

private:
  ReplaceOneLevelTypedefType *ConsumerInstance;
};

bool ReplaceOneLevelTypedefTypeCollectionVisitor::VisitTypedefTypeLoc(
       TypedefTypeLoc TLoc)
{
  ConsumerInstance->handleOneTypedefTypeLoc(TLoc);
  return true;
}

void ReplaceOneLevelTypedefType::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new ReplaceOneLevelTypedefTypeCollectionVisitor(this);
}

void ReplaceOneLevelTypedefType::HandleTranslationUnit(ASTContext &Ctx)
{
  CollectionVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
  analyzeTypeLocs();

  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);
  TransAssert(TheTypedefDecl && "NULL TheTypedefDecl!");
  rewriteTypedefType();
  removeTypedefs();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void ReplaceOneLevelTypedefType::analyzeTypeLocs()
{
  for (TypedefDeclToRefMap::iterator I = AllTypeDecls.begin(),
       E = AllTypeDecls.end(); I != E; ++I) {
    TypedefTypeLocVector *LocVec = (*I).second;

    // Only deal with typedefs which are used once
    if (LocVec->size() > 1)
      continue;
    ValidInstanceNum++;
    if (ValidInstanceNum == TransformationCounter) {
      TheTypedefDecl = (*I).first;
      TheTypeLoc = LocVec->back();
    }
  }
}

void ReplaceOneLevelTypedefType::rewriteTypedefType()
{
  std::string NewTyStr = TheTypedefDecl->getUnderlyingType().getAsString();
  SourceRange Range = TheTypeLoc.getSourceRange();
  TheRewriter.ReplaceText(Range, NewTyStr);
}

void ReplaceOneLevelTypedefType::removeTypedefs()
{
  for (TypedefDecl::redecl_iterator I = TheTypedefDecl->redecls_begin(),
       E = TheTypedefDecl->redecls_end(); I != E; ++I) {
    SourceRange Range = (*I)->getSourceRange();
    if (Range.isValid()) {
      RewriteHelper->removeTextUntil(Range, ';');
      Rewritten = true;
    }
  }
}

void ReplaceOneLevelTypedefType::handleOneTypedefTypeLoc(TypedefTypeLoc TLoc)
{
  const TypedefType *TdefTy = TLoc.getTypePtr();
  const TypedefDecl *TdefD = dyn_cast<TypedefDecl>(TdefTy->getDecl());

  if (!TdefD || TdefD->getLocStart().isInvalid() || !SrcManager->isInMainFile(TdefD->getLocStart()))
    return;

  // This can be used to process only the main file and those files which are (recursively) included from the main file
  // It then replaces !SrcManager->isInMainFile(...)
  // But at the moment only writing of the main file is supported
  //FileID rootFile;
  //SourceLocation usageLocation = TdefD->getLocStart();
  //
  //do
  //{
  //    rootFile = SrcManager->getFileID(usageLocation);
  //    usageLocation = SrcManager->getIncludeLoc(rootFile);
  //} while(usageLocation.isValid());

  //if(rootFile != SrcManager->getMainFileID())
  //    return;

  const TypedefDecl *CanonicalD = 
    dyn_cast<TypedefDecl>(TdefD->getCanonicalDecl());

  TypedefTypeLocVector *LocVec = AllTypeDecls[CanonicalD];

  if (!LocVec) {
    LocVec = new TypedefTypeLocVector();
    TransAssert(LocVec && "NULL LocVec!");
    AllTypeDecls[CanonicalD] = LocVec;
  }

  LocVec->push_back(TLoc);
}

ReplaceOneLevelTypedefType::~ReplaceOneLevelTypedefType(void)
{
  for (TypedefDeclToRefMap::iterator I = AllTypeDecls.begin(),
       E = AllTypeDecls.end(); I != E; ++I) {
    delete (*I).second;
  }
  delete CollectionVisitor;
}

