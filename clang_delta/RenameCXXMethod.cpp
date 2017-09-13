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

#include "RenameCXXMethod.h"

#include <sstream>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Another pass to increase readability of reduced code. \
It renames CXX methods names to m_fn1, m_fn2, ...\n";

static RegisterTransformation<RenameCXXMethod>
         Trans("rename-cxx-method", DescriptionMsg);

class RenameCXXMethodCollectionVisitor : public 
  RecursiveASTVisitor<RenameCXXMethodCollectionVisitor> {

public:

  explicit RenameCXXMethodCollectionVisitor(RenameCXXMethod *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitCXXRecordDecl(CXXRecordDecl *FD);

  bool VisitCXXMethodDecl(CXXMethodDecl *MD);

private:

  RenameCXXMethod *ConsumerInstance;

};

// ISSUE:
// we can't handle the case below:
// struct A {
//   int abc;
//   static int test(int);
//   enum {
//     value = (1 == sizeof(test(abc)))
//   };
// };
// The reason is that somehow expr (1== sizeof(test(abc)))
// is not presented in Clang's AST representation.
class RenameCXXMethodVisitor : 
        public RecursiveASTVisitor<RenameCXXMethodVisitor> {

public:

  explicit RenameCXXMethodVisitor(RenameCXXMethod *Instance)
    : ConsumerInstance(Instance)
  { }

  bool TraverseClassTemplateDecl(ClassTemplateDecl *D);

  bool VisitCXXMethodDecl(CXXMethodDecl *MD);

  bool VisitFunctionDecl(FunctionDecl *FD);

  bool VisitCXXRecordDecl(CXXRecordDecl *FD);

  bool VisitDeclRefExpr(DeclRefExpr *DRE);

  bool VisitMemberExpr(MemberExpr *ME);

  bool VisitCallExpr(CallExpr *CE);

  // we have to enable visiting template instantiations, otherwise,
  // we won't be able to see some explicit instantiation code, e.g.:
  // template<typename T> class A {
  //   T* foo(T* __p) <--- foo would be missing
  //   { __p; }
  // };
  // extern template class basic_string<char>;
  bool shouldVisitTemplateInstantiations() {
    return true;
  }

private:
  void TraverseClassInstantiations(ClassTemplateDecl *D);

  RenameCXXMethod *ConsumerInstance;
};

bool RenameCXXMethodCollectionVisitor::VisitCXXRecordDecl(CXXRecordDecl *RD)
{
  if (ConsumerInstance->isInIncludedFile(RD) || !RD->hasDefinition())
    return true;
  const CXXRecordDecl *RDDef = RD->getDefinition();
  ConsumerInstance->handleOneCXXRecordDecl(RDDef);
  return true;
}

bool RenameCXXMethodCollectionVisitor::VisitCXXMethodDecl(CXXMethodDecl *MD)
{
  if (ConsumerInstance->isInIncludedFile(MD))
    return true;
  const CXXMethodDecl *CanonicalMD = MD->getCanonicalDecl();
  if(ConsumerInstance->NewMethodNames.find(CanonicalMD) != 
       ConsumerInstance->NewMethodNames.end())
    return true;
  
  ConsumerInstance->handleOneMemberTemplateFunction(CanonicalMD);
  return true;
}

bool RenameCXXMethodVisitor::VisitCXXRecordDecl(CXXRecordDecl *RD)
{
  ConsumerInstance->setClassInstantiationFlag(RD);
  ConsumerInstance->clearFunctionInstantiationFlag();
  return true;
}

bool RenameCXXMethodVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  ConsumerInstance->CurrentFD = FD;
  const CXXMethodDecl *MD = dyn_cast<CXXMethodDecl>(FD);
  if (!MD) {
    ConsumerInstance->setFunctionInstantiationFlag(FD);
    ConsumerInstance->clearClassInstantiationFlag();
  }
  return true;
}

