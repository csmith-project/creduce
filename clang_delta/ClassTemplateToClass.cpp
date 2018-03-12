//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2014, 2015, 2017 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "ClassTemplateToClass.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg = 
"Change a class template to a class if this class template: \n\
   * has only one parameter, and \n\
   * the parameter is unused. \n";

static RegisterTransformation<ClassTemplateToClass>
         Trans("class-template-to-class", DescriptionMsg);

class ClassTemplateToClassASTVisitor : public 
  RecursiveASTVisitor<ClassTemplateToClassASTVisitor> {

public:
  explicit ClassTemplateToClassASTVisitor(ClassTemplateToClass *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitClassTemplateDecl(ClassTemplateDecl *D);

private:
  ClassTemplateToClass *ConsumerInstance;

};

class ClassTemplateToClassSpecializationTypeRewriteVisitor : public 
  RecursiveASTVisitor<ClassTemplateToClassSpecializationTypeRewriteVisitor> {

public:
  explicit ClassTemplateToClassSpecializationTypeRewriteVisitor(
             ClassTemplateToClass *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitTemplateSpecializationTypeLoc(TemplateSpecializationTypeLoc Loc);

private:

  ClassTemplateToClass *ConsumerInstance;
};

namespace {

class TemplateParameterTypeVisitor : public 
  RecursiveASTVisitor<TemplateParameterTypeVisitor> {

public:
  typedef llvm::SmallPtrSet<TemplateTypeParmDecl *, 8> TypeParmDeclSet;

  typedef llvm::SmallPtrSet<TemplateName *, 8> TemplateNameSet;

  ~TemplateParameterTypeVisitor(void) {
    for (TemplateNameSet::iterator I = TmplNames.begin(), E = TmplNames.end();
         I != E; ++I)
      delete (*I);
  }

  explicit TemplateParameterTypeVisitor(ASTContext *Ctx) 
    : Context(Ctx)
  { }

  bool VisitTemplateTypeParmType(TemplateTypeParmType *Ty);

  bool VisitTemplateSpecializationType(TemplateSpecializationType *Ty);

  bool isAUsedParameter(NamedDecl *ND);

private:
  TypeParmDeclSet ParmDecls;

  TemplateNameSet TmplNames;

  ASTContext *Context;
};

bool TemplateParameterTypeVisitor::VisitTemplateTypeParmType(
       TemplateTypeParmType *Ty)
{
  TemplateTypeParmDecl *D = Ty->getDecl();
  ParmDecls.insert(D);
  return true;
}

bool TemplateParameterTypeVisitor::VisitTemplateSpecializationType(
       TemplateSpecializationType *Ty)
{
  TemplateName Name = Ty->getTemplateName();
  if (Name.getKind() != TemplateName::Template)
    return true;

  TemplateName *NewName = new TemplateName(Name.getAsTemplateDecl());
  TmplNames.insert(NewName);
  return true;
}

bool TemplateParameterTypeVisitor::isAUsedParameter(NamedDecl *ND)
{
  if (TemplateTypeParmDecl *ParmD = dyn_cast<TemplateTypeParmDecl>(ND)) {
    return ParmDecls.count(ParmD);
  }

  if (TemplateTemplateParmDecl *ParmD = 
      dyn_cast<TemplateTemplateParmDecl>(ND)) {
    TemplateName Name(ParmD);
    for (TemplateNameSet::iterator I = TmplNames.begin(), E = TmplNames.end();
         I != E; ++I) {
      if (Context->hasSameTemplateName(*(*I), Name)) 
        return true;
    }
    return false;
  }

  TransAssert(0 && "Uncatched Template Parameter Kind!");
  return false;
}

}

bool ClassTemplateToClassASTVisitor::VisitClassTemplateDecl(
       ClassTemplateDecl *D)
{
  if (ConsumerInstance->isInIncludedFile(D))
    return true;

  ClassTemplateDecl *CanonicalD = D->getCanonicalDecl();
  if (ConsumerInstance->VisitedDecls.count(CanonicalD))
    return true;

  ConsumerInstance->VisitedDecls.insert(CanonicalD);
  if (ConsumerInstance->isValidClassTemplateDecl(D)) {
    ConsumerInstance->ValidInstanceNum++;
    if (ConsumerInstance->ValidInstanceNum == 
        ConsumerInstance->TransformationCounter) {
      ConsumerInstance->TheClassTemplateDecl = CanonicalD;
      ConsumerInstance->TheTemplateName = new TemplateName(CanonicalD);
    }
  }
  return true;
}

bool ClassTemplateToClassSpecializationTypeRewriteVisitor::
       VisitTemplateSpecializationTypeLoc(TemplateSpecializationTypeLoc Loc)
{
  const TemplateSpecializationType *Ty = 
    dyn_cast<TemplateSpecializationType>(Loc.getTypePtr());
  TransAssert(Ty && "Invalid TemplateSpecializationType!");

  TemplateName TmplName = Ty->getTemplateName();
  if (!ConsumerInstance->referToTheTemplateDecl(TmplName))
    return true;

  SourceLocation TmplKeyLoc = Loc.getTemplateKeywordLoc();
  if (TmplKeyLoc.isValid())
    ConsumerInstance->TheRewriter.RemoveText(TmplKeyLoc, 8);

  // it's necessary to check the validity of locations, otherwise
  // we will get assertion errors...
  // note that some implicit typeloc has been visited by Clang, e.g.
  // template < typename > struct A { };
  // struct B:A < int > {
  //  B (  ) { }
  // };
  // base initializer A<int> is not presented in the code but visited,
  // so, we need to make sure locations are valid.
  SourceLocation LAngleLoc = Loc.getLAngleLoc();
  if (LAngleLoc.isInvalid())
    return true;
  SourceLocation RAngleLoc = Loc.getRAngleLoc();
  if (RAngleLoc.isInvalid())
    return true;
  ConsumerInstance->TheRewriter.RemoveText(SourceRange(LAngleLoc,
                                                       RAngleLoc));
  return true;
}

void ClassTemplateToClass::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new ClassTemplateToClassASTVisitor(this);
  RewriteVisitor = 
    new ClassTemplateToClassSpecializationTypeRewriteVisitor(this);
}

void ClassTemplateToClass::HandleTranslationUnit(ASTContext &Ctx)
{
  if (TransformationManager::isCLangOpt() ||
      TransformationManager::isOpenCLLangOpt()) {
    ValidInstanceNum = 0;
  }
  else {
    CollectionVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
  }

  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  TransAssert(TheClassTemplateDecl && "NULL TheClassTemplateDecl!");
  TransAssert(RewriteVisitor && "NULL RewriteVisitor!");
  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);
  rewriteClassTemplateDecls();
  rewriteClassTemplatePartialSpecs();
  RewriteVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void ClassTemplateToClass::removeTemplateAndParameter(SourceLocation LocStart, 
       const TemplateParameterList *TPList)
{
  SourceLocation LocEnd = TPList->getRAngleLoc();
  TheRewriter.RemoveText(SourceRange(LocStart, LocEnd));
}

void ClassTemplateToClass::rewriteClassTemplateDecls(void)
{
  for (ClassTemplateDecl::redecl_iterator 
         I = TheClassTemplateDecl->redecls_begin(), 
         E = TheClassTemplateDecl->redecls_end();
       I != E; ++I) {
    const TemplateParameterList *TPList = (*I)->getTemplateParameters();
    SourceLocation LocStart = (*I)->getLocStart();
    removeTemplateAndParameter(LocStart, TPList);
  }
}

void ClassTemplateToClass::rewriteClassTemplatePartialSpecs(void)
{
  SmallVector<ClassTemplatePartialSpecializationDecl *, 10> PartialDecls;
  TheClassTemplateDecl->getPartialSpecializations(PartialDecls);

  for (SmallVector<ClassTemplatePartialSpecializationDecl *, 10>::iterator 
         I = PartialDecls.begin(), E = PartialDecls.end(); I != E; ++I) {
    const ClassTemplatePartialSpecializationDecl *PartialD = (*I);
    removeTemplateAndParameter(PartialD->getSourceRange().getBegin(), 
                               PartialD->getTemplateParameters());
    
    const ASTTemplateArgumentListInfo *ArgList = 
      PartialD->getTemplateArgsAsWritten();
    TransAssert(ArgList && "Invalid ArgList!");
    
    const TemplateArgumentLoc *ArgLocs = ArgList->getTemplateArgs();
    TransAssert(ArgLocs && "Invalid ArcLogs!");
    
    unsigned NumArgs = ArgList->NumTemplateArgs;
    TransAssert((NumArgs > 0) && "Invalid NumArgs!");
    
    const TemplateArgumentLoc FirstArgLoc = ArgLocs[0];
    SourceLocation StartLoc = FirstArgLoc.getSourceRange().getBegin();

    const TemplateArgumentLoc LastArgLoc = ArgLocs[NumArgs - 1];
    SourceRange LastRange = LastArgLoc.getSourceRange();
    SourceLocation EndLoc = RewriteHelper->getEndLocationUntil(LastRange, '>');
    
    RewriteHelper->removeTextFromLeftAt(SourceRange(StartLoc, EndLoc),
                                        '<', EndLoc);
  }
}

bool ClassTemplateToClass::isUsedNamedDecl(NamedDecl *ND, 
                                           Decl *D)
{
  TemplateParameterTypeVisitor ParamVisitor(Context);
  ParamVisitor.TraverseDecl(D);

  return ParamVisitor.isAUsedParameter(ND);
}

bool ClassTemplateToClass::hasUsedNameDecl(
       ClassTemplatePartialSpecializationDecl *PartialD)
{
  if (!PartialD->isCompleteDefinition())
    return false;

  llvm::SmallPtrSet<NamedDecl *, 8> Params;
  TemplateParameterList *PartialTPList = PartialD->getTemplateParameters();
  for (unsigned PI = 0; PI < PartialTPList->size(); ++PI) {
    NamedDecl *ND = PartialTPList->getParam(PI);
    if (dyn_cast<NonTypeTemplateParmDecl>(ND))
      continue;
    Params.insert(ND);  
  }

  TemplateParameterTypeVisitor ParamVisitor(Context);

  // Skip visiting parameters and arguments
  for (CXXRecordDecl::base_class_iterator I = PartialD->bases_begin(),
       E = PartialD->bases_end(); I != E; ++I) {
    ParamVisitor.TraverseType(I->getType());
  }

  DeclContext *Ctx = dyn_cast<DeclContext>(PartialD);
  for (DeclContext::decl_iterator DI = Ctx->decls_begin(), 
       DE = Ctx->decls_end(); DI != DE; ++DI) {
    ParamVisitor.TraverseDecl(*DI);
  }

  for (llvm::SmallPtrSet<NamedDecl *, 8>::iterator I = Params.begin(), 
       E = Params.end(); I != E; ++I) {
    if (ParamVisitor.isAUsedParameter(*I))
      return true;
  }
  return false;
}

bool ClassTemplateToClass::isValidClassTemplateDecl(ClassTemplateDecl *TmplD)
{
  TemplateParameterList *TPList = TmplD->getTemplateParameters();
  if (TPList->size() != 1)
    return false;

  CXXRecordDecl *CXXRD = TmplD->getTemplatedDecl();
  CXXRecordDecl *Def = CXXRD->getDefinition();
  if (!Def)
    return true;

  NamedDecl *ND = TPList->getParam(0);
  if (dyn_cast<NonTypeTemplateParmDecl>(ND))
    return true;

  if (isUsedNamedDecl(ND, Def))
    return false;

  SmallVector<ClassTemplatePartialSpecializationDecl *, 10> PartialDecls;
  TmplD->getPartialSpecializations(PartialDecls);
  for (SmallVector<ClassTemplatePartialSpecializationDecl *, 10>::iterator 
         I = PartialDecls.begin(), E = PartialDecls.end(); I != E; ++I) {
    if (hasUsedNameDecl(*I))
      return false;
  }

  return true;
}

bool ClassTemplateToClass::referToTheTemplateDecl(TemplateName TmplName)
{
  return Context->hasSameTemplateName(*TheTemplateName, TmplName);
}

ClassTemplateToClass::~ClassTemplateToClass(void)
{
  delete TheTemplateName;
  delete CollectionVisitor;
  delete RewriteVisitor;
}

