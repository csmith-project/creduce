//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef TEMPLATE_NON_TYPE_ARG_TO_INT_H
#define TEMPLATE_NON_TYPE_ARG_TO_INT_H

#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class Expr;
  class TemplateDecl;
  class TemplateArgumentLoc;
  class TemplateArgument;
  class TemplateSpecializationTypeLoc;
}

namespace clang_delta_common_visitor {
  template<typename T, typename Trans> class CommonTemplateArgumentVisitor;
}
class TemplateNonTypeArgToIntASTVisitor;

class TemplateNonTypeArgToInt : public Transformation {
friend class TemplateNonTypeArgToIntASTVisitor;
friend class clang_delta_common_visitor::CommonTemplateArgumentVisitor
               <TemplateNonTypeArgToIntASTVisitor, 
                TemplateNonTypeArgToInt>;

public:
  TemplateNonTypeArgToInt(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      TheExpr(NULL),
      IntString("1")
  {}

  ~TemplateNonTypeArgToInt();

private:

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void handleTemplateArgumentLocs(const clang::TemplateDecl *D, 
                                  const clang::TemplateArgumentLoc *TAL, 
                                  unsigned NumArgs);

  void handleOneTemplateArgumentLoc(const clang::TemplateArgumentLoc &ArgLoc);

  void handleTemplateSpecializationTypeLoc(
         const clang::TemplateSpecializationTypeLoc &TLoc);

  bool isValidTemplateArgument(const clang::TemplateArgument &Arg);

  TemplateNonTypeArgToIntASTVisitor *CollectionVisitor;

  clang::Expr *TheExpr;

  std::string IntString;

  // Unimplemented
  TemplateNonTypeArgToInt();

  TemplateNonTypeArgToInt(const TemplateNonTypeArgToInt &);

  void operator=(const TemplateNonTypeArgToInt &);
};

#endif