void RenameCXXMethodVisitor::TraverseClassInstantiations(
       ClassTemplateDecl *D)
{
  for (ClassTemplateDecl::spec_iterator I = D->spec_begin(),
       E = D->spec_end(); I != E; ++I) {
    ClassTemplateSpecializationDecl* SpecD = (*I);

    switch (SpecD->getSpecializationKind()) {
    case TSK_Undeclared:
    case TSK_ImplicitInstantiation:
      // Keep track if we are inside class instantiations.
      // We use a queue because we could have nested CXXRecord definition
      // and implicit instantiations
      ConsumerInstance->InstantiationQueue.push_back(SpecD);
      TraverseDecl(SpecD);
      ConsumerInstance->InstantiationQueue.pop_back();
      break;

    default:
    // TSK_ExplicitInstantiationDeclaration:
    // TSK_ExplicitInstantiationDefinition:
    // TSK_ExplicitSpecialization:
      break;
    }
  }
}

bool RenameCXXMethodVisitor::TraverseClassTemplateDecl(ClassTemplateDecl *D)
{
  CXXRecordDecl* TmplDecl = D->getTemplatedDecl();
  if (TmplDecl)
    TraverseDecl(TmplDecl);
  if (TemplateParameterList *TmplList = D->getTemplateParameters()) {
    for (TemplateParameterList::iterator I = TmplList->begin(), 
         E = TmplList->end(); I != E; ++I) {
      TraverseDecl(*I);
    }
  }

  if (D != D->getCanonicalDecl())
    return true;

  TraverseClassInstantiations(D);
  return true;
}

bool RenameCXXMethodVisitor::VisitCXXMethodDecl(CXXMethodDecl *MD)
{
  if (ConsumerInstance->isSpecialCXXMethod(MD) ||
      ConsumerInstance->isInIncludedFile(MD))
    return true;

  const CXXMethodDecl *CanonicalMD = MD->getCanonicalDecl();
  llvm::DenseMap<const CXXMethodDecl *, std::string>::iterator I = 
    ConsumerInstance->NewMethodNames.find(CanonicalMD);
  if (I != ConsumerInstance->NewMethodNames.end()) {
    ConsumerInstance->RewriteHelper->replaceFunctionDeclName(MD, (*I).second);
    return true;
  }
  
  TemplateSpecializationKind K = MD->getTemplateSpecializationKind();
  if (const FunctionDecl *MemberFD = MD->getInstantiatedFromMemberFunction()) {
    if (K != TSK_ExplicitSpecialization)
      return true;
    const CXXMethodDecl *MemberMD = dyn_cast<CXXMethodDecl>(MemberFD);
    TransAssert(MemberMD && "Invalid Member FD!");

    if (ConsumerInstance->VisitedSpecializedMethods.count(
          MD->getCanonicalDecl()))
      return true;

    ConsumerInstance->VisitedSpecializedMethods.insert(
        MD->getCanonicalDecl());
    CanonicalMD = MemberMD->getCanonicalDecl();
    I = ConsumerInstance->NewMethodNames.find(CanonicalMD);
    TransAssert((I != ConsumerInstance->NewMethodNames.end()) &&
                "Cannot find member function!");
    ConsumerInstance->RewriteHelper->replaceFunctionDeclName(MD, (*I).second);
    return true;
  }

  // only need to rewrite explicit instantiations
  if ((K == TSK_ExplicitInstantiationDeclaration) ||
      (K == TSK_ExplicitInstantiationDefinition)) {
    ConsumerInstance->rewriteFunctionTemplateExplicitInstantiation(MD);
  }

  return true;
}

bool RenameCXXMethodVisitor::VisitDeclRefExpr(DeclRefExpr *DRE)
{
  const ValueDecl *VD = DRE->getDecl();
  const CXXMethodDecl *MD = dyn_cast<CXXMethodDecl>(VD);
  if (!MD)
    return true;

  if (!ConsumerInstance->isExplicit(MD))
      return true;

  std::string NewName = "";
  if (!ConsumerInstance->getMethodNewName(MD, NewName))
    return true;
  TransAssert((NewName != "") && "Bad new name!");

  if (DRE->hasQualifier()) {
    NestedNameSpecifierLoc QualLoc = DRE->getQualifierLoc();
    ConsumerInstance->RewriteHelper->replaceCXXMethodNameAfterQualifier(
      &QualLoc, MD, NewName);
  }
  else {
    ConsumerInstance->TheRewriter.ReplaceText(DRE->getLocStart(),
      MD->getNameAsString().size(), NewName);
  }

  return true;
}

