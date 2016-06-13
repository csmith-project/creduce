//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2016 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef TEMPLATE_NON_TYPE_ARG_TO_INT_H
#define TEMPLATE_NON_TYPE_ARG_TO_INT_H

#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/DenseMap.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class Expr;
  class TemplateDecl;
  class TemplateArgumentLoc;
  class TemplateArgument;
  class TemplateSpecializationTypeLoc;
  class NamedDecl;
  class ValueDecl;
}

namespace clang_delta_common_visitor {
  template<typename T, typename Trans> class CommonTemplateArgumentVisitor;
}
class TemplateNonTypeArgToIntASTVisitor;
class TemplateNonTypeArgToIntArgCollector;

class TemplateNonTypeArgToInt : public Transformation {
friend class TemplateNonTypeArgToIntASTVisitor;
friend class clang_delta_common_visitor::CommonTemplateArgumentVisitor
               <TemplateNonTypeArgToIntASTVisitor, 
                TemplateNonTypeArgToInt>;
friend class TemplateNonTypeArgToIntArgCollector;

public:
  TemplateNonTypeArgToInt(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      ArgCollector(NULL),
      TheExpr(NULL),
      TheValueDecl(NULL),
      IntString("1")
  {}

  ~TemplateNonTypeArgToInt();

private:

  typedef llvm::SmallSet<unsigned, 8> TemplateParameterIdxSet;

  typedef llvm::DenseMap<const clang::TemplateDecl *, 
                         TemplateParameterIdxSet *> TemplateDeclToParamIdxMap;
  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void handleOneTemplateDecl(const clang::TemplateDecl *D);

  bool isValidParameter(const clang::NamedDecl *ND);

  void handleTemplateArgumentLocs(const clang::TemplateDecl *D, 
                                  const clang::TemplateArgumentLoc *TAL, 
                                  unsigned NumArgs);

  void handleOneTemplateArgumentLoc(const clang::TemplateArgumentLoc &ArgLoc);

  void handleTemplateSpecializationTypeLoc(
         const clang::TemplateSpecializationTypeLoc &TLoc);

  bool isValidTemplateArgument(const clang::TemplateArgument &Arg);

  TemplateDeclToParamIdxMap DeclToParamIdx;

  TemplateNonTypeArgToIntASTVisitor *CollectionVisitor;

  TemplateNonTypeArgToIntArgCollector *ArgCollector;

  clang::Expr *TheExpr;

  const clang::ValueDecl *TheValueDecl;

  std::string IntString;

  // Unimplemented
  TemplateNonTypeArgToInt();

  TemplateNonTypeArgToInt(const TemplateNonTypeArgToInt &);

  void operator=(const TemplateNonTypeArgToInt &);
};

#endif
