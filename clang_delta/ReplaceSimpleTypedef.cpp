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

#include "ReplaceSimpleTypedef.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;
using namespace llvm;

static const char *DescriptionMsg =
"This pass replaces typedef names with the underlying type if the \
underlying type falls into one of the following categories: \
  * scalar type; \n\
  * union; \n\
  * non-templated class; \n\
  * pointer to scalar type\n";

static RegisterTransformation<ReplaceSimpleTypedef>
         Trans("replace-simple-typedef", DescriptionMsg);

class ReplaceSimpleTypedefCollectionVisitor : public
  RecursiveASTVisitor<ReplaceSimpleTypedefCollectionVisitor> {

public:
  explicit ReplaceSimpleTypedefCollectionVisitor(ReplaceSimpleTypedef *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitTypedefDecl(TypedefDecl *D);

private:
  ReplaceSimpleTypedef *ConsumerInstance;

};

class ReplaceSimpleTypedefRewriteVisitor : public
  RecursiveASTVisitor<ReplaceSimpleTypedefRewriteVisitor> {

public:
  explicit ReplaceSimpleTypedefRewriteVisitor(ReplaceSimpleTypedef *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitTypedefTypeLoc(TypedefTypeLoc Loc);

  bool VisitElaboratedTypeLoc(ElaboratedTypeLoc Loc);

private:
  ReplaceSimpleTypedef *ConsumerInstance;

};

bool ReplaceSimpleTypedefCollectionVisitor::VisitTypedefDecl(TypedefDecl *TdefD)
{
  TypedefDecl *CanonicalD = dyn_cast<TypedefDecl>(TdefD->getCanonicalDecl());
  if (!ConsumerInstance->VisitedTypedefDecls.count(CanonicalD)) {
    ConsumerInstance->handleOneTypedefDecl(CanonicalD);
    ConsumerInstance->VisitedTypedefDecls.insert(CanonicalD);
  }

  return true;
}

bool ReplaceSimpleTypedefRewriteVisitor::VisitTypedefTypeLoc(TypedefTypeLoc Loc)
{
  const TypedefType *TdefTy = Loc.getTypePtr();
  const TypedefDecl *TdefD = dyn_cast<TypedefDecl>(TdefTy->getDecl());
  if (!TdefD || TdefD->getLocStart().isInvalid())
    return true;
 
  if (dyn_cast<TypedefDecl>(TdefD->getCanonicalDecl()) == 
      ConsumerInstance->TheTypedefDecl) {
    SourceRange Range = Loc.getSourceRange();
    ConsumerInstance->TheRewriter.ReplaceText(Range, ConsumerInstance->TyName);
    ConsumerInstance->Rewritten = true;
  }
  return true;
}

// Handle cases like:
// struct S {
//   typedef int Int;
// };
// S::Int g;
// where S::Int is referred as an ElaboratedType
bool ReplaceSimpleTypedefRewriteVisitor::VisitElaboratedTypeLoc(
       ElaboratedTypeLoc Loc)
{
  const ElaboratedType *ETy = dyn_cast<ElaboratedType>(Loc.getTypePtr());
  const Type *NamedTy = ETy->getNamedType().getTypePtr();
  const TypedefType *TdefTy = NamedTy->getAs<TypedefType>();
  if (!TdefTy)
    return true; 

  const TypedefDecl *TdefD = dyn_cast<TypedefDecl>(TdefTy->getDecl());
  if (!TdefD || (dyn_cast<TypedefDecl>(TdefD->getCanonicalDecl()) != 
                 ConsumerInstance->TheTypedefDecl)) {
    return true;
  }

  NestedNameSpecifierLoc QualifierLoc = Loc.getQualifierLoc();
  if (QualifierLoc && ConsumerInstance->IsScalarType) {
    ConsumerInstance->TheRewriter.RemoveText(QualifierLoc.getSourceRange());
    ConsumerInstance->Rewritten = true;
  }
  return true;
}

void ReplaceSimpleTypedef::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new ReplaceSimpleTypedefCollectionVisitor(this);
  RewriteVisitor = new ReplaceSimpleTypedefRewriteVisitor(this);
}

void ReplaceSimpleTypedef::HandleTranslationUnit(ASTContext &Ctx)
{
  CollectionVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);
  TransAssert(TheTypedefDecl && "NULL TheTypedefDecl!");
  RewriteVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
  removeTypedefs();

  if (!Rewritten) {
    TransError = TransNoTextModificationError;
    return;
  }
  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void ReplaceSimpleTypedef::removeTypedefs()
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

bool ReplaceSimpleTypedef::isValidType(const Type *Ty, const TypedefDecl *D)
{
  if (Ty->isEnumeralType() || Ty->isUnionType())
    return true;

  if (const RecordType *RDTy = Ty->getAs<RecordType>()) {
    const RecordDecl *RD = RDTy->getDecl();
    // omit some trivial cases, e.g.,
    // typedef struct S { int x; } S;
    if (RD->getNameAsString() == D->getNameAsString())
      return false;

    if (TransformationManager::isCLangOpt())
      return true;
    const CXXRecordDecl *CXXRD = dyn_cast<CXXRecordDecl>(RD);
    TransAssert(CXXRD && "NULL CXXRecordDecl!");
    return (CXXRD->getDescribedClassTemplate() == NULL);
  }

  if (Ty->isPointerType())
    Ty = getBasePointerElemType(Ty);

  if (Ty->isScalarType()) {
    IsScalarType = true;
    return true;
  }
  return false;
}

void ReplaceSimpleTypedef::handleOneTypedefDecl(const TypedefDecl *CanonicalD)
{
  // Skip functions which are not in the main file
  // Rewriting outside of the main file is currently not supported
  if(!SrcManager->isInMainFile(CanonicalD->getLocStart()))
  {
    return;
  }

  // omit some typedefs injected by Clang
  if (CanonicalD->getLocStart().isInvalid())
    return;

  FullSourceLoc FullLoc = Context->getFullLoc(CanonicalD->getLocStart());
  if (FullLoc.isInSystemHeader())
    return;

  const Type *Ty = CanonicalD->getUnderlyingType().getTypePtr();
  if (!isValidType(Ty, CanonicalD))
    return;

  ValidInstanceNum++;
  if (ValidInstanceNum == TransformationCounter) {
    TheTypedefDecl = CanonicalD;
    CanonicalD->getUnderlyingType().getAsStringInternal(TyName, 
                                      Context->getPrintingPolicy());
  }
}

ReplaceSimpleTypedef::~ReplaceSimpleTypedef(void)
{
  delete CollectionVisitor;
  delete RewriteVisitor;
}

