//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2018 The University of Utah
// Copyright (c) 2012 Konstantin Tokarev <annulen@yandex.ru>
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef REMOVE_UNUSED_MEMBER_VALUE_H
#define REMOVE_UNUSED_MEMBER_VALUE_H

#include <string>
#include "llvm/ADT/DenseMap.h"
#include "Transformation.h"

namespace clang {
  class EnumConstantDecl;
}

class RemoveEnumMemberValueAnalysisVisitor;

class RemoveEnumMemberValue : public Transformation {
friend class RemoveEnumMemberValueAnalysisVisitor;

public:

  RemoveEnumMemberValue(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      AnalysisVisitor(0),
      TheEnumConstantDecl(0)
  { }

  ~RemoveEnumMemberValue();

private:

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void removeEnumValue();

  RemoveEnumMemberValueAnalysisVisitor *AnalysisVisitor;

  clang::EnumConstantDecl *TheEnumConstantDecl;

  // Unimplemented
  RemoveEnumMemberValue();

  RemoveEnumMemberValue(const RemoveEnumMemberValue &);

  void operator=(const RemoveEnumMemberValue &);
};
#endif
