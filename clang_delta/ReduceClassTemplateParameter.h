//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2016 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef REDUCE_CLASS_TEMPLATE_PARAMETER_H
#define REDUCE_CLASS_TEMPLATE_PARAMETER_H

#include "llvm/ADT/SmallPtrSet.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class ClassTemplateDecl;
  class NamedDecl;
  class TemplateName;
  class TemplateArgument;
  class ClassTemplatePartialSpecializationDecl;
  class TemplateParameterList;
}

class ReduceClassTemplateParameterASTVisitor;
class ReduceClassTemplateParameterRewriteVisitor;

class ReduceClassTemplateParameter : public Transformation {
friend class ReduceClassTemplateParameterASTVisitor;
friend class ReduceClassTemplateParameterRewriteVisitor;

public:
  ReduceClassTemplateParameter(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      ArgRewriteVisitor(NULL),
      TheClassTemplateDecl(NULL),
      hasDefaultArg(false),
      TheParameterIndex(0),
      TheTemplateName(NULL)
  {}

  ~ReduceClassTemplateParameter();

  void removeParameterByRange(clang::SourceRange Range,
                              const clang::TemplateParameterList *TPList,
                              unsigned Index);

private:
  typedef llvm::SmallPtrSet<const clang::ClassTemplateDecl *, 20> 
            ClassTemplateDeclSet;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void setDefaultArgFlag(const clang::NamedDecl *ND);

  bool isValidClassTemplateDecl(const clang::ClassTemplateDecl *D);

  void removeParameterFromDecl();

  void removeParameterFromMethods();

  void removeParameterFromPartialSpecs();

  void removeOneParameterFromPartialDecl(
         const clang::ClassTemplatePartialSpecializationDecl *PartialD,
         const clang::TemplateArgument &Arg);

  void removeOneParameterByArgExpression(
         const clang::ClassTemplatePartialSpecializationDecl *PartialD,
         const clang::TemplateArgument &Arg);

  void removeOneParameterByArgType(
         const clang::ClassTemplatePartialSpecializationDecl *PartialD,
         const clang::TemplateArgument &Arg);

  void removeOneParameterByArgTemplate(
         const clang::ClassTemplatePartialSpecializationDecl *PartialD,
         const clang::TemplateArgument &Arg);

  void removeArgumentFromSpecializations();

  bool referToTheTemplateDecl(clang::TemplateName TmplName);

  bool reducePartialSpec(
         const clang::ClassTemplatePartialSpecializationDecl *PartialD);

  bool isValidForReduction(
         const clang::ClassTemplatePartialSpecializationDecl *PartialD);

  bool referToAParameter(
         const clang::ClassTemplatePartialSpecializationDecl *PartialD,
         const clang::TemplateArgument &Arg);

  const clang::NamedDecl *getNamedDecl(const clang::TemplateArgument &Arg);

  ClassTemplateDeclSet VisitedDecls;

  ReduceClassTemplateParameterASTVisitor *CollectionVisitor;

  ReduceClassTemplateParameterRewriteVisitor *ArgRewriteVisitor;

  clang::ClassTemplateDecl *TheClassTemplateDecl;

  bool hasDefaultArg;

  unsigned TheParameterIndex;

  clang::TemplateName *TheTemplateName;

  // Unimplemented
  ReduceClassTemplateParameter();

  ReduceClassTemplateParameter(const ReduceClassTemplateParameter &);

  void operator=(const ReduceClassTemplateParameter &);
};

#endif
