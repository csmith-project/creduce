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
  class FunctionTemplateDecl;
  class ClassTemplateDecl;
  class TemplateDecl;
  class TemplateArgumentList;
  class TemplateArgument;
  class QualType;
  class NamedDecl;
}

class InstantiateTemplateParamASTVisitor;
class InstantiateTemplateParamRewriteVisitor;

class InstantiateTemplateParam : public Transformation {
friend class InstantiateTemplateParamASTVisitor;
friend class InstantiateTemplateParamRewriteVisitor;

public:
  InstantiateTemplateParam(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      ParamRewriteVisitor(NULL),
      TheParameter(NULL),
      TheInstantiationString("")
  {}

  ~InstantiateTemplateParam();

private:

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void handleOneFunctionTemplateDecl(const clang::FunctionTemplateDecl *D);

  void handleOneClassTemplateDecl(const clang::ClassTemplateDecl *D);

  void handleOneTemplateSpecialization(
         const clang::TemplateDecl *D, 
         const clang::TemplateArgumentList & ArgList);

  bool getTemplateArgumentString(const clang::TemplateArgument &Arg, 
                                 std::string &Str);

  bool getTypeString(const clang::QualType &QT, std::string &ArgStr);

  InstantiateTemplateParamASTVisitor *CollectionVisitor;

  InstantiateTemplateParamRewriteVisitor *ParamRewriteVisitor;

  const clang::NamedDecl *TheParameter;

  std::string TheInstantiationString;

  // Unimplemented
  InstantiateTemplateParam();

  InstantiateTemplateParam(const InstantiateTemplateParam &);

  void operator=(const InstantiateTemplateParam &);
};

#endif
