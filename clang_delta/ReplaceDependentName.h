//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef REPLACE_DEPENDENT_NAME_H
#define REPLACE_DEPENDENT_NAME_H

#include <string>
#include "clang/Basic/SourceLocation.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class DependentNameTypeLoc;
  class ElaboratedTypeLoc;
}

class ReplaceDependentNameCollectionVisitor;

class ReplaceDependentName : public Transformation {
friend class ReplaceDependentNameCollectionVisitor;

public:
  ReplaceDependentName(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      TheTyName(""),
      NeedTypenameKeyword(false)
  {}

  ~ReplaceDependentName();

private:

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void handleOneDependentNameTypeLoc(const clang::DependentNameTypeLoc &TLoc);

  void handleOneElaboratedTypeLoc(const clang::ElaboratedTypeLoc &TLoc);

  void rewriteDependentName();

  clang::SourceLocation getElaboratedTypeLocBegin(
    const clang::ElaboratedTypeLoc &TLoc);

  ReplaceDependentNameCollectionVisitor *CollectionVisitor;

  std::string TheTyName;

  bool NeedTypenameKeyword;

  clang::SourceLocation TheLocBegin;

  clang::SourceLocation TheNameLocEnd;

  // Unimplemented
  ReplaceDependentName();

  ReplaceDependentName(const ReplaceDependentName &);

  void operator=(const ReplaceDependentName &);
};

#endif

