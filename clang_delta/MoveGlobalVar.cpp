//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2015, 2016 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "MoveGlobalVar.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Try to move global var/struct/union declarations above all function \
declarations except printf. Also move the declaration of printf to the top of \
the reduce code if it exists and is not at the top of the code.\n";

static RegisterTransformation<MoveGlobalVar>
         Trans("move-global-var", DescriptionMsg);

void MoveGlobalVar::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
}

bool MoveGlobalVar::isSpecialDecl(const std::string &Name)
{
  return ((Name == "__va_list_tag") ||
          (Name == "__builtin_va_list"));
}

bool MoveGlobalVar::HandleTopLevelDecl(DeclGroupRef D) 
{
  if (TransformationManager::isCXXLangOpt()) {
    ValidInstanceNum = 0;
    return true;
  }

  DeclGroupRef::iterator I = D.begin();
  TransAssert((I != D.end()) && "Bad DeclGroupRef!");

  if (isInIncludedFile(*I))
    return true;

  const NamedDecl *ND = dyn_cast<NamedDecl>(*I);
  if (!TheFirstDecl && ND && isSpecialDecl(ND->getNameAsString()))
    return true;

  FunctionDecl *FD = dyn_cast<FunctionDecl>(*I);
  if (FD) {
    handleFunctionDecl(FD); 
  }
  else {
    handleOtherDecl(D);
  }

  if (!TheFirstDecl)
    TheFirstDecl = (*I);
  return true;
}

void MoveGlobalVar::HandleTranslationUnit(ASTContext &Ctx)
{
  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  if (ThePrintfDecl)
    liftPrintfDecl();
  else
    liftOtherDecl();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void MoveGlobalVar::handleFunctionDecl(const FunctionDecl *FD)
{
  if (FD->getNameAsString() == "printf") {
    if (ThePrintfDecl || !TheFirstDecl || FD->isThisDeclarationADefinition())
      return;

    ValidInstanceNum++;
    if (TransformationCounter == ValidInstanceNum)
      ThePrintfDecl = FD;
  }
  else if (!TheFirstFunctionDecl) {
    TheFirstFunctionDecl = FD;
  }
}

void MoveGlobalVar::handleOtherDecl(DeclGroupRef DGR)
{
  if (!TheFirstFunctionDecl)
    return;

  ValidInstanceNum++;
  if (TransformationCounter == ValidInstanceNum)
    TheDGRPointer = DGR.getAsOpaquePtr();
}

void MoveGlobalVar::liftPrintfDecl(void)
{
  TransAssert(ThePrintfDecl && TheFirstDecl && 
              (ThePrintfDecl != TheFirstDecl) && "Invalid printf decl!");

  std::string PrintfDeclStr;
  RewriteHelper->getFunctionDeclStrAndRemove(ThePrintfDecl, PrintfDeclStr);

  SourceRange DeclRange = TheFirstDecl->getSourceRange();
  SourceLocation StartLoc = DeclRange.getBegin();
  PrintfDeclStr += ";\n";
  TheRewriter.InsertTextBefore(StartLoc, PrintfDeclStr);
}

void MoveGlobalVar::liftOtherDecl(void)
{
  TransAssert(TheDGRPointer && "NULL DGR pointer!");
  TransAssert(TheFirstFunctionDecl && "NULL First Decl!");
  DeclGroupRef DGR = DeclGroupRef::getFromOpaquePtr(TheDGRPointer);

  std::string DGRStr;
  RewriteHelper->getEntireDeclGroupStrAndRemove(DGR, DGRStr);

  SourceRange DeclRange = TheFirstFunctionDecl->getSourceRange();
  SourceLocation StartLoc = DeclRange.getBegin();
  DGRStr += ";\n";
  TheRewriter.InsertTextBefore(StartLoc, DGRStr);
}

MoveGlobalVar::~MoveGlobalVar(void)
{
  // Nothing to do
}

