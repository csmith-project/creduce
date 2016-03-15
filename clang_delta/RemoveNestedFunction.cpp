//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2014, 2015, 2016 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "RemoveNestedFunction.h"

#include <sstream>

#include "llvm/ADT/SmallVector.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
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

  bool VisitStmtExpr(StmtExpr *SE);

private:
  RemoveNestedFunction *ConsumerInstance;

};

bool RNFCollectionVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  if (ConsumerInstance->isInIncludedFile(FD) ||
      !FD->isThisDeclarationADefinition())
    return true;

  ConsumerInstance->StmtVisitor->setCurrentFunctionDecl(FD);
  ConsumerInstance->StmtVisitor->TraverseDecl(FD);
  ConsumerInstance->StmtVisitor->setCurrentFunctionDecl(NULL);
  return true;
}

bool RNFStatementVisitor::VisitStmtExpr(StmtExpr *SE)
{
  CompoundStmt *CS = SE->getSubStmt();
  if (ConsumerInstance->CallExprQueue.empty()) {
    TraverseStmt(CS);
    return false;
  }

  CallExpr *CallE = ConsumerInstance->CallExprQueue.back();
  CurrentStmt = CallE;

  for (clang::CompoundStmt::body_iterator I = CS->body_begin(),
       E = CS->body_end(); I != E; ++I) {
    TraverseStmt(*I);
  }
  return false;
}

