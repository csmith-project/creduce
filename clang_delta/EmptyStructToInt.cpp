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

#include "EmptyStructToInt.h"

#include "clang/Basic/SourceManager.h"

#include "clang/Lex/Lexer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "TransformationManager.h"

using namespace clang;
using namespace llvm;

static const char *DescriptionMsg =
"Replace an empty struct with type of int. A struct is defined to be empty if \
it: \
  * does not have any field; \n\
  * does not have any base class; \n\
  * is not a base class of another class; \n\
  * is not described by any template; \n";

static RegisterTransformation<EmptyStructToInt>
         Trans("empty-struct-to-int", DescriptionMsg);

class EmptyStructToIntASTVisitor : public 
  RecursiveASTVisitor<EmptyStructToIntASTVisitor> {

public:
  explicit EmptyStructToIntASTVisitor(EmptyStructToInt *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitRecordDecl(RecordDecl *RD);

  bool VisitCXXRecordDecl(CXXRecordDecl *CXXRD);

private:
  EmptyStructToInt *ConsumerInstance;
};

class EmptyStructToIntRewriteVisitor : public 
  RecursiveASTVisitor<EmptyStructToIntRewriteVisitor> {

public:
  explicit EmptyStructToIntRewriteVisitor(EmptyStructToInt *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitRecordTypeLoc(RecordTypeLoc RTLoc);

  bool VisitElaboratedTypeLoc(ElaboratedTypeLoc Loc);

private:
  EmptyStructToInt *ConsumerInstance;
};

bool EmptyStructToIntASTVisitor::VisitRecordDecl(RecordDecl *RD)
{
  if (!ConsumerInstance->isValidRecordDecl(RD))
    return true;
 
  const RecordDecl *CanonicalRD = dyn_cast<RecordDecl>(RD->getCanonicalDecl());
  if (ConsumerInstance->VisitedRecordDecls.count(CanonicalRD))
    return true;

  ConsumerInstance->VisitedRecordDecls.insert(CanonicalRD);
  return true;
}

bool EmptyStructToIntASTVisitor::VisitCXXRecordDecl(CXXRecordDecl *CXXRD)
{
  const CXXRecordDecl *CanonicalRD = CXXRD->getCanonicalDecl();
  if (ConsumerInstance->VisitedRecordDecls.count(CanonicalRD))
    return true;

  if (!CanonicalRD->hasDefinition())
    return true;

  for (CXXRecordDecl::base_class_const_iterator I = 
       CanonicalRD->bases_begin(), E = CanonicalRD->bases_end(); I != E; ++I) {
    const CXXBaseSpecifier *BS = I;
    const Type *Ty = BS->getType().getTypePtr();
    const CXXRecordDecl *Base = ConsumerInstance->getBaseDeclFromType(Ty);
    if (Base)
      ConsumerInstance->BaseClassDecls.insert(Base->getCanonicalDecl());
  }
  return true;
}

bool EmptyStructToIntRewriteVisitor::VisitRecordTypeLoc(RecordTypeLoc RTLoc)
{
  const RecordDecl *RD = RTLoc.getDecl();
  if (RD->getCanonicalDecl() == ConsumerInstance->TheRecordDecl) {
    SourceLocation LocStart = RTLoc.getLocStart();
    void *LocPtr = LocStart.getPtrEncoding();
    if (ConsumerInstance->VisitedLocs.count(LocPtr))
      return true;
    ConsumerInstance->VisitedLocs.insert(LocPtr);

    // handle a special case -
    // struct S1 {
    //   struct { } S;
    // };
    const IdentifierInfo *TypeId = RTLoc.getType().getBaseTypeIdentifier();
    if (!TypeId)
      return true;
    ConsumerInstance->RewriteHelper->replaceRecordType(RTLoc, "int");
  }
  return true;
}

bool EmptyStructToIntRewriteVisitor::VisitElaboratedTypeLoc(
       ElaboratedTypeLoc Loc)
{
  const ElaboratedType *ETy = dyn_cast<ElaboratedType>(Loc.getTypePtr());
  const Type *NamedTy = ETy->getNamedType().getTypePtr();
  const RecordType *RDTy = NamedTy->getAs<RecordType>();
  if (!RDTy)
    return true;

  const RecordDecl *RD = RDTy->getDecl();
  TransAssert(RD && "NULL RecordDecl!");
  if (RD->getCanonicalDecl() != ConsumerInstance->TheRecordDecl) {
    return true;
  }

  SourceLocation StartLoc = Loc.getLocStart();
  TypeLoc TyLoc = Loc.getNamedTypeLoc();
  SourceLocation EndLoc = TyLoc.getLocStart();
  EndLoc = EndLoc.getLocWithOffset(-1);
  const char *StartBuf = 
    ConsumerInstance->SrcManager->getCharacterData(StartLoc);
  const char *EndBuf = ConsumerInstance->SrcManager->getCharacterData(EndLoc);
  // It's possible, e.g., 
  // struct S1 {
  //   struct { } S;
  // };
  // Clang will translate struct { } S to
  // struct {
  // };
  //  struct <anonymous struct ...> S;
  // the last declaration is injected by clang.
  // We need to omit it.
  if (StartBuf > EndBuf) {
    SourceLocation KeywordLoc = Loc.getElaboratedKeywordLoc();
    const char *Keyword = TypeWithKeyword::getKeywordName(ETy->getKeyword());
    ConsumerInstance->TheRewriter.ReplaceText(KeywordLoc, 
                                              strlen(Keyword), "int");
    return true;
  }
  
  ConsumerInstance->TheRewriter.RemoveText(SourceRange(StartLoc, EndLoc));
  return true;
}

void EmptyStructToInt::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new EmptyStructToIntASTVisitor(this);
  RewriteVisitor = new EmptyStructToIntRewriteVisitor(this);
}

void EmptyStructToInt::HandleTranslationUnit(ASTContext &Ctx)
{
  CollectionVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
  doAnalysis();

  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);
  removeRecordDecls();
  RewriteVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
  
  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void EmptyStructToInt::doAnalysis(void)
{
  for (RecordDeclSet::const_iterator I = VisitedRecordDecls.begin(),
       E = VisitedRecordDecls.end(); I != E; ++I) {
    const RecordDecl *RD = (*I);
    if (const CXXRecordDecl *CXXRD = dyn_cast<CXXRecordDecl>(RD)) {
      if (BaseClassDecls.count(CXXRD->getCanonicalDecl()))
        continue;
    }
    ValidInstanceNum++;
    if (ValidInstanceNum == TransformationCounter)
      TheRecordDecl = RD;
  }
}

// ISSUE: we will have bad transformation for the case below:
// typedef struct S;
// S *s;
// ==>
// typedef
// int *s;
// This is bad because we don't catch the implicit declaration of struct S.
// But hopefully peephole pass will remove the keyword typedef,
// then we will be fine.
void EmptyStructToInt::removeRecordDecls(void)
{
  for (RecordDecl::redecl_iterator I = TheRecordDecl->redecls_begin(),
      E = TheRecordDecl->redecls_end(); I != E; ++I) {

    const RecordDecl *RD = dyn_cast<RecordDecl>(*I);
    SourceRange Range = RD->getSourceRange();
    SourceLocation LocEnd = Range.getEnd();
    SourceLocation SemiLoc = 
      Lexer::findLocationAfterToken(LocEnd, 
                                    tok::semi,
                                    *SrcManager,
                                    Context->getLangOpts(),
                                    /*SkipTrailingWhitespaceAndNewLine=*/true);
    // handle cases such as 
    // struct S {} s;
    if (SemiLoc.isInvalid()) {
      if (!RD->isThisDeclarationADefinition())
        return;
      SourceLocation RBLoc = RD->getRBraceLoc();
      if (RBLoc.isInvalid())
        return;
      RewriteHelper->removeTextFromLeftAt(SourceRange(RBLoc, RBLoc),
                                          '{', RBLoc);
    }
    else {
      LocEnd = RewriteHelper->getEndLocationUntil(Range, ';');
      TheRewriter.RemoveText(SourceRange(Range.getBegin(), LocEnd));
    }
  }
}

bool EmptyStructToInt::isValidRecordDecl(const RecordDecl *RD)
{
  const CXXRecordDecl *CXXRD = dyn_cast<CXXRecordDecl>(RD);
  if (!CXXRD) {
    const RecordDecl *Def = RD->getDefinition();
    if (!Def)
      return true;
    else 
      return Def->field_empty();
  }

  if (dyn_cast<ClassTemplateSpecializationDecl>(CXXRD) ||
      CXXRD->getDescribedClassTemplate() ||
      CXXRD->getInstantiatedFromMemberClass())
    return false;

  // It's possible that the described template does not
  // have definition, so we test hasDefinition after the
  // above `if' guard
  const CXXRecordDecl *CXXDef = CXXRD->getDefinition();
  if (!CXXDef)
    return true;

  if(CXXDef->getNumBases())
    return false;

  const DeclContext *Ctx = dyn_cast<DeclContext>(CXXDef);
  TransAssert(Ctx && "Invalid DeclContext!");
  for (DeclContext::decl_iterator I = Ctx->decls_begin(),
       E = Ctx->decls_end(); I != E; ++I) {
    if (!(*I)->isImplicit())
      return false;
  }
  return true;
}

EmptyStructToInt::~EmptyStructToInt(void)
{
  if (CollectionVisitor)
    delete CollectionVisitor;
  if (RewriteVisitor)
    delete RewriteVisitor;
}

