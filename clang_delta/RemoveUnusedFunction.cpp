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

#include "RemoveUnusedFunction.h"

#include <cctype>
#include <algorithm>
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"
#include "clang/AST/Attr.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Remove unused function declarations. \n";

static RegisterTransformation<RemoveUnusedFunction>
         Trans("remove-unused-function", DescriptionMsg);

class RUFAnalysisVisitor : public RecursiveASTVisitor<RUFAnalysisVisitor> {
public:

  explicit RUFAnalysisVisitor(RemoveUnusedFunction *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitFunctionDecl(FunctionDecl *FD);

private:

  RemoveUnusedFunction *ConsumerInstance;
};

typedef llvm::DenseMap<const clang::UsingDecl *,
                       const clang::FunctionDecl *>
          UsingFunctionDeclsMap;

typedef llvm::SmallPtrSet<const clang::FunctionDecl *, 32>
          FunctionDeclsSet;

typedef llvm::SmallPtrSet<const clang::FunctionDecl *, 5>
          MemberSpecializationSet;

namespace {

class UsingDeclVisitor : public
        RecursiveASTVisitor<UsingDeclVisitor> {
public:
  UsingDeclVisitor(const FunctionDecl *FD,
                   RemoveUnusedFunction *Instance)
    : CurrentFD(FD),
      ConsumerInstance(Instance)
  { }

  bool VisitUsingDecl(UsingDecl *D);

private:
  const FunctionDecl *CurrentFD;

  RemoveUnusedFunction *ConsumerInstance;
};

bool UsingDeclVisitor::VisitUsingDecl(UsingDecl *D)
{
  ConsumerInstance->handleOneUsingDecl(CurrentFD, D);
  return true;
}
// end of UsingDeclVisitor

class SpecializationVisitor : public
        RecursiveASTVisitor<SpecializationVisitor> {
public:
  explicit SpecializationVisitor(RemoveUnusedFunction *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitFunctionDecl(FunctionDecl *FD);

  bool VisitMemberExpr(MemberExpr *E);

  bool VisitCXXOperatorCallExpr(CXXOperatorCallExpr *E);

  bool VisitCallExpr(CallExpr *E);

  bool shouldVisitTemplateInstantiations() {
    return true;
  }

private:

  RemoveUnusedFunction *ConsumerInstance;
};

bool SpecializationVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  ConsumerInstance->handleOneFunctionDecl(FD);
  return true;
}

bool SpecializationVisitor::VisitMemberExpr(MemberExpr *E)
{
  ConsumerInstance->handleOneMemberExpr(E);
  return true;
}

bool SpecializationVisitor::VisitCXXOperatorCallExpr(
       CXXOperatorCallExpr *E)
{
  // ConsumerInstance->handleOneCXXOperatorCallExpr(E);
  return true;
}

bool SpecializationVisitor::VisitCallExpr(
       CallExpr *E)
{
  ConsumerInstance->handleOneCallExpr(E);
  return true;
}


// end of SpecializationVisitor

class ReferencedFunctionDeclVisitor : public
        RecursiveASTVisitor<ReferencedFunctionDeclVisitor> {
public:
  ReferencedFunctionDeclVisitor(const FunctionDecl *FD,
                                RemoveUnusedFunction *Instance)
    : CurrentFD(FD),
      ConsumerInstance(Instance)
  { }

  bool VisitCXXDependentScopeMemberExpr(CXXDependentScopeMemberExpr *E);

  bool VisitUnresolvedLookupExpr(UnresolvedLookupExpr *E);

private:
  const FunctionDecl *CurrentFD;

  RemoveUnusedFunction *ConsumerInstance;
};

bool ReferencedFunctionDeclVisitor::VisitUnresolvedLookupExpr(
       UnresolvedLookupExpr *E)
{
  ConsumerInstance->handleOneUnresolvedLookupExpr(CurrentFD, E);
  return true;
}

bool ReferencedFunctionDeclVisitor::VisitCXXDependentScopeMemberExpr(
       CXXDependentScopeMemberExpr *E)
{
  ConsumerInstance->handleOneCXXDependentScopeMemberExpr(CurrentFD, E);
  return true;
}

} // end of anon namespace

class ExtraReferenceVisitorWrapper : public
        RecursiveASTVisitor<ExtraReferenceVisitorWrapper> {
public:
  explicit ExtraReferenceVisitorWrapper(RemoveUnusedFunction *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitFunctionDecl(FunctionDecl *FD);

private:

  RemoveUnusedFunction *ConsumerInstance;
};

bool ExtraReferenceVisitorWrapper::VisitFunctionDecl(FunctionDecl *FD)
{
  if (!FD->isThisDeclarationADefinition())
    return true;
  UsingDeclVisitor Visitor(FD, ConsumerInstance);
  Visitor.TraverseDecl(FD);
  ReferencedFunctionDeclVisitor FuncVisitor(FD, ConsumerInstance);
  FuncVisitor.TraverseDecl(FD);
  ConsumerInstance->setInlinedSystemFunctions(FD);
  return true;
}

bool RUFAnalysisVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  if (ConsumerInstance->isInIncludedFile(FD))
    return true;
  const FunctionDecl *CanonicalFD = FD->getCanonicalDecl();
  if (ConsumerInstance->VisitedFDs.count(CanonicalFD))
    return true;
  ConsumerInstance->VisitedFDs.insert(CanonicalFD);

  FunctionDecl::TemplatedKind TK = FD->getTemplatedKind();
  if (TK == FunctionDecl::TK_MemberSpecialization) {
    const FunctionDecl *Member = FD->getInstantiatedFromMemberFunction();
    ConsumerInstance->addOneMemberSpecialization(FD, Member);
    return true;
  }
  else if (TK == FunctionDecl::TK_DependentFunctionTemplateSpecialization) {
    const DependentFunctionTemplateSpecializationInfo *Info =
      FD->getDependentSpecializationInfo();
    // don't need to track all specs, just associate FD with one
    // of those
    if (Info->getNumTemplates() > 0) {
      const FunctionDecl *Member =
        Info->getTemplate(0)->getTemplatedDecl();
      ConsumerInstance->addOneMemberSpecialization(FD, Member);
    }
    return true;
  }

  TemplateSpecializationKind K = FD->getTemplateSpecializationKind();
  if (K == TSK_ExplicitSpecialization) {
    ConsumerInstance->addFuncToExplicitSpecs(FD);
    return true;
  }
  // Why implicit instantiation would appear here? e.g.:
  // template<typename T> void foo(T &);
  // struct S { friend void foo<>(bool&); };
  // OK, just add it into MemberSpec set.
  if (K ==  TSK_ImplicitInstantiation) {
    const FunctionTemplateDecl *FTD = FD->getPrimaryTemplate();
    TransAssert(FTD && "NULL FunctionTemplateDecl!");
    const FunctionDecl *Member = FTD->getTemplatedDecl();
    ConsumerInstance->addOneMemberSpecialization(FD, Member);
    return true;
  }

  if (FD->isReferenced() ||
      FD->isMain() ||
      FD->hasAttr<OpenCLKernelAttr>() ||
      ConsumerInstance->hasReferencedSpecialization(CanonicalFD) ||
      ConsumerInstance->isInlinedSystemFunction(CanonicalFD) ||
      ConsumerInstance->isInReferencedSet(CanonicalFD) ||
      !ConsumerInstance->hasAtLeastOneValidLocation(CanonicalFD))
    return true;

  ConsumerInstance->addOneFunctionDecl(CanonicalFD);
  return true;
}

void RemoveUnusedFunction::Initialize(ASTContext &context)
{
  Transformation::Initialize(context);
  AnalysisVisitor = new RUFAnalysisVisitor(this);
  VisitorWrapper = new ExtraReferenceVisitorWrapper(this);
  initializeInlinedSystemFunctions();
}

void RemoveUnusedFunction::HandleTranslationUnit(ASTContext &Ctx)
{
  VisitorWrapper->TraverseDecl(Ctx.getTranslationUnitDecl());
  // visit using decls declared outsided function-scope
  UsingDeclVisitor Visitor(NULL, this);
  Visitor.TraverseDecl(Ctx.getTranslationUnitDecl());
  SpecializationVisitor SpecVisitor(this);
  SpecVisitor.TraverseDecl(Ctx.getTranslationUnitDecl());
  AnalysisVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

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

// Each "key" function is inlined into the corresponding "value"
// function, and its isReferenced flag is false...
void RemoveUnusedFunction::initializeInlinedSystemFunctions()
{
  InlinedSystemFunctions["__fprintf_chk"] = "fprintf";
  InlinedSystemFunctions["__printf_chk"] = "printf";
  InlinedSystemFunctions["__dprintf_chk"] = "dprintf";
  InlinedSystemFunctions["__asprintf_chk"] = "asprintf";
  InlinedSystemFunctions["__obstack_printf_chk"] = "obstack_printf";
  InlinedSystemFunctions["__swprintf_chk"] = "swprintf";
  InlinedSystemFunctions["__swprintf_alias"] = "swprintf";
  InlinedSystemFunctions["__fwprintf_chk"] = "fwprintf";
  InlinedSystemFunctions["__wprintf_chk"] = "wprintf";
}

void RemoveUnusedFunction::doRewriting()
{
  if (ToCounter <= 0) {
    TransAssert(TheFunctionDecl && "NULL TheFunctionDecl!");
    // add FD under removal first in order to avoid recursion, e.g.
    // void foo() { using ::foo; }
    RemovedFDs.insert(TheFunctionDecl);
    removeOneFunctionDeclGroup(TheFunctionDecl);
    return;
  }

  TransAssert((TransformationCounter <=
                 static_cast<int>(AllValidFunctionDecls.size())) &&
              "TransformationCounter is larger than the number of defs!");
  TransAssert((ToCounter <= static_cast<int>(AllValidFunctionDecls.size())) &&
              "ToCounter is larger than the number of defs!");
  for (int I = ToCounter; I >= TransformationCounter; --I) {
    TransAssert((I >= 1) && "Invalid Index!");
    const FunctionDecl *FD = AllValidFunctionDecls[I-1];
    TransAssert(FD && "NULL FunctionDecl!");
    RemovedFDs.insert(FD);
    removeOneFunctionDeclGroup(FD);
  }
}

SourceLocation RemoveUnusedFunction::getExtensionLocStart(
                 SourceLocation Loc)
{
  SourceLocation StartLoc = Loc;
  FileID FID = SrcManager->getMainFileID();
  SourceLocation FileStartLoc = SrcManager->getLocForStartOfFile(FID);
  const char * const FileStartBuf = SrcManager->getCharacterData(FileStartLoc);
  const char *StartBuf = SrcManager->getCharacterData(StartLoc);
  const char *MatchedPos = NULL;
  bool Out = false;
  std::string Ext = "__noisnetxe__";
  while (true) {
    do {
      StartBuf--;
      if (StartBuf < FileStartBuf)
        break;
    } while (isspace(*StartBuf));

    int Len = static_cast<int>(Ext.length());
    for (int I = 0; I < Len; I++) {
      if (StartBuf < FileStartBuf || *StartBuf != Ext[I]) {
        Out = true;
        break;
      }
      StartBuf--;
    }
    if (Out)
      break;
    StartBuf++;
    MatchedPos = StartBuf;
  }
  // no __extension__ in front
  if (MatchedPos == NULL)
    return Loc;

  TransAssert((MatchedPos >= FileStartBuf) && "Invalid MatchedPos!");
  std::string ExtStr(MatchedPos, Ext.length());
  TransAssert((ExtStr == "__extension__") && "Invalid ExtStr!"); (void)ExtStr;
  int Offset = MatchedPos - (SrcManager->getCharacterData(StartLoc));
  return StartLoc.getLocWithOffset(Offset);
}

bool RemoveUnusedFunction::hasValidOuterLocStart(
       const FunctionTemplateDecl *FTD, const FunctionDecl *FD)
{
  SourceLocation FTDLocStart = FTD->getSourceRange().getBegin();
  SourceLocation FDLocStart = FD->getSourceRange().getBegin();
  const char *FTDStartPos = SrcManager->getCharacterData(FTDLocStart);
  const char *FDStartPos = SrcManager->getCharacterData(FDLocStart);
  return (FTDStartPos < FDStartPos);
}

SourceLocation RemoveUnusedFunction::getFunctionOuterLocStart(
                 const FunctionDecl *FD)
{
  SourceLocation LocStart = FD->getLocStart();
  bool RecordLoc = false;

  // check if FD is from a function template
  if (FunctionTemplateDecl *FTD = FD->getDescribedFunctionTemplate()) {
    // get FTD->getLocStart() only if it is less than FD->getLocStart,
    // for example, in the code below:
    //   template <typename T> struct S {template <typename T1> void foo();};
    //   template<typename T> template<typename T1> void S<T>::foo() { }
    // where
    //   FTD->getLocStart() points to the begining of "template<typename T1>"
    if (hasValidOuterLocStart(FTD, FD)) {
      LocStart = FTD->getLocStart();
      RecordLoc = true;
    }
  }

  if (LocStart.isMacroID())
    LocStart = SrcManager->getExpansionLoc(LocStart);

  // this is ugly, but how do we get the location of __extension__? e.g.:
  // __extension__ void foo();
  LocStart = getExtensionLocStart(LocStart);

  // In some cases where the given input is not well-formed, we may
  // end up with duplicate locations for the FunctionTemplateDecl
  // case. In such cases, we simply return an invalid SourceLocation,
  // which will ben skipped by the caller of getFunctionOuterLocStart.
  if (RecordLoc) {
    if (VisitedLocations.count(LocStart))
      return SourceLocation();
    VisitedLocations.insert(LocStart);
  }

  return LocStart;
}

bool RemoveUnusedFunction::isTokenOperator(SourceLocation Loc)
{
  const char * Buf = SrcManager->getCharacterData(Loc);
  std::string OStr = "operator";
  std::string S(Buf, OStr.length());
  return (S == OStr);
}

// this is ugly, but seems clang gives "bad" location for the code below:
// template<typename T> void foo(T *);
// template <typename T> struct S {
//   template<typename T1> friend void foo(T1 *);
//                                     ^ FD.end_loc points here
// };
// template struct S<char>;
SourceLocation RemoveUnusedFunction::getFunctionLocEnd(
                 SourceLocation LocStart,
                 SourceLocation LocEnd,
                 const FunctionDecl *FD)
{
  if (!FD->getDescribedFunctionTemplate()) {
    // Remove trailing ; if function has no body
    if (!FD->hasBody()) {
      return RewriteHelper->getLocationUntil(LocEnd, ';');
    }
    else {
      return LocEnd;
    }
  }

  SourceLocation FDLoc = FD->getLocation();
  if (FDLoc.isMacroID())
    FDLoc = SrcManager->getExpansionLoc(FDLoc);
  const char * const FDBuf = SrcManager->getCharacterData(FDLoc);
  const char * LocEndBuf = SrcManager->getCharacterData(LocEnd);
  if ((FDBuf < LocEndBuf) && !isTokenOperator(FDLoc))
    return LocEnd;
  std::string Str;
  RewriteHelper->getStringBetweenLocs(Str, LocStart, LocEnd);
  if (Str.find("friend ") == std::string::npos &&
      Str.find("friend\t") == std::string::npos &&
      Str.find("friend\n") == std::string::npos)
    return LocEnd;
  int Offset = 0;
  while (*LocEndBuf != ';') {
    LocEndBuf++;
    Offset++;
  }
  return LocEnd.getLocWithOffset(Offset-1);
}

void RemoveUnusedFunction::removeOneFunctionDecl(const FunctionDecl *FD)
{
  SourceRange FuncRange = FD->getSourceRange();
  SourceLocation LocEnd = FuncRange.getEnd();
  if (LocEnd.isMacroID())
    LocEnd = SrcManager->getExpansionLoc(LocEnd);
  if (!FD->isInExternCContext() && !FD->isInExternCXXContext()) {
    SourceLocation FuncLocStart = getFunctionOuterLocStart(FD);
    if (FuncLocStart.isInvalid())
      return;
    LocEnd = getFunctionLocEnd(FuncLocStart, LocEnd, FD);
    if (SrcManager->isWrittenInMainFile(FuncLocStart) &&
        SrcManager->isWrittenInMainFile(LocEnd))
      TheRewriter.RemoveText(SourceRange(FuncLocStart, LocEnd));
    return;
  }

  const DeclContext *Ctx = FD->getLookupParent();
  const LinkageSpecDecl *Linkage = dyn_cast<LinkageSpecDecl>(Ctx);
  if (!Linkage) {
    SourceLocation FuncLocStart = getFunctionOuterLocStart(FD);
    if (FuncLocStart.isInvalid())
      return;
    LocEnd = getFunctionLocEnd(FuncLocStart, LocEnd, FD);
    TheRewriter.RemoveText(SourceRange(FuncLocStart, LocEnd));
    return;
  }
  // cases like:
  // extern "C++" { void foo(); }
  // namespace { using ::foo; }
  if (Linkage->hasBraces()) {
    SourceLocation FuncLocStart = getFunctionOuterLocStart(FD);
    if (FuncLocStart.isInvalid())
      return;
    TheRewriter.RemoveText(SourceRange(FuncLocStart, LocEnd));
    return;
  }
  // cases like:
  // extern "C++" void foo();
  // it also handles cases such as extern "C++" template<typename T> ...
  SourceLocation LocStart = Linkage->getExternLoc();
  if (LocStart.isMacroID())
    LocStart = SrcManager->getExpansionLoc(LocStart);
  LocStart = getExtensionLocStart(LocStart);
  TheRewriter.RemoveText(SourceRange(LocStart, LocEnd));
}

void RemoveUnusedFunction::removeOneExplicitInstantiation(
       const FunctionDecl *Spec)
{
  FileID FID = SrcManager->getMainFileID();
  SourceLocation FileStartLoc = SrcManager->getLocForStartOfFile(FID);
  const char * const FileStartBuf = SrcManager->getCharacterData(FileStartLoc);

  SourceLocation Loc = Spec->getPointOfInstantiation();
  const char *OrigStartBuf = SrcManager->getCharacterData(Loc);
  const char *StartBuf = OrigStartBuf;
  int Offset = 0;
  while ((*StartBuf != ';') && (*StartBuf != '{') && (*StartBuf != '}') &&
         (StartBuf >= FileStartBuf)) {
    StartBuf--;
    Offset--;
  }
  SourceLocation LocStart = Loc.getLocWithOffset(Offset + 1);
  Offset = 0;
  StartBuf = OrigStartBuf;
  while (*StartBuf != ';') {
    StartBuf++;
    Offset++;
  }
  SourceLocation LocEnd = Loc.getLocWithOffset(Offset - 1);
  TheRewriter.RemoveText(SourceRange(LocStart, LocEnd));
}

void RemoveUnusedFunction::removeRemainingExplicitSpecs(
       MemberSpecializationSet *ExplicitSpecs)
{
  if (!ExplicitSpecs)
    return;
  for (MemberSpecializationSet::iterator I = ExplicitSpecs->begin(),
       E = ExplicitSpecs->end(); I != E; ++I) {
    removeOneFunctionDecl(*I);
  }
}

// We can't handle function template explicit instantiation because
// I am not sure how to analyze its source location.
// For example, in the code below:
// template<typename T> bool foo(const int&) {return true;}
// template bool foo<char>(const int&);
//               ^ point of instantiation
// What I could get is the point of the instantiation, but I
// don't know the source range for the entire instantiation declaration.
void RemoveUnusedFunction::removeFunctionExplicitInstantiations(
       const FunctionDecl *FD)
{
  const FunctionTemplateDecl *FTD = getTopDescribedTemplate(FD);
  if (!FTD)
    return;

  const FunctionDecl *CanonicalFD = FD->getCanonicalDecl();
  MemberSpecializationSet *S = MemberToInstantiations[CanonicalFD];
  MemberSpecializationSet *ExplicitSpecs =
          FuncToExplicitSpecs[CanonicalFD];

  for (FunctionTemplateDecl::spec_iterator I = FTD->spec_begin(),
       E = FTD->spec_end(); I != E; ++I) {
    FunctionDecl *Spec = (*I);
    TemplateSpecializationKind K = Spec->getTemplateSpecializationKind();
    if (K == TSK_ExplicitSpecialization) {
      // check if the explicit spec points to any dependent specs,
      // skip it if yes
      if (!ExplicitSpecs) {
        removeOneFunctionDecl(Spec);
      }
      else if (ExplicitSpecs->count(Spec)) {
        removeOneFunctionDecl(Spec);
        ExplicitSpecs->erase(Spec);
      }
      continue;
    }
    if (K != TSK_ExplicitInstantiationDeclaration &&
        K != TSK_ExplicitInstantiationDefinition)
      continue;
    TransAssert(!Spec->isReferenced() && "Referenced Instantiation!");
    removeOneExplicitInstantiation(Spec);
    if (S && S->count(Spec)) {
      S->erase(Spec);
    }
  }

  removeRemainingExplicitSpecs(ExplicitSpecs);

  if (!S)
    return;
  for (MemberSpecializationSet::iterator I = S->begin(), E = S->end();
       I != E; ++I) {
    const FunctionDecl *Spec = (*I);
    TemplateSpecializationKind K = Spec->getTemplateSpecializationKind();
    TransAssert(((K == TSK_ExplicitInstantiationDeclaration) ||
                  (K == TSK_ExplicitInstantiationDefinition)) &&
                "Bad Instantiation!"); (void)K;
    removeOneExplicitInstantiation(Spec);
  }
}

void RemoveUnusedFunction::removeMemberSpecializations(const FunctionDecl *FD)
{
  MemberSpecializationSet *S = MemberToSpecs[FD->getCanonicalDecl()];
  if (!S)
    return;
  for (MemberSpecializationSet::iterator I = S->begin(), E = S->end();
       I != E; ++I) {
    const FunctionDecl *Spec = (*I);
    removeOneFunctionDecl(Spec);
  }
}

void RemoveUnusedFunction::removeOneFunctionDeclGroup(const FunctionDecl *FD)
{
  for (FunctionDecl::redecl_iterator RI = FD->redecls_begin(),
       RE = FD->redecls_end(); RI != RE; ++RI) {
    removeOneFunctionDecl(*RI);
  }

  removeMemberSpecializations(FD);
  removeFunctionExplicitInstantiations(FD);

  for (UsingFunctionDeclsMap::const_iterator I = UsingFDs.begin(),
       E = UsingFDs.end(); I != E; ++I) {
    if ((*I).second != FD)
      continue;
    const FunctionDecl *ParentFD = UsingParentFDs[(*I).first];
    if (ParentFD && RemovedFDs.count(ParentFD->getCanonicalDecl()))
      continue;
    RewriteHelper->removeDecl((*I).first);
  }
}

bool RemoveUnusedFunction::hasAtLeastOneValidLocation(const FunctionDecl *FD)
{
  for (FunctionDecl::redecl_iterator RI = FD->redecls_begin(),
       RE = FD->redecls_end(); RI != RE; ++RI) {
    SourceRange FuncRange = FD->getSourceRange();
    SourceLocation StartLoc = FuncRange.getBegin();
    if (StartLoc.isMacroID())
      StartLoc = SrcManager->getExpansionLoc(StartLoc);
    SourceLocation EndLoc = FuncRange.getEnd();
    if (EndLoc.isMacroID())
      EndLoc = SrcManager->getExpansionLoc(EndLoc);
    if (SrcManager->isWrittenInMainFile(StartLoc) &&
        SrcManager->isWrittenInMainFile(EndLoc))
      return true;
  }
  return false;
}

bool RemoveUnusedFunction::isInReferencedSet(const FunctionDecl *CanonicalFD)
{
  TransAssert(CanonicalFD && "NULL FunctionDecl!");
  return ReferencedFDs.count(CanonicalFD);
}

const FunctionDecl *
RemoveUnusedFunction::lookupFunctionDeclShallow(const DeclarationName &DName,
                                                const DeclContext *Ctx)
{
  if (dyn_cast<LinkageSpecDecl>(Ctx))
    return NULL;
  DeclContext::lookup_result Result = Ctx->lookup(DName);
  for (auto I = Result.begin(), E = Result.end();
       I != E; ++I) {
    if (const FunctionDecl *FD = dyn_cast<FunctionDecl>(*I))
      return FD;
    if (const UsingShadowDecl *USD = dyn_cast<UsingShadowDecl>(*I)) {
      const NamedDecl *ND = USD->getTargetDecl();
      if (const FunctionDecl *FD = dyn_cast<FunctionDecl>(ND))
        return FD;
    }
    if (const FunctionTemplateDecl *TD = dyn_cast<FunctionTemplateDecl>(*I)) {
      return TD->getTemplatedDecl();
    }
  }

  for (auto *I : Ctx->using_directives()) {
    const NamespaceDecl *ND = I->getNominatedNamespace();
    // avoid infinite recursion
    if (ND->getLookupParent() == Ctx)
      return NULL;
    if (const FunctionDecl *FD = lookupFunctionDeclShallow(DName, ND))
      return FD;
  }
  return NULL;
}

const FunctionDecl *RemoveUnusedFunction::getFunctionDeclFromSpecifier(
        const DeclarationName &Name, const NestedNameSpecifier *NNS)
{
  const FunctionDecl *FD = NULL;
  switch (NNS->getKind()) {
  case NestedNameSpecifier::Namespace:
    FD = lookupFunctionDeclShallow(Name,
                                   NNS->getAsNamespace());
    break;
  case NestedNameSpecifier::NamespaceAlias:
    FD = lookupFunctionDeclShallow(Name,
           NNS->getAsNamespaceAlias()->getNamespace());
    break;
  case NestedNameSpecifier::Global:
    FD = lookupFunctionDeclShallow(Name,
                                   Context->getTranslationUnitDecl());
    break;
  default:
    return NULL;
  }
  return FD;
}

void RemoveUnusedFunction::handleOneUsingDecl(const FunctionDecl *CurrentFD,
                                              const UsingDecl *UD)
{
  if (VisitedUsingDecls.count(UD))
    return;

  VisitedUsingDecls.insert(UD);
  const NestedNameSpecifier *NNS = UD->getQualifier();
  if (!NNS)
    return;
  DeclarationName Name = UD->getUnderlyingDecl()->getDeclName();
  const FunctionDecl *FD = getFunctionDeclFromSpecifier(Name, NNS);
  if (!FD || FD->isReferenced())
    return;

  // we don't put FD into ReferencedFD because we will
  // delete both the FD and the UsingDecl if FD is only referenced
  // by UsingDecl[s].
  const FunctionDecl *CanonicalFD = FD->getCanonicalDecl();
  TransAssert((UsingFDs.find(UD) == UsingFDs.end()) &&
              "Duplicate UsingDecl to FD map!");
  UsingFDs[UD] = CanonicalFD;
  if (CurrentFD) {
    TransAssert(CurrentFD->isThisDeclarationADefinition() &&
                "CurrentFD is not a definition!");
    TransAssert((UsingParentFDs.find(UD) == UsingParentFDs.end()) &&
                "Duplicate UsingDecl to ParentFD map!");
    UsingParentFDs[UD] = CurrentFD;
  }
}

void RemoveUnusedFunction::handleOneCXXDependentScopeMemberExpr(
       const FunctionDecl *CurrentFD,
       const CXXDependentScopeMemberExpr *E)
{
  if (E->isImplicitAccess())
    return;
  DeclarationName DName = E->getMember();
  DeclarationName::NameKind K = DName.getNameKind();
  if ((K != DeclarationName::CXXOperatorName) &&
      (K != DeclarationName::Identifier))
    return;
  const Expr *Base = E->getBase()->IgnoreParenCasts();
  const FunctionDecl *FD = NULL;
  if (dyn_cast<CXXThisExpr>(Base)) {
    TransAssert(CurrentFD && "NULL CurrentFD");
    const DeclContext *Ctx = CurrentFD->getLookupParent();
    TransAssert(Ctx && "Bad DeclContext!");
    DeclContextSet VisitedCtxs;
    FD = lookupFunctionDecl(DName, Ctx, VisitedCtxs);
    // we may not get FD in cases where we have this->m_field
    if (FD)
      addOneReferencedFunction(FD);
    return;
  }
}

void RemoveUnusedFunction::handleOneUnresolvedLookupExpr(
       const FunctionDecl *CurrentFD,
       const UnresolvedLookupExpr *E)
{
  DeclarationName DName = E->getName();
  DeclarationName::NameKind K = DName.getNameKind();
  if ((K != DeclarationName::CXXOperatorName) &&
      (K != DeclarationName::Identifier))
    return;
  const NestedNameSpecifier *NNS = E->getQualifier();
  // we fail only if UE is invoked with some qualifier or
  // instantiation, e.g.:
  // namespace NS { template<typename T> void foo(T&) { } }
  // template<typename T> void bar(T p) { NS::foo(p); }
  // removing foo would fail.
  //
  // template <typename T> void foo() { }
  // template <typename T> void bar() { foo<T>(); }
  // removing foo would faill, too
  //
  // On the other handle, the following code is ok:
  // template<typename T> void foo(T&) { }
  // template<typename T> void bar(T p) { foo(p); }
  const FunctionDecl *FD = NULL;
  if (NNS) {
    FD = getFunctionDeclFromSpecifier(DName, NNS);
  }
  else {
    const DeclContext *Ctx = CurrentFD->getLookupParent();
    FD = lookupFunctionDeclShallow(DName, Ctx);
  }

  if (!FD || FD->isReferenced())
    return;
  addOneReferencedFunction(FD);
}

void RemoveUnusedFunction::handleOneMemberExpr(
       const MemberExpr *ME)
{
  const ValueDecl *VD = ME->getMemberDecl();
  const CXXMethodDecl *MD = dyn_cast<CXXMethodDecl>(VD);
  if (!MD)
    return;
  if (const FunctionDecl *FD = MD->getInstantiatedFromMemberFunction())
    addOneReferencedFunction(FD);
}

const FunctionDecl *RemoveUnusedFunction::getSourceFunctionDecl(
        const FunctionDecl *TheFD)
{
  if (FunctionTemplateDecl *FTD = TheFD->getPrimaryTemplate()) {
    if (const FunctionTemplateDecl *D =
        FTD->getInstantiatedFromMemberTemplate())
      return D->getTemplatedDecl();
    else
      return FTD->getTemplatedDecl();
  }
  if (const FunctionDecl *FD = TheFD->getInstantiatedFromMemberFunction())
    return FD;
  return TheFD;
}

// we can't handle cases like below:
// template<typename T> struct S;
// template<typename T> void foo(S<T>);
// template<> void foo(S<char>);
// template void foo(S<char>);
// the second instantiation will refer to the specialization, and thus
// we lose the source range of it... in fact, it points to the
// original template declaration.
void RemoveUnusedFunction::handleOneFunctionDecl(const FunctionDecl *TheFD)
{
  const FunctionDecl *FD = getSourceFunctionDecl(TheFD);
  if (TheFD->isReferenced()) {
    addOneReferencedFunction(FD);
    return;
  }

  FunctionDecl::TemplatedKind TK = TheFD->getTemplatedKind();
  // Now it's another ugly part, with dependent function template spec,
  // we lose the explicit specialization in Member's spec_iterator, e.g.:
  // template <class T> void foo(T);           // line 1
  // template <class T1, class T2> class A {   // line 2
  //   friend void foo<>(T1);                  // line 3
  // };                                        // line 4
  // template <> void foo(char);               // line 5
  // template class A<char, bool>;             // line 6
  // In the code above, foo's spec_iterator has an explicit spec,
  // but this spec's source range points to the dependent_spec at line 3,
  // and then we will be in trouble...
  if (TK == FunctionDecl::TK_DependentFunctionTemplateSpecialization) {
    const DependentFunctionTemplateSpecializationInfo *Info =
      TheFD->getDependentSpecializationInfo();
    // don't need to track all specs, just associate FD with one
    // of those
    if (Info->getNumTemplates() > 0) {
      const FunctionDecl *Member =
        Info->getTemplate(0)->getTemplatedDecl();
      createFuncToExplicitSpecs(Member);
    }
    return;
  }

  // keep explicit instantiations that are not recored by original
  // template's spec_iterator, e.g.:
  // template<typename T> class S {
  // public:
  //   template<typename V> void foo(V& v) {};
  // };
  // typedef S<char> SS;
  // template void SS::foo(unsigned short&);
  FunctionTemplateDecl *FTD = TheFD->getPrimaryTemplate();
  if (!FTD)
    return;

  const FunctionTemplateDecl *D = FTD->getInstantiatedFromMemberTemplate();
  if (!D)
    return;
  const FunctionDecl *OrigFD = D->getTemplatedDecl();
  TemplateSpecializationKind K = TheFD->getTemplateSpecializationKind();
  if (K != TSK_ExplicitInstantiationDeclaration &&
      K != TSK_ExplicitInstantiationDefinition)
    return;

  MemberSpecializationSet *S =
    MemberToInstantiations[OrigFD->getCanonicalDecl()];
  if (S == NULL) {
    S = new MemberSpecializationSet();
    MemberToInstantiations[OrigFD->getCanonicalDecl()] = S;
  }
  S->insert(TheFD);
}

void RemoveUnusedFunction::handleOneCallExpr(const CallExpr *E)
{
  const FunctionDecl *FD = E->getDirectCallee();
  if (!FD)
    return;
  const FunctionDecl *TheFD = getSourceFunctionDecl(FD);
  addOneReferencedFunction(TheFD);
}

void RemoveUnusedFunction::handleOneCXXOperatorCallExpr(
       const CXXOperatorCallExpr *E)
{
  const FunctionDecl *FD = E->getDirectCallee();
  if (!FD)
    return;
  const CXXMethodDecl *MD = dyn_cast<CXXMethodDecl>(FD);
  if (!MD)
    return;
  if (const FunctionDecl *OrigFD = MD->getInstantiatedFromMemberFunction())
    addOneReferencedFunction(OrigFD);
}

void RemoveUnusedFunction::addOneFunctionDecl(const FunctionDecl *CanonicalFD)
{
  ValidInstanceNum++;
  if (ToCounter > 0) {
    AllValidFunctionDecls.push_back(CanonicalFD);
    return;
  }
  if (ValidInstanceNum == TransformationCounter) {
    TheFunctionDecl = CanonicalFD;
  }
}

void RemoveUnusedFunction::addOneReferencedFunction(
       const FunctionDecl *FD)
{
  ReferencedFDs.insert(FD->getCanonicalDecl());
}

const FunctionTemplateDecl *RemoveUnusedFunction::getTopDescribedTemplate(
        const FunctionDecl *FD)
{
  FunctionTemplateDecl *FTD = FD->getDescribedFunctionTemplate();
  if (!FTD)
    return NULL;
  if (const FunctionTemplateDecl *D = FTD->getInstantiatedFromMemberTemplate())
    return D;
  else
    return FTD;
}

bool RemoveUnusedFunction::hasReferencedSpecialization(const FunctionDecl *FD)
{
  if (const FunctionTemplateDecl *FTD = getTopDescribedTemplate(FD)) {
    for (FunctionTemplateDecl::spec_iterator I = FTD->spec_begin(),
         E = FTD->spec_end(); I != E; ++I) {
      if ((*I)->isReferenced())
        return true;
    }
  }
  return false;
}

void RemoveUnusedFunction::setInlinedSystemFunctions(const FunctionDecl *FD)
{
  if (!FD->isInlined())
    return;

  std::string FDNameStr = FD->getNameAsString();
  for (InlinedSystemFunctionsMap::iterator I = InlinedSystemFunctions.begin(),
       E = InlinedSystemFunctions.end(); I != E; ++I) {
    if (FDNameStr == (*I).second) {
      ExistingSystemFunctions.insert(FDNameStr);
      return;
    }
  }
}

bool RemoveUnusedFunction::isInlinedSystemFunction(const FunctionDecl *FD)
{
  std::string FDNameStr = FD->getNameAsString();
  InlinedSystemFunctionsMap::iterator I =
    InlinedSystemFunctions.find(FDNameStr);
  if (I == InlinedSystemFunctions.end())
    return false;
  return ExistingSystemFunctions.count((*I).second);
}

void RemoveUnusedFunction::addOneMemberSpecialization(
       const FunctionDecl *FD, const FunctionDecl *Member)
{
  MemberSpecializationSet *S =
    MemberToSpecs[Member->getCanonicalDecl()];
  if (S == NULL) {
    S = new MemberSpecializationSet();
    MemberToSpecs[Member->getCanonicalDecl()] = S;
  }
  S->insert(FD);
}

void RemoveUnusedFunction::createFuncToExplicitSpecs(const FunctionDecl *FD)
{
  MemberSpecializationSet *S =
    FuncToExplicitSpecs[FD->getCanonicalDecl()];
  if (S == NULL) {
    S = new MemberSpecializationSet();
    FuncToExplicitSpecs[FD->getCanonicalDecl()] = S;
  }
}

void RemoveUnusedFunction::addFuncToExplicitSpecs(const FunctionDecl *FD)
{
  TransAssert((FD->getTemplateSpecializationKind()
               == TSK_ExplicitSpecialization) &&
              "Invalid template specialization kind!");
  const FunctionTemplateDecl *FTD = FD->getPrimaryTemplate();
  TransAssert(FTD && "NULL FunctionTemplateDecl!");
  const FunctionDecl *TemplatedFD = FTD->getTemplatedDecl();
  MemberSpecializationSet *S =
    FuncToExplicitSpecs[TemplatedFD->getCanonicalDecl()];
  if (S != NULL) {
    S->insert(FD);
  }
}

RemoveUnusedFunction::~RemoveUnusedFunction()
{
  for (MemberToSpecializationMap::iterator I = MemberToSpecs.begin(),
       E = MemberToSpecs.end(); I != E; ++I) {
    delete ((*I).second);
  }
  for (MemberToSpecializationMap::iterator I = MemberToInstantiations.begin(),
       E = MemberToInstantiations.end(); I != E; ++I) {
    delete ((*I).second);
  }
  delete AnalysisVisitor;
  delete VisitorWrapper;
}

