//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2014, 2015, 2016, 2017 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "ReplaceFunctionDefWithDecl.h"

#include <cctype>
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"This pass replaces a function's definition with its declaration.\n";

static RegisterTransformation<ReplaceFunctionDefWithDecl>
         Trans("replace-function-def-with-decl", DescriptionMsg);

class ReplaceFunctionDefWithDeclCollectionVisitor : public 
        RecursiveASTVisitor<ReplaceFunctionDefWithDeclCollectionVisitor> {
public:

  explicit ReplaceFunctionDefWithDeclCollectionVisitor(
             ReplaceFunctionDefWithDecl *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitFunctionDecl(FunctionDecl *FD);

private:

  ReplaceFunctionDefWithDecl *ConsumerInstance;
};

bool ReplaceFunctionDefWithDeclCollectionVisitor::VisitFunctionDecl(
       FunctionDecl *FD)
{
  if (ConsumerInstance->isInIncludedFile(FD))
    return true;

  if (FD->isThisDeclarationADefinition() && 
      FD->hasBody() &&
      !FD->isDeleted() &&
      !FD->isDefaulted() &&
      !ConsumerInstance->isMacroExpansion(FD))
    ConsumerInstance->addOneFunctionDef(FD);
  return true;
}

void ReplaceFunctionDefWithDecl::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new ReplaceFunctionDefWithDeclCollectionVisitor(this);
}

