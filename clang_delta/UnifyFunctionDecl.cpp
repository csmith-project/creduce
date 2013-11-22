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

#include "UnifyFunctionDecl.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"If a function is declared as static, make its definition \
as static as well, and remove the declaration. \n";

static RegisterTransformation<UnifyFunctionDecl>
         Trans("unify-function-decl", DescriptionMsg);

void UnifyFunctionDecl::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
}

bool UnifyFunctionDecl::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    const FunctionDecl *FD = dyn_cast<FunctionDecl>(*I);
    if (!FD)
      return true;    

    if (!FD->hasBody())
      return true;

    const FunctionDecl *CanonicalFD = FD->getCanonicalDecl();
    if (VisitedFunctionDecls.count(CanonicalFD))
      return true;

    VisitedFunctionDecls.insert(CanonicalFD);
  }
  return true;
}

void UnifyFunctionDecl::HandleTranslationUnit(ASTContext &Ctx)
{
  doAnalysis();

  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  TransAssert(TheFunctionDecl && "NULL TheFunctionDecl!");
  TransAssert(TheFunctionDef && "NULL TheFunctionDef!");

  doRewriting();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void UnifyFunctionDecl::doAnalysis(void)
{
  for (FunctionDeclSet::iterator I = VisitedFunctionDecls.begin(),
       E = VisitedFunctionDecls.end(); I != E; ++I) {

    const FunctionDecl *FDDef = NULL; 
    const FunctionDecl *FDDecl = NULL;
    const FunctionDecl *CanonicalFD = (*I);
    const FunctionDecl *FirstFD = CanonicalFD->getCanonicalDecl();

    FunctionDecl::redecl_iterator RI = FirstFD->redecls_begin();
    if (FirstFD->getSourceRange().isInvalid())
      continue;
    ++RI;
    if (RI == FirstFD->redecls_end())
      continue;

    const FunctionDecl *SecondFD = (*RI);
    if (SecondFD->getSourceRange().isInvalid())
      continue;
    if (FirstFD->isThisDeclarationADefinition()) {
      FDDef = FirstFD;
      TransAssert(!SecondFD->isThisDeclarationADefinition() &&
                  "Duplicated Definition!");
      FDDecl = SecondFD;
    }
    else if (SecondFD->isThisDeclarationADefinition()) {
      FDDef = SecondFD;
      TransAssert(!FirstFD->isThisDeclarationADefinition() &&
                  "Duplicated Definition!");
      FDDecl = FirstFD;
    }
    else {
      continue;
    }

    if (!hasStaticKeyword(FDDecl) || hasStaticKeyword(FDDef))
      continue;

    ValidInstanceNum++;
    if (TransformationCounter == ValidInstanceNum) {
      TheFunctionDef = FDDef;
      TheFunctionDecl = FDDecl;
    }
  }
}

bool UnifyFunctionDecl::hasStaticKeyword(const FunctionDecl *FD)
{
  SourceRange FDRange = FD->getSourceRange();
  SourceLocation StartLoc = FDRange.getBegin();
  SourceLocation EndLoc = FD->getLocation();

  const char *StartBuf = SrcManager->getCharacterData(StartLoc);
  const char *EndBuf = SrcManager->getCharacterData(EndLoc);
  std::string Str;
  if (StartBuf == EndBuf)
    return false;

  TransAssert(StartBuf < EndBuf);
  size_t Off = EndBuf - StartBuf;
  Str.assign(StartBuf, Off);

  size_t Pos = Str.find("static");
  return (Pos != std::string::npos);
}

void UnifyFunctionDecl::doRewriting(void)
{
  SourceRange FDDefRange = TheFunctionDef->getSourceRange();
  SourceLocation StartLoc = FDDefRange.getBegin();
  TheRewriter.InsertTextBefore(StartLoc, "static ");

  std::string TmpStr;
  RewriteHelper->getFunctionDeclStrAndRemove(TheFunctionDecl, TmpStr);
}

UnifyFunctionDecl::~UnifyFunctionDecl(void)
{
  // Nothing to do
}

