//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MoveFunctionBody.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Move function body towards its declaration. \
Note that this pass would generate incompilable code. \n";

static RegisterTransformation<MoveFunctionBody>
         Trans("move-function-body", DescriptionMsg);

void MoveFunctionBody::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
}

void MoveFunctionBody::HandleTopLevelDecl(DeclGroupRef D) 
{
  FunctionDecl *PrevFD = NULL;

  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    FunctionDecl *FD = dyn_cast<FunctionDecl>(*I);
    if (!FD) {
      PrevFD = NULL;
      continue;
    }

    FunctionDecl *CanonicalFD = FD->getCanonicalDecl();
    if (FD->isThisDeclarationADefinition()) {
      FunctionDecl *FDDecl = AllValidFunctionDecls[CanonicalFD];
      if (!FDDecl) {
        PrevFD = NULL;
        continue;
      }

      // Declaration and Definition are next to each other
      if (PrevFD) {
        FunctionDecl *CanonicalPrevFD = PrevFD->getCanonicalDecl();
        if (CanonicalFD == CanonicalPrevFD) {
          PrevFD = NULL;
          continue;
        }
      }

      FuncDeclToFuncDef[FDDecl] = FD;
    }

    PrevFD = FD;
    // We only need the first FunctionDecl
    if (AllValidFunctionDecls[CanonicalFD])
      continue;

    AllValidFunctionDecls[CanonicalFD] = FD;
  }
}
 
void MoveFunctionBody::HandleTranslationUnit(ASTContext &Ctx)
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
  TransAssert(!TheFunctionDecl->isThisDeclarationADefinition() &&
              "Invalid Function Declaration!");
  TransAssert(TheFunctionDef && "NULL TheFunctionDef!");
  TransAssert(TheFunctionDef->isThisDeclarationADefinition() &&
              "Invalid Function Definition!");

  doRewriting();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void MoveFunctionBody::doAnalysis(void)
{
  for (FuncDeclToFuncDeclMap::iterator I = FuncDeclToFuncDef.begin(),
       E = FuncDeclToFuncDef.end(); I != E; ++I) {
    ValidInstanceNum++;

    if (ValidInstanceNum == TransformationCounter) {
      TheFunctionDecl = (*I).first;
      TheFunctionDef = (*I).second;
    }
  }
}

void MoveFunctionBody::doRewriting(void)
{
  std::string FuncDefStr;
  RewriteHelper->getFunctionDefStrAndRemove(TheFunctionDef, FuncDefStr);
  RewriteHelper->addStringAfterFuncDecl(TheFunctionDecl, FuncDefStr);
}

MoveFunctionBody::~MoveFunctionBody(void)
{
  // Nothing to do
}