void ReplaceFunctionDefWithDecl::HandleTranslationUnit(ASTContext &Ctx)
{
  CollectionVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }
  if (ToCounter > ValidInstanceNum) {
    TransError = TransToCounterTooBigError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  doRewriting();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

unsigned ReplaceFunctionDefWithDecl::getNumWrittenInitializers(
           const CXXConstructorDecl *Ctor)
{
  unsigned Num = 0;
  for (CXXConstructorDecl::init_const_iterator I = Ctor->init_begin(),
       E = Ctor->init_end(); I != E; ++I) {
    const CXXCtorInitializer *Init = (*I);
    if (Init->isWritten())
      Num++;
  }
  return Num;
}

void ReplaceFunctionDefWithDecl::removeCtorInitializers(
       const CXXConstructorDecl *Ctor)
{
  if (!getNumWrittenInitializers(Ctor))
    return;
  CXXConstructorDecl::init_const_iterator I = Ctor->init_begin();
  while (!(*I)->isWritten())
    ++I;
  const CXXCtorInitializer *FirstInit = (*I);
  SourceRange Range = FirstInit->getSourceRange();
  SourceLocation LocStart = Range.getBegin();
  // RewriteHelper->removeTextFromLeftAt(Range, ':', 
  //                                     LocStart.getLocWithOffset(-1));
  // make sure we handle cases like:
  // namespace NS { struct A {}; }
  // struct B : NS::A { B() : NS::A() {} };
  SourceLocation Loc = RewriteHelper->getLocationFromLeftUntil(LocStart, ':');
  Loc = RewriteHelper->getLocationFromLeftUntil(Loc, ')');
  TheRewriter.RemoveText(SourceRange(Loc.getLocWithOffset(1), 
                                     LocStart.getLocWithOffset(-1)));
  CXXConstructorDecl::init_const_iterator E = Ctor->init_end();
  --E;
  while (!(*E)->isWritten())
    --E;
  const CXXCtorInitializer *LastInit = (*E);
  TransAssert(LastInit->isWritten() && "Init is not written!");
  SourceLocation LocEnd = LastInit->getSourceRange().getEnd();
  TheRewriter.RemoveText(SourceRange(LocStart, LocEnd));
}

bool ReplaceFunctionDefWithDecl::hasValidOuterLocStart(
       const FunctionTemplateDecl *FTD, const FunctionDecl *FD)
{
  SourceLocation FTDLocStart = FTD->getSourceRange().getBegin();
  SourceLocation FDLocStart = FD->getSourceRange().getBegin();
  const char *FTDStartPos = SrcManager->getCharacterData(FTDLocStart);
  const char *FDStartPos = SrcManager->getCharacterData(FDLocStart);
  return (FDStartPos < FTDStartPos); 
}

bool ReplaceFunctionDefWithDecl::removeOneInlineKeyword(
       const std::string &LeadingInlineStr, 
       const std::string &InlineStr, 
       const std::string &Str,
       const SourceLocation &StartLoc)
{
  if (!Str.compare(0, LeadingInlineStr.length(), LeadingInlineStr)) {
    TheRewriter.RemoveText(SourceRange(
                  StartLoc,
                  StartLoc.getLocWithOffset(LeadingInlineStr.length() - 1)));
    return true;
  }

  size_t Off = Str.find(InlineStr);
  if (Off == std::string::npos)
    return false;

  TheRewriter.RemoveText(SourceRange(
                StartLoc.getLocWithOffset(Off),
                StartLoc.getLocWithOffset(Off + InlineStr.length() - 1)));
  return true;
}

bool ReplaceFunctionDefWithDecl::removeInlineKeyword(
       const std::string &InlineStr, 
       const std::string &Str,
       const SourceLocation &StartLoc)
{
  char Spaces[] = {' ', '\t', '\r', '\n'};
  unsigned Len = sizeof(Spaces) / sizeof(char);
  for (unsigned I = 0; I < Len; ++I) {
    std::string LeadingInlineStr = InlineStr + Spaces[I];
    for (unsigned J = 0; J < Len; ++J) {
      for (unsigned K = 0; K < Len; ++K) {
        std::string InlineStrVariant = Spaces[J] + InlineStr + Spaces[K];
        if (removeOneInlineKeyword(LeadingInlineStr, InlineStrVariant, 
                                   Str, StartLoc))
          return true;
      }
    }
  }
  return false;
}

void ReplaceFunctionDefWithDecl::removeStringBeforeTypeIdentifier(
       const SourceLocation &StartLoc, const SourceLocation &EndLoc)
{
  const char *StartPos = SrcManager->getCharacterData(StartLoc);
  const char *EndPos = SrcManager->getCharacterData(EndLoc);
  // skip the first char of function's name
  EndPos--;
  while (isspace(*EndPos) && (EndPos != StartPos)) {
    EndPos--;
  }
  assert((EndPos > StartPos) && "Invalid EndPos!");
  while (!isspace(*EndPos) && (EndPos != StartPos)) {
    EndPos--;
  }
  EndPos++;
  assert((EndPos != StartPos) && "Bad Type Location?");
  TheRewriter.RemoveText(StartLoc, EndPos - StartPos);
}

void ReplaceFunctionDefWithDecl::removeInlineKeywordFromOneFunctionDecl(
       const FunctionDecl *FD)
{
  if (!FD->isInlineSpecified())
    return;
  SourceLocation StartLoc = FD->getSourceRange().getBegin();
  SourceLocation EndLoc = FD->getLocation();
  std::string Str;
  RewriteHelper->getStringBetweenLocs(Str, StartLoc, EndLoc);
  if (removeInlineKeyword("inline", Str, StartLoc))
    return;
  if (removeInlineKeyword("_inline", Str, StartLoc))
    return;
  if (removeInlineKeyword("__inline", Str, StartLoc))
    return;
  if (removeInlineKeyword("__forceinline", Str, StartLoc))
    return;
  if (removeInlineKeyword("__inline__", Str, StartLoc))
    return;
  // OK, just remove whatever appears before the type identifier...
  // It's mainly for dealing with non-preprocessed code 
  removeStringBeforeTypeIdentifier(StartLoc, EndLoc);
}

void ReplaceFunctionDefWithDecl::removeInlineKeywordFromFunctionDecls(
       const FunctionDecl *FD)
{
  if (!FD->isInlineSpecified())
    return;

  const FunctionDecl *FirstFD = FD->getCanonicalDecl();
  for (FunctionDecl::redecl_iterator I = FirstFD->redecls_begin(),
       E = FirstFD->redecls_end(); I != E; ++I) {
    removeInlineKeywordFromOneFunctionDecl(*I);
  }
}

void ReplaceFunctionDefWithDecl::rewriteOneFunctionDef(
       const FunctionDecl *FD)
{
  const CXXMethodDecl *CXXMD = dyn_cast<CXXMethodDecl>(FD);
  if (!CXXMD) {
    RewriteHelper->replaceFunctionDefWithStr(FD, ";");
    // compiler warns about used-but-not-defined inlined specified function, 
    // so get rid of the inline keyword from FD's decls
    removeInlineKeywordFromFunctionDecls(FD);
    return;
  }

  if (CXXMD->isOutOfLine()) {
    // Not sure why, but FD->getOuterLocStart() doesn't work well for 
    // function template decl, e.g. for the code below:
    //   struct A { template<typename T> A(); };
    //   template <typename T> A::A() {}
    // FD->getOuterLocStart() returns the same LocStart as 
    // FD->getSourceRange().getBegin(), so we have to check if FD has 
    // described function template
    if (FunctionTemplateDecl *FTD = FD->getDescribedFunctionTemplate()) {
      // here is another ugly part, without this check, we couldn't remove
      // "template <typename T> in the following code:
      //   template <typename T> struct S {template <typename T1> void foo();};
      //   template<typename T> template<typename T1> void S<T>::foo() { }
      if (!hasValidOuterLocStart(FTD, FD)) {
        TheRewriter.RemoveText(FTD->getSourceRange());
        return;
      }
    }
    SourceRange R = FD->getSourceRange();
    SourceLocation LocStart = R.getBegin();
    SourceLocation LocEnd = R.getEnd();
    if (LocStart.isMacroID()) {
      LocStart = SrcManager->getFileLoc(LocStart);
    }
    TheRewriter.RemoveText(SourceRange(LocStart, LocEnd));
    return;
  }

  if (const CXXConstructorDecl *Ctor = 
      dyn_cast<const CXXConstructorDecl>(FD)) {
    removeCtorInitializers(Ctor);
  }
  RewriteHelper->replaceFunctionDefWithStr(FD, ";");
  removeInlineKeywordFromFunctionDecls(FD);
}

void ReplaceFunctionDefWithDecl::doRewriting()
{
  if (ToCounter <= 0) {
    TransAssert(TheFunctionDef && "NULL TheFunctionDef!");
    rewriteOneFunctionDef(TheFunctionDef);
    return;
  }

  TransAssert((TransformationCounter <= 
                 static_cast<int>(AllValidFunctionDefs.size())) &&
              "TransformationCounter is larger than the number of defs!");
  TransAssert((ToCounter <= static_cast<int>(AllValidFunctionDefs.size())) &&
              "ToCounter is larger than the number of defs!");
  // To cope with local struct definition defined inside a function 
  // to be replaced, e.g.:
  // void foo(void) { { struct A { A() {} }; } }
  // If we replace foo() {...} first, we will mess up when we try to
  // replace A() {} because its text has gone already
  for (int I = ToCounter; I >= TransformationCounter; --I) {
    TransAssert((I >= 1) && "Invalid Index!");
    const FunctionDecl *FD = AllValidFunctionDefs[I-1];
    TransAssert(FD && "NULL FunctionDecl!");
    rewriteOneFunctionDef(FD);
  }
}

bool ReplaceFunctionDefWithDecl::isMacroExpansion(const FunctionDecl *FD)
{
  const Stmt *Body = FD->getBody();
  if (!Body)
    return false;
  return SrcManager->isMacroBodyExpansion(Body->getLocStart());
}

void ReplaceFunctionDefWithDecl::addOneFunctionDef(const FunctionDecl *FD)
{
  ValidInstanceNum++;
  if (ToCounter > 0) {
    AllValidFunctionDefs.push_back(FD);
    return;
  }
  if (ValidInstanceNum == TransformationCounter)
    TheFunctionDef = FD;
}

ReplaceFunctionDefWithDecl::~ReplaceFunctionDefWithDecl()
{
  delete CollectionVisitor;
}

