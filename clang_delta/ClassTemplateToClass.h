//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef CLASS_TEMPLATE_TO_CLASS_H
#define CLASS_TEMPLATE_TO_CLASS_H

#include "Transformation.h"
#include "llvm/ADT/SmallPtrSet.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class ClassTemplateDecl;
  class NamedDecl;
  class CXXRecordDecl;
  class ClassTemplatePartialSpecializationDecl;
  class TemplateParameterList;
  class TemplateName;
}

class ClassTemplateToClassASTVisitor;
class ClassTemplateToClassSpecializationTypeRewriteVisitor;

class ClassTemplateToClass : public Transformation {
friend class ClassTemplateToClassASTVisitor;
friend class ClassTemplateToClassSpecializationTypeRewriteVisitor;

public:
  ClassTemplateToClass(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      RewriteVisitor(NULL),
      TheClassTemplateDecl(NULL),
      TheTemplateName(NULL)
  {}

  ~ClassTemplateToClass(void);

private:
  typedef llvm::SmallPtrSet<const clang::ClassTemplateDecl *, 20> 
            ClassTemplateDeclSet;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  bool isValidClassTemplateDecl(clang::ClassTemplateDecl *D);

  bool isUsedNamedDecl(clang::NamedDecl *ND, clang::Decl *Def);

  bool hasUsedNameDecl(clang::ClassTemplatePartialSpecializationDecl *PartialD);

  void removeTemplateAndParameter(clang::SourceLocation LocStart, 
         const clang::TemplateParameterList *TPList);

  void rewriteClassTemplateDecls(void);

  void rewriteClassTemplatePartialSpecs(void);

  bool referToTheTemplateDecl(clang::TemplateName TmplName);

  ClassTemplateDeclSet VisitedDecls;

  ClassTemplateToClassASTVisitor *CollectionVisitor;

  ClassTemplateToClassSpecializationTypeRewriteVisitor *RewriteVisitor;

  clang::ClassTemplateDecl *TheClassTemplateDecl;

  clang::TemplateName *TheTemplateName;

  // Unimplemented
  ClassTemplateToClass(void);

  ClassTemplateToClass(const ClassTemplateToClass &);
};

#endif
