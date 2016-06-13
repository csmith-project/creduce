//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2015 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef REPLACE_CLASS_WITH_BASE_TEMPLATE_SPEC_H
#define REPLACE_CLASS_WITH_BASE_TEMPLATE_SPEC_H

#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
}

class ReplaceClassWithBaseTemplateSpecVisitor;
class ReplaceClassWithBaseTemplateSpecRewriteVisitor;

class ReplaceClassWithBaseTemplateSpec : public Transformation {
friend class ReplaceClassWithBaseTemplateSpecVisitor;
friend class ReplaceClassWithBaseTemplateSpecRewriteVisitor;

public:
  ReplaceClassWithBaseTemplateSpec(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      RewriteVisitor(NULL),
      TheCXXRecord(NULL),
      TheBaseName("")
  { }

  ~ReplaceClassWithBaseTemplateSpec(void);

private:
  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void handleOneCXXRecordDecl(const clang::CXXRecordDecl *CXXRD);

  void removeBaseSpecifier(void);

  ReplaceClassWithBaseTemplateSpecVisitor *CollectionVisitor;

  ReplaceClassWithBaseTemplateSpecRewriteVisitor *RewriteVisitor;

  const clang::CXXRecordDecl *TheCXXRecord;

  std::string TheBaseName;

  // Unimplemented
  ReplaceClassWithBaseTemplateSpec(void);

  ReplaceClassWithBaseTemplateSpec(const ReplaceClassWithBaseTemplateSpec &);

  void operator=(const ReplaceClassWithBaseTemplateSpec &);
};

#endif

