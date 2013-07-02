//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef INSTANTIATE_TEMPLATE_TYPE_PARAM_TO_INT_H
#define INSTANTIATE_TEMPLATE_TYPE_PARAM_TO_INT_H

#include "llvm/ADT/SmallPtrSet.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class TemplateDecl;
  class NamedDecl;
}

class InstantiateTemplateTypeParamToIntASTVisitor;
class InstantiateTemplateTypeParamToIntRewriteVisitor;

class InstantiateTemplateTypeParamToInt : public Transformation {
friend class InstantiateTemplateTypeParamToIntASTVisitor;
friend class InstantiateTemplateTypeParamToIntRewriteVisitor;
friend class TemplateParameterFilterVisitor;

public:
  InstantiateTemplateTypeParamToInt(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      ParamRewriteVisitor(NULL),
      TheParameter(NULL)
  {}

  ~InstantiateTemplateTypeParamToInt();

private:

  typedef llvm::SmallPtrSet<const clang::TemplateDecl *, 10> TemplateDeclSet;

  typedef llvm::SmallPtrSet<const clang::NamedDecl *, 8> TemplateParameterSet;

  typedef llvm::SmallPtrSet<void *, 10> LocPtrSet;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void handleOneTemplateDecl(const clang::TemplateDecl *D);

  void filterInvalidParams(const clang::TemplateDecl *D, 
                           TemplateParameterSet &Params);

  TemplateDeclSet VisitedTemplateDecls;

  LocPtrSet VisitedLocs;

  InstantiateTemplateTypeParamToIntASTVisitor *CollectionVisitor;

  InstantiateTemplateTypeParamToIntRewriteVisitor *ParamRewriteVisitor;

  const clang::NamedDecl *TheParameter;

  // Unimplemented
  InstantiateTemplateTypeParamToInt();

  InstantiateTemplateTypeParamToInt(const InstantiateTemplateTypeParamToInt &);

  void operator=(const InstantiateTemplateTypeParamToInt &);
};

#endif
