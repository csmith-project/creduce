//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef SIMPLIFY_RECURSIVE_TEMPLATE_INSTANTIATION_H
#define SIMPLIFY_RECURSIVE_TEMPLATE_INSTANTIATION_H

#include "llvm/ADT/SmallVector.h"
#include "clang/AST/TypeLoc.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class TemplateDecl;
}

class SimplifyRecursiveTemplateInstantiationASTVisitor;

class SimplifyRecursiveTemplateInstantiation : public Transformation {
friend class SimplifyRecursiveTemplateInstantiationASTVisitor;

public:
  SimplifyRecursiveTemplateInstantiation(const char *TransName, 
                                         const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      TheLocPair(NULL)
  {}

  ~SimplifyRecursiveTemplateInstantiation();

  void handleInnerTemplateSpecializationTypeLoc(
         const clang::TemplateSpecializationTypeLoc &ParentTLoc, 
         const clang::TemplateSpecializationTypeLoc &TLoc);

private:

  typedef llvm::SmallVector<clang::TemplateSpecializationTypeLoc, 2> 
                  SpecTypeLocPair;

  typedef llvm::SmallVector<SpecTypeLocPair *, 20> 
            SpecTypeLocPairQueue;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void handleTemplateSpecializationTypeLoc(
         const clang::TemplateSpecializationTypeLoc &TLoc);

  const clang::TemplateDecl *
  getTemplateDeclFromSpecLoc(const clang::TemplateSpecializationTypeLoc &TLoc);

  void rewriteTemplateArgument();

  void analyzeLocPairs();

  SpecTypeLocPairQueue LocPairQueue;

  SimplifyRecursiveTemplateInstantiationASTVisitor *CollectionVisitor;

  SpecTypeLocPair *TheLocPair;

  // Unimplemented
  SimplifyRecursiveTemplateInstantiation();

  SimplifyRecursiveTemplateInstantiation(
    const SimplifyRecursiveTemplateInstantiation &);

  void operator=(const SimplifyRecursiveTemplateInstantiation &);
};

#endif
