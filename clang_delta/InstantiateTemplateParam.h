//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef INSTANTIATE_TEMPLATE_PARAM_H
#define INSTANTIATE_TEMPLATE_PARAM_H

#include "llvm/ADT/SmallPtrSet.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class TemplateDecl;
  class NamedDecl;
}

class InstantiateTemplateParamASTVisitor;
class InstantiateTemplateParamRewriteVisitor;

class InstantiateTemplateParam : public Transformation {
friend class InstantiateTemplateParamASTVisitor;
friend class InstantiateTemplateParamRewriteVisitor;
friend class TemplateParameterFilterVisitor;

public:
  InstantiateTemplateParam(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      ParamRewriteVisitor(NULL),
      TheParameter(NULL)
  {}

  ~InstantiateTemplateParam();

private:

  typedef llvm::SmallPtrSet<const clang::TemplateDecl *, 10> TemplateDeclSet;

  typedef llvm::SmallPtrSet<const clang::NamedDecl *, 8> TemplateParameterSet;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void handleOneTemplateDecl(const clang::TemplateDecl *D);

  void filterInvalidParams(const clang::TemplateDecl *D, 
                           TemplateParameterSet &Params);

  TemplateDeclSet VisitedTemplateDecls;

  InstantiateTemplateParamASTVisitor *CollectionVisitor;

  InstantiateTemplateParamRewriteVisitor *ParamRewriteVisitor;

  const clang::NamedDecl *TheParameter;

  // Unimplemented
  InstantiateTemplateParam();

  InstantiateTemplateParam(const InstantiateTemplateParam &);

  void operator=(const InstantiateTemplateParam &);
};

#endif
