//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2014, 2016 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef COMMON_TEMPLATE_ARGUMENT_VISITOR_H
#define COMMON_TEMPLATE_ARGUMENT_VISITOR_H

#include "clang/AST/RecursiveASTVisitor.h"

namespace clang_delta_common_visitor {

template<typename T, typename Trans>
class CommonTemplateArgumentVisitor : public clang::RecursiveASTVisitor<T> {

public:
  explicit CommonTemplateArgumentVisitor(Trans *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitClassTemplatePartialSpecializationDecl(
         clang::ClassTemplatePartialSpecializationDecl *D);

  bool VisitTemplateSpecializationTypeLoc(
         clang::TemplateSpecializationTypeLoc TLoc);

  bool VisitFunctionDecl(clang::FunctionDecl *D);

  bool VisitDeclRefExpr(clang::DeclRefExpr *E);

protected:
  Trans *ConsumerInstance;

};

template<typename T, typename Trans>
bool CommonTemplateArgumentVisitor<T, Trans>::
       VisitTemplateSpecializationTypeLoc(
         clang::TemplateSpecializationTypeLoc TLoc)
{
  ConsumerInstance->handleTemplateSpecializationTypeLoc(TLoc);
  return true;
}

template<typename T, typename Trans>
bool CommonTemplateArgumentVisitor<T, Trans>::
       VisitClassTemplatePartialSpecializationDecl(
         clang::ClassTemplatePartialSpecializationDecl *D)
{
  ConsumerInstance->handleTemplateArgumentLocs(
    D->getSpecializedTemplate(),
    D->getTemplateArgsAsWritten()->getTemplateArgs(),
    D->getTemplateArgsAsWritten()->NumTemplateArgs);
  return true;
}

template<typename T, typename Trans>
bool CommonTemplateArgumentVisitor<T, Trans>::VisitFunctionDecl(
       clang::FunctionDecl *D)
{
  const clang::FunctionTemplateSpecializationInfo *FTSI =
          D->getTemplateSpecializationInfo();
  if (!FTSI)
    return true;

  if ((FTSI->getTemplateSpecializationKind() == clang::TSK_Undeclared) ||
      (FTSI->getTemplateSpecializationKind() == 
         clang::TSK_ImplicitInstantiation))
    return true;

  if (const clang::ASTTemplateArgumentListInfo *TALI =
        FTSI->TemplateArgumentsAsWritten) {
    ConsumerInstance->handleTemplateArgumentLocs(
      D->getPrimaryTemplate(),
      TALI->getTemplateArgs(),
      TALI->NumTemplateArgs);
  }

  return true;
}

template<typename T, typename Trans>
bool CommonTemplateArgumentVisitor<T, Trans>::VisitDeclRefExpr(
       clang::DeclRefExpr *E)
{
  const clang::ValueDecl *VD = E->getDecl();
  const clang::TemplateDecl *TempD = NULL;
  if (const clang::FunctionDecl *FD = 
      clang::dyn_cast<clang::FunctionDecl>(VD)) {
    TempD = FD->getDescribedFunctionTemplate();
  }
  else {
    const clang::Type *Ty = VD->getType().getTypePtr();
    if (const clang::ArrayType *AT = clang::dyn_cast<clang::ArrayType>(Ty)) {
      Ty = AT->getElementType().getTypePtr();
    }
    if (Ty->isPointerType() || Ty->isReferenceType())
      Ty = ConsumerInstance->getBasePointerElemType(Ty);
    const clang::CXXRecordDecl *CXXRD = ConsumerInstance->getBaseDeclFromType(Ty);
    if (!CXXRD)
      return true;
    TempD = CXXRD->getDescribedClassTemplate();
  }
  if (!TempD)
    return true;

  ConsumerInstance->handleTemplateArgumentLocs(TempD, E->getTemplateArgs(), 
                                               E->getNumTemplateArgs());
  return true;
}

#endif
} // end namespace clang_delta_common_visitor
