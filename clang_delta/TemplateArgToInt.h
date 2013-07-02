//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef TEMPLATE_ARG_TO_INT_H
#define TEMPLATE_ARG_TO_INT_H

#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/DenseMap.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class TemplateDecl;
  class NamedDecl;
  class TemplateArgumentLoc;
  class TemplateSpecializationTypeLoc;
  class TypeSourceInfo;
  class Type;
  class SubstTemplateTypeParmType;
}

class TemplateArgToIntASTVisitor;
class TemplateArgToIntArgCollector;
class TemplateGlobalInvalidParameterVisitor;

namespace clang_delta_common_visitor {
  template<typename T, typename Trans> class CommonTemplateArgumentVisitor;
}

class TemplateArgToInt : public Transformation {
friend class TemplateArgToIntASTVisitor;
friend class TemplateArgToIntArgCollector;
friend class TemplateInvalidParameterVisitor;
friend class TemplateGlobalInvalidParameterVisitor;
friend class clang_delta_common_visitor::
               CommonTemplateArgumentVisitor<TemplateArgToIntArgCollector, 
                                             TemplateArgToInt>;

public:
  TemplateArgToInt(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      ArgCollector(NULL),
      GlobalParamFilter(NULL)
  {}

  ~TemplateArgToInt();

private:

  typedef llvm::SmallPtrSet<const clang::TemplateDecl *, 10> TemplateDeclSet;

  typedef llvm::SmallPtrSet<const clang::NamedDecl *, 8> TemplateParameterSet;

  typedef llvm::SmallSet<unsigned, 8> TemplateParameterIdxSet;

  typedef llvm::DenseMap<const clang::TemplateDecl *, 
                         TemplateParameterIdxSet *> TemplateDeclToParamIdxMap;

  typedef llvm::DenseMap<const clang::NamedDecl *, 
            const clang::TemplateDecl *> ParameterToTemplateDeclMap;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void collectInvalidParamIdx(const clang::TemplateDecl *D,
                              TemplateParameterIdxSet &InvalidParamIdx);

  void handleOneTemplateDecl(const clang::TemplateDecl *D);

  void handleTemplateArgumentLocs(const clang::TemplateDecl *D, 
                                  const clang::TemplateArgumentLoc *TAL, 
                                  unsigned NumArgs);

  void handleOneTemplateArgumentLoc(const clang::TemplateArgumentLoc &ArgLoc);

  void handleTemplateSpecializationTypeLoc(
         const clang::TemplateSpecializationTypeLoc &TLoc);

  void handleOneType(const clang::Type *Ty);

  const clang::SubstTemplateTypeParmType *
  getSubstTemplateTypeParmType(const clang::Type *Ty);

  void rewriteTemplateArgument();

  TemplateDeclToParamIdxMap DeclToParamIdx;

  ParameterToTemplateDeclMap ParamToTemplateDecl;

  TemplateArgToIntASTVisitor *CollectionVisitor;

  TemplateArgToIntArgCollector *ArgCollector;

  TemplateGlobalInvalidParameterVisitor *GlobalParamFilter;

  clang::TypeSourceInfo *TheTypeSourceInfo;

  // Unimplemented
  TemplateArgToInt();

  TemplateArgToInt(const TemplateArgToInt &);

  void operator=(const TemplateArgToInt &);
};

#endif
