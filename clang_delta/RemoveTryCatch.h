//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// Copyright (c) 2012 Konstantin Tokarev <annulen@yandex.ru>
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef REMOVE_TRY_CATCH_H
#define REMOVE_TRY_CATCH_H

#include <string>
#include "llvm/ADT/DenseMap.h"
#include "Transformation.h"

namespace clang {
  class Stmt;
}

class RemoveTryCatchAnalysisVisitor;

class RemoveTryCatch : public Transformation {
friend class RemoveTryCatchAnalysisVisitor;

public:

  RemoveTryCatch(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      AnalysisVisitor(0),
      TheTryCatchStmt(0)
  { }

  ~RemoveTryCatch();

private:

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void removeStmt();

  RemoveTryCatchAnalysisVisitor *AnalysisVisitor;

  clang::Stmt *TheTryCatchStmt;

  // Unimplemented
  RemoveTryCatch();

  RemoveTryCatch(const RemoveTryCatch &);

  void operator=(const RemoveTryCatch &);
};
#endif
