//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2015, 2017 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "SimplifyRecursiveTemplateInstantiation.h"

#include "clang/Lex/Lexer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg = 
"This pass handles a special case where we have recursive template \
instantiations. For example, given the code blow: \n\
  C<B<C<B<C<B<D>, G<int> > > \n\
it will replace an outer instantiation of B with an inner one, e.g. B<D>.\n\
The the number of transformation instances counts from inside to \n\
outside. Currently we only handle class template instantiations. \n\
";

static RegisterTransformation<SimplifyRecursiveTemplateInstantiation>
         Trans("simplify-recursive-template-instantiation", DescriptionMsg);

namespace {
class LocalTemplateArgVisitor : public 
  RecursiveASTVisitor<LocalTemplateArgVisitor> {

public:
  LocalTemplateArgVisitor(const TemplateSpecializationTypeLoc &TLoc, 
                  SimplifyRecursiveTemplateInstantiation *Instance)
    : ParentTLoc(TLoc),
      ConsumerInstance(Instance)
  { }

  bool VisitTemplateSpecializationTypeLoc(
         TemplateSpecializationTypeLoc TLoc);

private:
  const TemplateSpecializationTypeLoc &ParentTLoc;

  SimplifyRecursiveTemplateInstantiation *ConsumerInstance;
};

bool LocalTemplateArgVisitor::VisitTemplateSpecializationTypeLoc(
       TemplateSpecializationTypeLoc TLoc)
{
  ConsumerInstance->handleInnerTemplateSpecializationTypeLoc(ParentTLoc, TLoc);
  return true;
}

} // end anon namespace

class SimplifyRecursiveTemplateInstantiationASTVisitor : public 
  RecursiveASTVisitor<SimplifyRecursiveTemplateInstantiationASTVisitor> {

public:
  explicit SimplifyRecursiveTemplateInstantiationASTVisitor(
             SimplifyRecursiveTemplateInstantiation *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitTemplateSpecializationTypeLoc(
         TemplateSpecializationTypeLoc TLoc);

private:
  SimplifyRecursiveTemplateInstantiation *ConsumerInstance;

};

bool SimplifyRecursiveTemplateInstantiationASTVisitor::
       VisitTemplateSpecializationTypeLoc(TemplateSpecializationTypeLoc TLoc)
{
  ConsumerInstance->handleTemplateSpecializationTypeLoc(TLoc);
  return true;
}

void SimplifyRecursiveTemplateInstantiation::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = 
    new SimplifyRecursiveTemplateInstantiationASTVisitor(this);
}

void
SimplifyRecursiveTemplateInstantiation::HandleTranslationUnit(ASTContext &Ctx)
{
  if (TransformationManager::isCLangOpt() ||
      TransformationManager::isOpenCLLangOpt()) {
    ValidInstanceNum = 0;
  }
  else {
    CollectionVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
    analyzeLocPairs();
  }

  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);
  rewriteTemplateArgument();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void SimplifyRecursiveTemplateInstantiation::analyzeLocPairs()
{
  for (SpecTypeLocPairQueue::reverse_iterator I = LocPairQueue.rbegin(),
       E = LocPairQueue.rend(); I != E; ++I) {
    ValidInstanceNum++;
    if (ValidInstanceNum == TransformationCounter)
      TheLocPair = (*I);
  }
}

void
SimplifyRecursiveTemplateInstantiation::handleTemplateSpecializationTypeLoc(
       const TemplateSpecializationTypeLoc &TLoc)
{
  if (isInIncludedFile(TLoc.getBeginLoc()))
    return;
  for (unsigned I = 0; I < TLoc.getNumArgs(); ++I) {
    TemplateArgumentLoc ArgLoc = TLoc.getArgLoc(I);
    if (ArgLoc.getLocation().isInvalid())
      continue;
    LocalTemplateArgVisitor ArgVisitor(TLoc, this);
    ArgVisitor.TraverseTemplateArgumentLoc(ArgLoc);
  }
}

const TemplateDecl *
SimplifyRecursiveTemplateInstantiation::getTemplateDeclFromSpecLoc(
    const TemplateSpecializationTypeLoc &TLoc)
{
  const Type *Ty = TLoc.getTypePtr();
  const TemplateSpecializationType *TST = 
    Ty->getAs<TemplateSpecializationType>();
  TemplateName TplName = TST->getTemplateName();
  return TplName.getAsTemplateDecl();
}

void SimplifyRecursiveTemplateInstantiation::
     handleInnerTemplateSpecializationTypeLoc(
       const TemplateSpecializationTypeLoc &ParentTLoc, 
       const TemplateSpecializationTypeLoc &TLoc)
{
  const TemplateDecl *ParentD = getTemplateDeclFromSpecLoc(ParentTLoc);
  TransAssert(ParentD && "NULL ParentD!");
  const TemplateDecl *D = getTemplateDeclFromSpecLoc(TLoc);
  TransAssert(D && "NULL Decl!");
  if (ParentD->getCanonicalDecl() != D->getCanonicalDecl())
    return;
  SpecTypeLocPair *P = new SpecTypeLocPair();
  TransAssert(P && "Failed to alloc SpecTypeLocPair!");
  P->push_back(ParentTLoc);
  P->push_back(TLoc);
  LocPairQueue.push_back(P);
}

void SimplifyRecursiveTemplateInstantiation::rewriteTemplateArgument()
{
  TransAssert(TheLocPair && "NULL TheLocPair!");
  TransAssert((TheLocPair->size() == 2) && "Invalid size of TheLocPair!");
  TemplateSpecializationTypeLoc TLoc = TheLocPair->pop_back_val();
  TemplateSpecializationTypeLoc ParentTLoc = TheLocPair->pop_back_val();

  SourceLocation LAngleLoc  = TLoc.getLAngleLoc();
  SourceLocation RAngleLoc  = TLoc.getRAngleLoc();
  SourceLocation ParentLAngleLoc = ParentTLoc.getLAngleLoc();
  SourceLocation ParentRAngleLoc = ParentTLoc.getRAngleLoc();
  std::string InnerStr = "";
  RewriteHelper->getStringBetweenLocs(InnerStr, LAngleLoc, RAngleLoc);
  TheRewriter.ReplaceText(SourceRange(ParentLAngleLoc, ParentRAngleLoc),
                           InnerStr + ">");
}

SimplifyRecursiveTemplateInstantiation::
  ~SimplifyRecursiveTemplateInstantiation()
{
  for (SpecTypeLocPairQueue::iterator I = LocPairQueue.begin(),
       E = LocPairQueue.end(); I != E; ++I) {
    delete (*I);
  } 
  delete CollectionVisitor;
}