bool RenameCXXMethodVisitor::VisitMemberExpr(MemberExpr *ME)
{
  const ValueDecl *VD = ME->getMemberDecl();
  const CXXMethodDecl *MD = dyn_cast<CXXMethodDecl>(VD);
  if (!MD)
    return true;
  if (!ConsumerInstance->isExplicit(MD)) {
      return true;
  }
  std::string NewName = "";
  if (!ConsumerInstance->getMethodNewName(MD, NewName))
    return true;
  TransAssert((NewName != "") && "Bad new name!");

  if (ME->hasQualifier()) {
    NestedNameSpecifierLoc QualLoc = ME->getQualifierLoc();
    ConsumerInstance->RewriteHelper->replaceCXXMethodNameAfterQualifier(
      &QualLoc, MD, NewName);
  }
  else {
    ConsumerInstance->TheRewriter.ReplaceText(ME->getMemberLoc(),
      MD->getNameAsString().size(), NewName);
  }

  return true;
}

bool RenameCXXMethodVisitor::VisitCallExpr(CallExpr *CE)
{
  // handled by VisitMemberExpr
  if (CE->getDirectCallee())
    return true;

  // deal with UnresolvedLookupExpr
  const Expr *E = CE->getCallee();
  ConsumerInstance->rewriteDependentExpr(E);
  return true; 
}

void RenameCXXMethod::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  MethodCollectionVisitor = new RenameCXXMethodCollectionVisitor(this);
  RenameVisitor = new RenameCXXMethodVisitor(this);
  ValidInstanceNum = 1;
}

bool RenameCXXMethod::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    MethodCollectionVisitor->TraverseDecl(*I);
  }
  return true;
}

