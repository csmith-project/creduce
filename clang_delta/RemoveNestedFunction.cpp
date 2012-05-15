//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "RemoveNestedFunction.h"

#include <sstream>

#include "llvm/ADT/SmallVector.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"
#include "CommonStatementVisitor.h"

using namespace clang;

static const char *DescriptionMsg =
"Remove a nested function invocation from its enclosing \
expression. The transformation will create a temporary \
variable with the correct type, assgin the return value \
of the selected nested function to the created variable, \
and replace the function invocation with this temporary \
variable. (Note that this transformation is unsound because \
it changes the semantics of loop executions in some cases. \n";


static RegisterTransformation<RemoveNestedFunction>
         Trans("remove-nested-function", DescriptionMsg);

class RNFCollectionVisitor : public RecursiveASTVisitor<RNFCollectionVisitor> {
public:

  explicit RNFCollectionVisitor(RemoveNestedFunction *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitFunctionDecl(FunctionDecl *FD);

private:
  RemoveNestedFunction *ConsumerInstance;
  
};

class RNFStatementVisitor : public CommonStatementVisitor<RNFStatementVisitor> {
public:

  explicit RNFStatementVisitor(RemoveNestedFunction *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitCallExpr(CallExpr *CallE);

private:
  RemoveNestedFunction *ConsumerInstance;

};

bool RNFCollectionVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  if (!FD->isThisDeclarationADefinition())
    return true;

  ConsumerInstance->StmtVisitor->setCurrentFunctionDecl(FD);
  ConsumerInstance->StmtVisitor->TraverseDecl(FD);
  ConsumerInstance->StmtVisitor->setCurrentFunctionDecl(NULL);
  return true;
}

bool RNFStatementVisitor::VisitCallExpr(CallExpr *CallE) 
{
  if ((std::find(ConsumerInstance->ValidCallExprs.begin(), 
                 ConsumerInstance->ValidCallExprs.end(), CallE) 
          == ConsumerInstance->ValidCallExprs.end()) && 
      !ConsumerInstance->CallExprQueue.empty()) {

    ConsumerInstance->ValidInstanceNum++;
    ConsumerInstance->ValidCallExprs.push_back(CallE);

    if (ConsumerInstance->ValidInstanceNum == 
        ConsumerInstance->TransformationCounter) {
      ConsumerInstance->TheFuncDecl = CurrentFuncDecl;
      ConsumerInstance->TheStmt = CurrentStmt;
      ConsumerInstance->TheCallExpr = CallE;
      ConsumerInstance->NeedParen = NeedParen;
    }
  }

  ConsumerInstance->CallExprQueue.push_back(CallE);

  for (CallExpr::arg_iterator I = CallE->arg_begin(),
       E = CallE->arg_end(); I != E; ++I) {
      TraverseStmt(*I);
  }

  ConsumerInstance->CallExprQueue.pop_back();

  return true;
}

void RemoveNestedFunction::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  NestedInvocationVisitor = new RNFCollectionVisitor(this);
  StmtVisitor = new RNFStatementVisitor(this);
  NameQueryWrap = 
    new TransNameQueryWrap(RewriteHelper->getTmpVarNamePrefix());
}

bool RemoveNestedFunction::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    NestedInvocationVisitor->TraverseDecl(*I);
  }
  return true;
}
 
void RemoveNestedFunction::HandleTranslationUnit(ASTContext &Ctx)
{
  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  TransAssert(TheFuncDecl && "NULL TheFuncDecl!");
  TransAssert(TheStmt && "NULL TheStmt!");
  TransAssert(TheCallExpr && "NULL TheCallExpr");

  NameQueryWrap->TraverseDecl(Ctx.getTranslationUnitDecl());

  addNewTmpVariable();
  addNewAssignStmt();
  replaceCallExpr();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

bool RemoveNestedFunction::addNewTmpVariable(void)
{
  std::string VarStr;
  std::stringstream SS;
  unsigned int NamePostfix = NameQueryWrap->getMaxNamePostfix();

  SS << RewriteHelper->getTmpVarNamePrefix() << (NamePostfix + 1);
  VarStr = SS.str();
  setTmpVarName(VarStr);

  QualType QT;
  const Expr *E = TheCallExpr->getCallee();

  // ISSUE: also handle UnresolvedMemberExpr? 
  if (const UnresolvedLookupExpr *UE = dyn_cast<UnresolvedLookupExpr>(E)) {
    // clang doesn't always resolve CallExpr's callee. For example:
    //   template<typename T> int foo1(int p) {return p;}
    //   template<typename T> int foo2(int p) {return p;}
    //   template<typename T>
    //   void bar(void) { foo1<T>(foo2<T>(1)); }
    // foo2<T>(1) has BuiltinType and hence 
    // TheCallExpr->getCallReturnType() will segfault.
    // In this case, we have to lookup a corresponding function decl

    DeclarationName DName = UE->getName();
    TransAssert((DName.getNameKind() == DeclarationName::Identifier) &&
                "Not an indentifier!");
    const FunctionDecl *FD = NULL;
    if (const NestedNameSpecifier *NNS = UE->getQualifier()) {
      if (const DeclContext *Ctx = getDeclContextFromSpecifier(NNS))
        FD = lookupFunctionDecl(DName, Ctx);
    }
    if (!FD)
      FD = lookupFunctionDecl(DName, TheFuncDecl->getLookupParent());
    TransAssert(FD && "Cannot resolve DName!");
    QT = FD->getResultType();
  }
  else {
    QT = TheCallExpr->getCallReturnType();
  }

  QT.getAsStringInternal(VarStr,
                         Context->getPrintingPolicy());

  VarStr += ";";
  return RewriteHelper->addLocalVarToFunc(VarStr, TheFuncDecl);
}

bool RemoveNestedFunction::addNewAssignStmt(void)
{
  return RewriteHelper->addNewAssignStmtBefore(TheStmt,
                                              getTmpVarName(),
                                              TheCallExpr, 
                                              NeedParen);

}

bool RemoveNestedFunction::replaceCallExpr(void)
{
  return RewriteHelper->replaceExpr(TheCallExpr, TmpVarName);
}

RemoveNestedFunction::~RemoveNestedFunction(void)
{
  if (NestedInvocationVisitor)
    delete NestedInvocationVisitor;

  if (StmtVisitor)
    delete StmtVisitor;

  if (NameQueryWrap)
    delete NameQueryWrap;
}