bool RNFStatementVisitor::VisitCallExpr(CallExpr *CallE)
{
  if (const CXXOperatorCallExpr *OpE = dyn_cast<CXXOperatorCallExpr>(CallE)) {
    if (ConsumerInstance->isInvalidOperator(OpE))
      return true;
  }

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
    Expr *Exp = *I;
    TraverseStmt(Exp);
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

  addNewTmpVariable(Ctx);

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void RemoveNestedFunction::getVarStrForTemplateSpecialization(
       std::string &VarStr,
       const TemplateSpecializationType *TST)
{
  unsigned NumArgs = TST->getNumArgs();
  if (NumArgs == 0) {
    return;
  }

  std::string ArgStr;
  llvm::raw_string_ostream Stream(ArgStr);
  TST->getArg(0).print(Context->getPrintingPolicy(), Stream);

  for (unsigned I = 1; I < NumArgs; ++I) {
    const TemplateArgument &Arg = TST->getArg(I);
    Stream << ", ";
    Arg.print(Context->getPrintingPolicy(), Stream);
  }
  size_t BeginPos = VarStr.find_first_of('<');
  size_t EndPos = VarStr.find_last_of('>');
  TransAssert((BeginPos != std::string::npos) && "Cannot find < !");
  TransAssert((EndPos != std::string::npos) && "Cannot find > !");
  TransAssert((EndPos > BeginPos) && "Invalid <> pair!");
  VarStr.replace(BeginPos + 1, (EndPos - BeginPos - 1), Stream.str());
}

void RemoveNestedFunction::getNewTmpVariable(const QualType &QT,
                                             std::string &VarStr)
{
  QT.getAsStringInternal(VarStr, Context->getPrintingPolicy());
}

void RemoveNestedFunction::getNewIntTmpVariable(std::string &VarStr)
{
  VarStr = "int " + VarStr;
}

void RemoveNestedFunction::addNewTmpVariable(ASTContext &ASTCtx)
{
  std::string VarStr;

  getNewTmpVariableStr(ASTCtx, VarStr);
  if (TransformationManager::isCXXLangOpt()) {
    // TheStmt and TheCallExpr may share the same start location, e.g..
    // TheCallExpr is a CXXOperatorCallExpr. In this case, we just replace
    // TheCallExpr with tmp variable's definition and the tmp variable.
    // Otherwise, we would end up with assertion failure, because we
    // modify the same location twice (through addnewAssignStmtBefore
    // and replaceExpr.
    if (TheStmt->getLocStart() == TheCallExpr->getLocStart()) {
      std::string ExprStr;
      RewriteHelper->getExprString(TheCallExpr, ExprStr);
      VarStr += " = " + ExprStr + ";\n" + TmpVarName;
      RewriteHelper->replaceExpr(TheCallExpr, VarStr);
      return;
    }
    RewriteHelper->addNewAssignStmtBefore(TheStmt, VarStr,
                                          TheCallExpr, NeedParen);
  }
  else {
    RewriteHelper->addLocalVarToFunc(VarStr + ";", TheFuncDecl);
    RewriteHelper->addNewAssignStmtBefore(TheStmt, getTmpVarName(),
                                          TheCallExpr, NeedParen);
  }
  RewriteHelper->replaceExpr(TheCallExpr, TmpVarName);
}

void RemoveNestedFunction::getNewTmpVariableStr(ASTContext &ASTCtx,
                                                std::string &VarStr)
{
  std::stringstream SS;
  unsigned int NamePostfix = NameQueryWrap->getMaxNamePostfix();

  SS << RewriteHelper->getTmpVarNamePrefix() << (NamePostfix + 1);
  VarStr = SS.str();
  setTmpVarName(VarStr);

  QualType QT;
  const Expr *E = TheCallExpr->getCallee();

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
    TransAssert(((DName.getNameKind() == DeclarationName::Identifier) ||
                 (DName.getNameKind() == DeclarationName::CXXOperatorName)) &&
                "Not an indentifier!");
    const FunctionDecl *FD = NULL;
    if (const NestedNameSpecifier *NNS = UE->getQualifier()) {
      if (const DeclContext *Ctx = getDeclContextFromSpecifier(NNS)) {
        DeclContextSet VisitedCtxs;
        FD = lookupFunctionDecl(DName, Ctx, VisitedCtxs);
      }
    }
    if (!FD) {
      DeclContextSet VisitedCtxs;
      FD =
        lookupFunctionDecl(DName, TheFuncDecl->getLookupParent(), VisitedCtxs);
    }
    // give up and generate a tmp var of int type, e.g.:
    // template <class T> struct S {
    //   T x;
    //   template <class A> void foo(A &a0) { x(y(a0)); }
    // };
    if (!FD)
      return getNewIntTmpVariable(VarStr);

    QT = FD->getReturnType();
    //FIXME: This is actually not quite correct, we should get the instantiated
    // type here.
    return getNewTmpVariable(QT, VarStr);
  }

  if (const UnresolvedMemberExpr *UM = dyn_cast<UnresolvedMemberExpr>(E)) {
    DeclarationName DName = UM->getMemberName();
    CXXRecordDecl *CXXRD = UM->getNamingClass();
    DeclContextSet VisitedCtxs;
    const FunctionDecl *FD = lookupFunctionDecl(DName, CXXRD, VisitedCtxs);
    // FIXME: try to resolve FD here
    if (FD)
      QT = FD->getReturnType();
    return getNewTmpVariable(QT, VarStr);
  }

  if (const CXXTemporaryObjectExpr *CXXTE =
      dyn_cast<CXXTemporaryObjectExpr>(E)) {
    const CXXConstructorDecl *CXXCtor = CXXTE->getConstructor();
    QT = CXXCtor->getThisType(ASTCtx);
    return getNewTmpVariable(QT, VarStr);
  }

  if (const CXXTemporaryObjectExpr *CXXTE =
      dyn_cast<CXXTemporaryObjectExpr>(E)) {
    const CXXConstructorDecl *CXXCtor = CXXTE->getConstructor();
    QT = CXXCtor->getThisType(ASTCtx);
    return getNewTmpVariable(QT, VarStr);
  }

  if (const CXXDependentScopeMemberExpr *ME =
      dyn_cast<CXXDependentScopeMemberExpr>(E)) {

    if (ME->isImplicitAccess())
      return;
    DeclarationName DName = ME->getMember();
    TransAssert((DName.getNameKind() == DeclarationName::Identifier) &&
                "Not an indentifier!");
    const Expr *E = ME->getBase();
    TransAssert(E && "NULL Base Expr!");
    const Expr *BaseE = E->IgnoreParens();

    // handle cases where base expr or member name is dependent, e.g.,
    // template<typename T>
    // class S {
    //   int f1(int p1) { return p1; };
    //   int f2(int p2) { return p2; };
    //   void f3(void);
    // };
    // template<typename T>
    // void S<T>::f3(void)
    // {
    //   f1(this->f2(1));
    // }
    // where this->f2(1) is a CXXDependentScopeMemberExpr
    if (dyn_cast<CXXThisExpr>(BaseE)) {
      const DeclContext *Ctx = TheFuncDecl->getLookupParent();
      TransAssert(Ctx && "Bad DeclContext!");
      DeclContextSet VisitedCtxs;
      const FunctionDecl *FD = lookupFunctionDecl(DName, Ctx, VisitedCtxs);
      TransAssert(FD && "Cannot resolve DName!");
      QT = FD->getReturnType();
      return getNewTmpVariable(QT, VarStr);
    }

    // handle other cases where lookupDeclContext is different from
    // the current CXXRecord, e.g.,
    const Type *Ty = ME->getBaseType().getTypePtr();
    if (const DeclContext *Ctx = getBaseDeclFromType(Ty)) {
      DeclContextSet VisitedCtxs;
      const FunctionDecl *FD = lookupFunctionDecl(DName, Ctx, VisitedCtxs);
      if (!FD) {
        return getNewTmpVariable(QT, VarStr);
      }
      QT = FD->getReturnType();
      const Type *RVTy = QT.getTypePtr();
      if (RVTy->getAs<InjectedClassNameType>()) {
        // handle cases like:
        // template <typename> struct D {
        //   D f();
        // };
        // template <typename T> void foo(D<T>);
        // template <typename T > void bar () {
        //   D<T> G;
        //   foo(G.f());
        // }
        // in this case, seems it's hard to retrieve the instantiated type
        // of f's return type, because `D<T> G' is dependent. I tried
        // findSpecialization from ClassTemplateDecl, but it didn't work.
        // So use a really ugly way, i.e., manipulating strings...
        const TemplateSpecializationType *TST =
          Ty->getAs<TemplateSpecializationType>();
        TransAssert(TST && "Invalid TemplateSpecialization Type!");

        QT.getAsStringInternal(VarStr,
                               Context->getPrintingPolicy());
        return getVarStrForTemplateSpecialization(VarStr, TST);
      }
      else {
        // other cases:
        // template <typename> struct D {
        //   int f();
        // };
        // void foo(int);
        // template <typename T > void bar () {
        //   D<T> G;
        //   foo(G.f());
        // }
        return getNewTmpVariable(QT, VarStr);
      }
    }
    else {
      // template <typename> struct D {
      // D f();
      // D operator[] (int);
      // };
      // template <typename T> void foo(D<T>);
      // template <typename T > void bar () {
      //   D<T> G;
      //   foo(G[0].f());
      // }
      // In this case, G[0] is of BuiltinType.
      // But why does clang represent a dependent type as BuiltinType here?

      TransAssert((Ty->getAs<BuiltinType>() ||
                   Ty->getAs<TemplateTypeParmType>() ||
                   Ty->getAs<TypedefType>() ||
                   Ty->getAs<DependentNameType>())
                  && "Uncaught Type");
      // FIXME: This is incorrect!
      // a couple of questions
      //  - how can we find a correct DeclContext where we could lookup f?
      //  - can we obtain the dependent template argument from BuiltinType?
      // Probably we cannot do these? Comments from lib/AST/ASTContext.cpp:
      //
      // Placeholder type for type-dependent expressions whose type is
      // completely unknown. No code should ever check a type against
      // DependentTy and users should never see it; however, it is here to
      // help diagnose failures to properly check for type-dependent
      // expressions.
      return getNewIntTmpVariable(VarStr);
    }
  }

  const Type *CalleeType = E->getType().getTypePtr();
  // template <class T1, class T2> struct S {
  //   T1 x; T2 y;
  //   template <class A> void foo(A &a0) { x(y(a0)); }
  // };
  if (const TemplateTypeParmType *PT =
      dyn_cast<TemplateTypeParmType>(CalleeType)) {
    const TemplateTypeParmDecl *PD = PT->getDecl();
    std::string DStr = PD->getNameAsString();
    VarStr = DStr + " " + VarStr;
    return;
  }
  QT = TheCallExpr->getCallReturnType(ASTCtx);
  getNewTmpVariable(
    QT.getTypePtr()->getUnqualifiedDesugaredType()->getCanonicalTypeInternal(),
    VarStr);
}

bool RemoveNestedFunction::isInvalidOperator(const CXXOperatorCallExpr *OpE)
{
  OverloadedOperatorKind K = OpE->getOperator();
  // ISSUE: overloaded Equal-family operators cause bad recursion,
  //        omit it for now.
  switch (K) {
  case OO_Equal:
  case OO_EqualEqual:
  case OO_PlusEqual:
  case OO_MinusEqual:
  case OO_StarEqual:
  case OO_SlashEqual:
  case OO_PercentEqual:
  case OO_CaretEqual:
  case OO_AmpEqual:
  case OO_PipeEqual:
  case OO_LessLessEqual:
  case OO_GreaterGreaterEqual:
  case OO_ExclaimEqual:
  case OO_LessEqual:
  case OO_LessLess:
  case OO_GreaterEqual: // Fall-through
    return true;
  default:
    return false;
  }
}

RemoveNestedFunction::~RemoveNestedFunction(void)
{
  delete NestedInvocationVisitor;
  delete StmtVisitor;
  delete NameQueryWrap;
}