void RenameCXXMethod::HandleTranslationUnit(ASTContext &Ctx)
{
  if (QueryInstanceOnly) {
    if (hasValidMethods())
      ValidInstanceNum = 1;
    else
      ValidInstanceNum = 0;
    return;
  }

  if (!hasValidMethods()) {
    TransError = TransNoValidFunsError;
    return;
  }
  else if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  TransAssert(RenameVisitor && "NULL RenameVisitor!");
  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  RenameVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

bool RenameCXXMethod::hasValidMethods()
{
  return ((NewMethodNames.size() != 0) && DoRenaming);
}

// note that we don't handle the possible ambiguity here, e.g.
// class A {
// public:
//   virtual void foo();
// };
// class B {
// public:
//   virtual void foo();
// };
// class C : public A, B {
//   virtual void foo();
// };
// In this example, getNumInheritedFunctions(C) will return
// 2, but it doesn't really matter --- the number 2 is only used
// for numbering non-virtual functions. 
// We are fine unless we don't rename a non-virtual function to
// a possible virtual function, either defined in the same
// class or inherited from a base.
// Furthermore, we don't want to hide any function in 
// the base class through renaming, for example:
// class A {
//   void foo();
// };
// class B : public A {
//   void bar();
// };
//
// the transformation below is bad:
// class A {
//   void m_fn1();
// };
// class B : public A {
//   void m_fn1();
// };
unsigned int RenameCXXMethod::getNumInheritedFunctions(
               const CXXRecordDecl *RD)
{
  TransAssert(RD->isThisDeclarationADefinition() && "Not a definition!");

  unsigned int Num = 0;
  for (CXXRecordDecl::base_class_const_iterator I =
       RD->bases_begin(), E = RD->bases_end(); I != E; ++I) {

    const CXXBaseSpecifier *BS = I;
    const Type *Ty = BS->getType().getTypePtr();
    const CXXRecordDecl *Base = getBaseDeclFromType(Ty);
    if (!Base)
      continue;

    const CXXRecordDecl *CanonicalBase = Base->getCanonicalDecl();
    // e.g., 
    // template<typename T> struct A;
    // template<typename T> struct B : A<T> {
    //    void foo() {}
    // };
    if (!Base->hasDefinition() && Base->getDescribedClassTemplate())
      continue;

    CXXRecordDeclToNumMap::iterator NI = 
      NumMemberFunctions.find(CanonicalBase);
    if (NI != NumMemberFunctions.end()) {
      Num += NI->second;
      continue;
    }

    if (!Base->hasDefinition())
      continue;
    const CXXRecordDecl *BaseDef = Base->getDefinition();
    handleOneCXXRecordDecl(BaseDef);
    Num += NumMemberFunctions[CanonicalBase];
  }
  return Num; 
}

bool RenameCXXMethod::isValidName(const StringRef &Name)
{
  size_t PrefixLen = MethodNamePrefix.length();
  StringRef NamePrefix = Name.substr(0, PrefixLen);
  if (!NamePrefix.equals(MethodNamePrefix))
    return false;
  llvm::APInt Num;
  return !Name.drop_front(PrefixLen).getAsInteger(10, Num);
}

void RenameCXXMethod::addOneMethodName(const CXXMethodDecl *MD,
                                       unsigned int /*Num*/)
{
  const CXXMethodDecl *CanonicalMD = MD->getCanonicalDecl();
  TransAssert((NewMethodNames.find(CanonicalMD) == NewMethodNames.end()) &&
              "Duplicate CXXMethodDecl!");
  std::stringstream SS;
  // SS << MethodNamePrefix << Num;
  NumRenamedMethods++;
  SS << MethodNamePrefix << NumRenamedMethods;
  NewMethodNames[CanonicalMD] = SS.str();

  // Now we check if the old name actually has a valid format, i.e. m_fn([0-9]+)
  // if not, set DoRenaming to be false, then we will need to
  // do renaming later.
  if (!isValidName(CanonicalMD->getNameAsString()))
    DoRenaming = true;
}

void RenameCXXMethod::addOneInheritedName(const CXXMethodDecl *MD,
                                          const CXXMethodDecl *BaseMD)
{
  const CXXMethodDecl *CanonicalMD = MD->getCanonicalDecl();
  CXXMethodDeclToNameMap::iterator I = NewMethodNames.find(CanonicalMD);
  (void)I;
  TransAssert((I == NewMethodNames.end()) && "Duplicate CXXMethodDecl!");

  const CXXMethodDecl *CanonicalBaseMD = BaseMD->getCanonicalDecl();
  // MD may be inherited of a member function of a class template
  if (const FunctionDecl *FD = 
        CanonicalBaseMD->getInstantiatedFromMemberFunction()) {
    CanonicalBaseMD = dyn_cast<CXXMethodDecl>(FD);
    TransAssert(CanonicalBaseMD && "bad conversion from FD to MD!");
  }
  CXXMethodDeclToNameMap::iterator BaseI = NewMethodNames.find(CanonicalBaseMD);
  TransAssert((BaseI != NewMethodNames.end()) && 
              "Cannot find base CXXMethodDecl!");
  NewMethodNames[CanonicalMD] = BaseI->second;
}

void RenameCXXMethod::handleOneCXXRecordDecl(const CXXRecordDecl *RD)
{
  TransAssert(RD->isThisDeclarationADefinition() &&
              "Can only handle class definition!");
  if (VisitedCXXRecordDecls.count(RD))
    return;
  VisitedCXXRecordDecls.insert(RD);

  // We need to skip explicit instantiations
  // Note that shouldVisitTemplateInstantiations is false here,
  // so we don't need to worry about implicit instantiations
  if (const ClassTemplateSpecializationDecl *Spec =
      dyn_cast<ClassTemplateSpecializationDecl>(RD)) {
    TemplateSpecializationKind K = Spec->getSpecializationKind();
    if ((K == TSK_ExplicitInstantiationDefinition) ||
        (K == TSK_ExplicitInstantiationDeclaration))
      return;
  }

  unsigned int NumFuns = getNumInheritedFunctions(RD);
  CXXMethodDeclSet NonVirtualFuns;
  for (CXXRecordDecl::method_iterator I = RD->method_begin(),
       E = RD->method_end(); I != E; ++I) {
    const CXXMethodDecl *MD = (*I);

    if (isSpecialCXXMethod(MD))
      continue;
    if (!MD->isVirtual()) {
      NonVirtualFuns.insert(MD);
      continue;
    }

    CXXMethodDecl::method_iterator MI = MD->begin_overridden_methods();
    if (MI == MD->end_overridden_methods()) {
      // new virtual function
      NumFuns++;
      addOneMethodName(MD, NumFuns);
    }
    else {
      // if MD is overidden from any base class,
      // it has been counted, so we don't need to increase NumFuns.
      addOneInheritedName(MD, *MI);
    }
  }
  const CXXRecordDecl *CanonicalRD = RD->getCanonicalDecl();

  for (CXXMethodDeclSet::iterator MI = NonVirtualFuns.begin(),
       ME = NonVirtualFuns.end(); MI != ME; ++MI) {
    NumFuns++;
    const CXXMethodDecl *MD = (*MI);
    addOneMethodName(MD, NumFuns);
  }
  NumMemberFunctions[CanonicalRD] = NumFuns;
}

bool RenameCXXMethod::isSpecialCXXMethod(const CXXMethodDecl *MD)
{
  if (dyn_cast<CXXConstructorDecl>(MD) ||
      dyn_cast<CXXDestructorDecl>(MD) ||
      dyn_cast<CXXConversionDecl>(MD))
    return true;

  if (MD->isUsualDeallocationFunction() ||
      MD->isCopyAssignmentOperator() ||
      MD->isMoveAssignmentOperator() ||
      MD->isLambdaStaticInvoker() ||
      MD->isOverloadedOperator())
    return true;

  return false;
}

// Handle function tempates in a class, which are not iterated throught
// CXXRecordDecl::method_iterator.
// For example:
// class A { template<typename T> void foo(T p) {} };
void RenameCXXMethod::handleOneMemberTemplateFunction(
       const CXXMethodDecl *MD)
{
  // we could have template constructors
  if (isSpecialCXXMethod(MD))
    return;

  const FunctionTemplateDecl *FTD = 
      MD->getDescribedFunctionTemplate();
  if (!FTD) 
    return;
  const CXXRecordDecl *CanonicalRD = MD->getParent()->getCanonicalDecl();

  CXXRecordDeclToNumMap::iterator I = NumMemberFunctions.find(CanonicalRD);
  TransAssert((I != NumMemberFunctions.end()) && "Cannot find class!");
  unsigned int NumFuns = I->second;
  NumFuns++;
  addOneMethodName(MD, NumFuns);
  NumMemberFunctions[CanonicalRD] = NumFuns;
}

// handle UnresolvedLookupExpr:
// class A {
//   template <typename T> static int foo(T, int) {
//      foo<T, 0>(0, 0);
//   }
// };
// or UnresolvedMemberExpr:
// class A {
//   template < typename T > void foo (T) { foo <T> (0); }
// };
void RenameCXXMethod::rewriteOverloadExpr(const OverloadExpr *OE)
{
  const FunctionDecl *FD = getFunctionDeclFromOverloadExpr(OE);
  if (!FD)
    return;
  const CXXMethodDecl *MD = dyn_cast<CXXMethodDecl>(FD);
  TransAssert(MD && "Invalid CXXMethodDecl!");

  std::string NewName = "";
  if (!getMethodNewName(MD, NewName))
    return;
  TransAssert((NewName != "") && "Bad new name!");
  if (OE->getQualifier()) {
    NestedNameSpecifierLoc QualLoc = OE->getQualifierLoc();
    RewriteHelper->replaceCXXMethodNameAfterQualifier(
      &QualLoc, MD, NewName);
  }
  else {
    TheRewriter.ReplaceText(OE->getNameLoc(),
      MD->getNameAsString().size(), NewName);
  }
}

const FunctionDecl* RenameCXXMethod::getFunctionDeclFromOverloadExpr(
        const OverloadExpr *OE)
{
  const CXXRecordDecl *RD = OE->getNamingClass();
  if (!RD)
    return NULL;
  DeclarationName DName = OE->getName();
  TransAssert((DName.getNameKind() == DeclarationName::Identifier) &&
              "Not an indentifier!"); 
  DeclContextSet VisitedCtxs;
  return lookupFunctionDecl(DName, RD, VisitedCtxs);
}

const FunctionDecl* RenameCXXMethod::getFunctionDeclFromType(
        const Type *Ty, DeclarationName &DName)
{
  if (Ty->isPointerType() || Ty->isReferenceType())
    Ty = getBasePointerElemType(Ty);
  const FunctionDecl *FD = NULL;
  if (const CXXRecordDecl *BaseRD = getBaseDeclFromType(Ty)) {
    DeclContextSet VisitedCtxs;
    FD = lookupFunctionDecl(DName, BaseRD, VisitedCtxs);
  }
  return FD;
}

const FunctionDecl* RenameCXXMethod::getFunctionDeclFromOverloadTemplate(
        const CallExpr *CE, const OverloadExpr *OE, DeclarationName &DName)
{
  const FunctionDecl *FD = getFunctionDeclFromOverloadExpr(OE);
  // FD is not necessary a member function
  if (!FD) {
    TransAssert(CurrentFD && "Invalid CurrentFD!");
    const DeclContext *Ctx = CurrentFD->getLookupParent();
    TransAssert(Ctx && "Bad DeclContext!");
    DeclarationName FunName = OE->getName();
    DeclContextSet VisitedCtxs;
    FD = lookupFunctionDecl(FunName, Ctx, VisitedCtxs);
    if (!FD)
      return NULL;
  }

  const Type *Ty = FD->getReturnType().getTypePtr();
  return getFunctionDeclFromType(Ty, DName);
}

const FunctionDecl* RenameCXXMethod::getFunctionDeclFromReturnType(
        const CallExpr *CE, DeclarationName &DName)
{
  const Expr *CalleeExpr = CE->getCallee();
  TransAssert(CalleeExpr && "NULL CalleeExpr!");

  // template<typename T> class A {
  //   struct B{
  //     static B& bar() {}
  //   };
  //   B& foo() {}
  //   void baz() {
  //     foo().bar();
  //   }
  // };
  // get the return type of foo().
  if (const MemberExpr *ME = dyn_cast<MemberExpr>(CalleeExpr)) {
    const ValueDecl *VD = ME->getMemberDecl();
    const CXXMethodDecl *MD = dyn_cast<CXXMethodDecl>(VD);
    if (!MD)
      return NULL;
    const Type *Ty = MD->getReturnType().getTypePtr();
    return getFunctionDeclFromType(Ty, DName);
  }
  else if (const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(CalleeExpr)) {
    const ValueDecl *VD = DRE->getDecl();
    const CXXMethodDecl *MD = dyn_cast<CXXMethodDecl>(VD);
    if (!MD)
      return NULL;
    const Type *Ty = MD->getReturnType().getTypePtr();
    return getFunctionDeclFromType(Ty, DName);
  }
  else if (const OverloadExpr *OE = dyn_cast<OverloadExpr>(CalleeExpr)) {
    return getFunctionDeclFromOverloadTemplate(CE, OE, DName);
  }
  else if (const CXXDependentScopeMemberExpr *DE =
           dyn_cast<CXXDependentScopeMemberExpr>(CalleeExpr)) {
    // template <typename T> class A {
    // public:
    //   T foo();
    // };
    // template <typename T> class B {
    // public:
    //   A<T> a;
    //   void bar();
    //   A<T> &return_a() { return a; }
    // };
    // template <typename T> void bar() {
    //   B<T> b;
    //   T xx = b.return_a().foo();
    // }
    // CallExpr could be invoked via CXXDependentScopeMemberExpr,
    // then we get the called function (e.g., return_a) via looking up
    // the base(e.g., b) of CXXDependentScopeMemberExpr
    const FunctionDecl *FD = getFunctionDecl(DE);
    if (!FD)
      return NULL;
    const Type *Ty = FD->getReturnType().getTypePtr();
    // Note that it's not always true that we could get a 
    // non-null function here, e.g.:
    // template <typename T> class A {
    //   public:
    //   T foo();
    // };
    // template <typename T> class B {
    // public:
    //   T a;
    //   void bar();
    //   T &return_a() { return a; }
    // };
    // template <typename T> void bar() {
    //   B<T> b;
    //   T xx = b.return_a().foo();
    // }
    // here we cannot resolve return_a's return type
    return getFunctionDeclFromType(Ty, DName);
  }
  return NULL;
}

// could return NULL
const FunctionDecl* RenameCXXMethod::getFunctionDecl(
        const CXXDependentScopeMemberExpr *DE)
{
  if (DE->isImplicitAccess())
    return NULL;
  DeclarationName DName = DE->getMember();
  if (DName.getNameKind() == DeclarationName::CXXOperatorName)
    return NULL;

  TransAssert((DName.getNameKind() == DeclarationName::Identifier) &&
                "Not an indentifier!");
  const Expr *E = DE->getBase();
  TransAssert(E && "NULL Base Expr!");
  const Expr *BaseE = E->IgnoreParens();

  const FunctionDecl *FD = NULL;
  if (dyn_cast<CXXThisExpr>(BaseE)) {
    TransAssert(CurrentFD && "NULL CurrentFD!");
    const DeclContext *Ctx = CurrentFD->getLookupParent();
    TransAssert(Ctx && "Bad DeclContext!");
    DeclContextSet VisitedCtxs;
    FD = lookupFunctionDecl(DName, Ctx, VisitedCtxs);
    TransAssert(FD && "Cannot resolve DName!");
    return FD;
  }

  // it's not always the case we can get a non-null FD, e.g.
  // template<typename T>
  // struct S
  // {
  //   static int foo(T t) { return t.bar(); }
  // };
  // here we can't resolve bar()
  // What should we do? assign a fresh name to this kind of function?
  if (const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(BaseE)) {
    FD = getFunctionDeclFromType(DRE->getType().getTypePtr(), DName);
    return FD;
  }

  if (const CallExpr *CE = dyn_cast<CallExpr>(BaseE)) {
    FD = getFunctionDeclFromReturnType(CE, DName);
    // TransAssert(FD && "Cannot find FunctionDecl!");
    return FD;
  }

  if (const MemberExpr *ME = dyn_cast<MemberExpr>(BaseE)) {
    const Expr *MEBase = ME->getBase();
    return getFunctionDeclFromType(MEBase->getType().getTypePtr(), DName);
  }

  const Type *Ty = DE->getBaseType().getTypePtr();
  if (Ty->isPointerType() || Ty->isReferenceType())
    Ty = getBasePointerElemType(Ty);
  if (const DeclContext *Ctx = getBaseDeclFromType(Ty)) {
    DeclContextSet VisitedCtxs;
    return lookupFunctionDecl(DName, Ctx, VisitedCtxs);
  }
  return NULL;
}

void RenameCXXMethod::rewriteCXXDependentScopeMemberExpr(
       const CXXDependentScopeMemberExpr *DE)
{
  const FunctionDecl *FD = getFunctionDecl(DE);

  if (!FD)
    return;
  const CXXMethodDecl *MD = dyn_cast<CXXMethodDecl>(FD);
  TransAssert(MD && "Invalid MD!");
  std::string NewName = "";
  if (!getMethodNewName(MD, NewName))
    return;
  TransAssert((NewName != "") && "Bad new name!");
  if (DE->getQualifier()) {
    NestedNameSpecifierLoc QualLoc = DE->getQualifierLoc();
    RewriteHelper->replaceCXXMethodNameAfterQualifier(
    &QualLoc, MD, NewName);
  }
  else {
    TheRewriter.ReplaceText(DE->getMemberLoc(),
      MD->getNameAsString().size(), NewName);
  }
}

void RenameCXXMethod::rewriteDependentExpr(const Expr *E)
{
  if (const OverloadExpr *OE = dyn_cast<OverloadExpr>(E)) {
    rewriteOverloadExpr(OE);
    return;
  }
  if (const CXXDependentScopeMemberExpr *DE =
      dyn_cast<CXXDependentScopeMemberExpr>(E)) {
    rewriteCXXDependentScopeMemberExpr(DE);
    return;
  }
}

const CXXMethodDecl* RenameCXXMethod::getCXXMethodFromMemberFunction(
       const CXXMethodDecl *MD)
{
  // coud happen, e.g.:
  // template <typename T> struct A { int foo(); };
  // void bar () {
  //   A<int> a;
  //   a.foo(); <-- here we find foo via getInstantiatedFromMemberFunction
  // }
  const FunctionDecl *FD = MD->getInstantiatedFromMemberFunction();
  if (FD) {
    MD = dyn_cast<CXXMethodDecl>(FD);
    TransAssert(MD && "bad conversion from FD to MD!");
    return MD;
  }

  // one more try
  FD = MD->getTemplateInstantiationPattern();
  if (FD) {
    MD = dyn_cast<CXXMethodDecl>(FD);
    TransAssert(MD && "bad conversion from FD to MD!");
    return MD;
  }
  return NULL;
}

bool RenameCXXMethod::getMethodNewName(const CXXMethodDecl *MD,
                                       std::string &NewName)
{
  const CXXMethodDecl *CanonicalMD = MD->getCanonicalDecl();
  llvm::DenseMap<const CXXMethodDecl *, std::string>::iterator I = 
    NewMethodNames.find(CanonicalMD);
  if (I == NewMethodNames.end()) {
    CanonicalMD = getCXXMethodFromMemberFunction(CanonicalMD);
    I = NewMethodNames.find(CanonicalMD);
    if (I == NewMethodNames.end())
      return false;
  }
  NewName = I->second;
  return true;
}

void RenameCXXMethod::rewriteFunctionTemplateExplicitInstantiation(
       const FunctionDecl *FD)
{
  const CXXMethodDecl *MD = dyn_cast<CXXMethodDecl>(FD);
  TransAssert(MD && "Not a CXXMethodDecl!");

  const FunctionDecl *Pattern = MD->getTemplateInstantiationPattern();
  TransAssert(Pattern && "Cannot find Template Pattern!");
  const CXXMethodDecl *TmplMD = dyn_cast<CXXMethodDecl>(Pattern);
  TransAssert(TmplMD && "Invalid CXXMethodDecl!");

  CXXMethodDeclToNameMap::iterator I = 
    NewMethodNames.find(TmplMD->getCanonicalDecl());
  TransAssert((I != NewMethodNames.end()) && "Cannot find CXXMethodDecl!");
  std::string Name = I->second;
  I = NewMethodNames.find(MD);
  TransAssert((I == NewMethodNames.end()) && "Duplicate find CXXMethodDecl?");

  SourceLocation Loc = MD->getPointOfInstantiation();
  TheRewriter.ReplaceText(Loc, MD->getNameAsString().size(), Name);
}

// avoid rewriting the content of an implicit instantiated record decl, e.g.
// struct A {};
// template <typename T = A> class B {
// public:
//   B() { foo(0); } <--- avoid writing the foo of the implicit instantiation
//   void foo ( int );
// };
// struct C { B<> b; };
// void foo() { new C; }
// template <typename T> B<T>::B() { foo(0); }
bool RenameCXXMethod::isExplicit(const CXXMethodDecl *MD)
{
  if (dyn_cast<CXXMethodDecl>(CurrentFD)) {
    if (ClassInstantiation)
      return false;
    return (InstantiationQueue.size() == 0);
  }
  else {
    if (FunctionInstantiation && MD->isStatic())
      return true;
    return !FunctionInstantiation;
  }
}

void RenameCXXMethod::setClassInstantiationFlag(const RecordDecl *RD)
{
  if (const ClassTemplateSpecializationDecl *Spec =
      dyn_cast<ClassTemplateSpecializationDecl>(RD)) {
    TemplateSpecializationKind K = Spec->getSpecializationKind();
    if (K == TSK_ExplicitInstantiationDeclaration) {
      ClassInstantiation = true;
      return;
    }
  }
  ClassInstantiation = false;
}

void RenameCXXMethod::setFunctionInstantiationFlag(const FunctionDecl *FD)
{
  TemplateSpecializationKind K = FD->getTemplateSpecializationKind();
  if ((K == TSK_ImplicitInstantiation) ||
      (K == TSK_ExplicitInstantiationDeclaration) ||
      (K == TSK_ExplicitInstantiationDefinition)) {
    FunctionInstantiation = true;
  }
  else {
    FunctionInstantiation = false;
  }
}

RenameCXXMethod::~RenameCXXMethod(void)
{
  delete MethodCollectionVisitor;
  delete RenameVisitor;
}

