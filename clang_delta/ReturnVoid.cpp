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

#include "ReturnVoid.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Make a function return void. \
Only change the prototype of the function and \
delete all return statements in the function, \
but skip the call sites of this function.\n";
 
static RegisterTransformation<ReturnVoid> 
         Trans("return-void", DescriptionMsg);

class RVASTVisitor : public RecursiveASTVisitor<RVASTVisitor> {
public:
  explicit RVASTVisitor(ReturnVoid *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitFunctionDecl(FunctionDecl *FD);

  bool VisitReturnStmt(ReturnStmt *RS);

private:

  ReturnVoid *ConsumerInstance;

  bool rewriteFuncDecl(FunctionDecl *FP);

  bool rewriteReturnStmt(ReturnStmt *RS);

};


class RVCollectionVisitor : public RecursiveASTVisitor<RVCollectionVisitor> {
public:
  explicit RVCollectionVisitor(ReturnVoid *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitFunctionDecl(FunctionDecl *FD);

private:

  ReturnVoid *ConsumerInstance;
};

bool RVCollectionVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  FunctionDecl *CanonicalDecl = FD->getCanonicalDecl();
  if (ConsumerInstance->isNonVoidReturnFunction(CanonicalDecl)) {
    ConsumerInstance->ValidInstanceNum++;
    ConsumerInstance->ValidFuncDecls.push_back(CanonicalDecl);

    if (ConsumerInstance->ValidInstanceNum == 
        ConsumerInstance->TransformationCounter)
      ConsumerInstance->TheFuncDecl = CanonicalDecl;
  }

  if ((ConsumerInstance->TheFuncDecl == CanonicalDecl) && 
       FD->isThisDeclarationADefinition())
    ConsumerInstance->keepFuncDefRange(FD);

  return true;
}

void ReturnVoid::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new RVCollectionVisitor(this);
  TransformationASTVisitor = new RVASTVisitor(this);
}

bool ReturnVoid::isNonVoidReturnFunction(FunctionDecl *FD)
{
  // Avoid duplications
  if (std::find(ValidFuncDecls.begin(), 
                ValidFuncDecls.end(), FD) != 
      ValidFuncDecls.end())
    return false;

  // this function happen to have a library function, e.g. strcpy,
  // then the type source info won't be available, let's try to
  // get one from the one which is in the source
  if (!FD->getTypeSourceInfo()) {
    const FunctionDecl *FirstFD = FD->getCanonicalDecl();
    FD = NULL;
    for (FunctionDecl::redecl_iterator I = FirstFD->redecls_begin(), 
         E = FirstFD->redecls_end(); I != E; ++I) {
      if ((*I)->getTypeSourceInfo()) {
        FD = (*I);
        break;
      }
    }
    if (!FD)
      return false;
  }
  TypeLoc TLoc = FD->getTypeSourceInfo()->getTypeLoc();
  SourceLocation SLoc = TLoc.getBeginLoc();
  if (SLoc.isInvalid())
    return false;
  QualType RVType = FD->getResultType();
  return !(RVType.getTypePtr()->isVoidType());
}

void ReturnVoid::keepFuncDefRange(FunctionDecl *FD)
{
  TransAssert(!FuncDefStartPos && !FuncDefEndPos && 
         "Duplicated function definition?");

  SourceRange FuncDefRange = FD->getSourceRange();

  SourceLocation StartLoc = FuncDefRange.getBegin();
  FuncDefStartPos = 
      SrcManager->getCharacterData(StartLoc);

  SourceLocation EndLoc = FuncDefRange.getEnd();
  FuncDefEndPos = 
      SrcManager->getCharacterData(EndLoc);
}

bool ReturnVoid::isInTheFuncDef(ReturnStmt *RS)
{
  // The candidate function doesn't have a body
  if (!FuncDefStartPos)
    return false;

  SourceRange RSRange = RS->getSourceRange();

  SourceLocation StartLoc = RSRange.getBegin();
  SourceLocation EndLoc = RSRange.getEnd();
  const char *StartPos =
      SrcManager->getCharacterData(StartLoc);
  const char *EndPos =   
      SrcManager->getCharacterData(EndLoc);
  (void)EndPos;

  if ((StartPos > FuncDefStartPos) && (StartPos < FuncDefEndPos)) {
    TransAssert((EndPos > FuncDefStartPos) && (EndPos < FuncDefEndPos) && 
            "Bad return statement range!");
    return true;
  }

  return false;
}

void ReturnVoid::HandleTranslationUnit(ASTContext &Ctx)
{
  CollectionVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  TransAssert(TransformationASTVisitor && "NULL TransformationASTVisitor!");
  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);
  TransAssert(TheFuncDecl && "NULL TheFuncDecl!");

  TransformationASTVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (!Rewritten) {
    TransError = TransNoTextModificationError;
    return;
  }
  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

bool RVASTVisitor::rewriteFuncDecl(FunctionDecl *FD)
{
  DeclarationNameInfo NameInfo = FD->getNameInfo();
  SourceLocation NameInfoStartLoc = NameInfo.getBeginLoc();

  SourceRange FuncDefRange = FD->getSourceRange();
  SourceLocation FuncStartLoc = FuncDefRange.getBegin();
  
  const char *FuncStartBuf =
      ConsumerInstance->SrcManager->getCharacterData(FuncStartLoc);
  const char *NameInfoStartBuf =
      ConsumerInstance->SrcManager->getCharacterData(NameInfoStartLoc);
  if (FuncStartBuf == NameInfoStartBuf)
    return true;

  int Offset = NameInfoStartBuf - FuncStartBuf;

  NameInfoStartBuf--;
  while ((*NameInfoStartBuf == '(') || (*NameInfoStartBuf == ' ') ||
         (*NameInfoStartBuf == '\t') || (*NameInfoStartBuf == '\n')) {
    Offset--;
    NameInfoStartBuf--;
  }

  TransAssert(Offset >= 0);
  ConsumerInstance->Rewritten = true;
  return !(ConsumerInstance->TheRewriter.ReplaceText(FuncStartLoc, 
                 Offset, "void "));
}

bool RVASTVisitor::rewriteReturnStmt(ReturnStmt *RS)
{
  // SourceRange RSRange = RS->getSourceRange();

  // return !(ConsumerInstance->TheRewriter.ReplaceText(RSRange, "return"));

  // Instead of replace an entire ReturnStmt with return, let's keep Ret Expr.
  // The reason is that RetExpr could have side-effects and these side-effects
  // could cause bugs. But we still could remove "return" keyword

  ConsumerInstance->Rewritten = true;
  SourceLocation Loc = RS->getReturnLoc();
  return !(ConsumerInstance->TheRewriter.RemoveText(Loc, 6));
}

bool RVASTVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  FunctionDecl *CanonicalFD = FD->getCanonicalDecl();

  if (CanonicalFD == ConsumerInstance->TheFuncDecl)
    return rewriteFuncDecl(FD);

  return true;
}

bool RVASTVisitor::VisitReturnStmt(ReturnStmt *RS)
{
  if (ConsumerInstance->isInTheFuncDef(RS))
    return rewriteReturnStmt(RS);

  return true;
}

ReturnVoid::~ReturnVoid(void)
{
  delete CollectionVisitor;
  delete TransformationASTVisitor;
}

