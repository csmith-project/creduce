//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2015 The University of Utah
// Copyright (c) 2012 Konstantin Tokarev <annulen@yandex.ru>
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef REMOVE_UNUSED_VAR_H
#define REMOVE_UNUSED_VAR_H

#include <string>
#include "llvm/ADT/DenseMap.h"
#include "clang/AST/Decl.h"
#include "Transformation.h"

class RemoveUnusedEnumMemberAnalysisVisitor;

class RemoveUnusedEnumMember : public Transformation {
friend class RemoveUnusedEnumMemberAnalysisVisitor;

public:

  RemoveUnusedEnumMember(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      AnalysisVisitor(0),
      TheEnumDecl(0)
  { }

  ~RemoveUnusedEnumMember();

private:

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void removeEnumConstantDecl();

  RemoveUnusedEnumMemberAnalysisVisitor *AnalysisVisitor;

  clang::EnumDecl *TheEnumDecl;
  clang::EnumDecl::enumerator_iterator TheEnumIterator;

  // Unimplemented
  RemoveUnusedEnumMember();

  RemoveUnusedEnumMember(const RemoveUnusedEnumMember &);

  void operator=(const RemoveUnusedEnumMember &);
};
#endif
