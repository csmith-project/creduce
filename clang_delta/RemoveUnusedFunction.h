//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef REMOVE_UNUSED_FUNCTION_H
#define REMOVE_UNUSED_FUNCTION_H

#include <string>
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class FunctionDecl;
}

class RUFAnalysisVisitor;

class RemoveUnusedFunction : public Transformation {
friend class RUFAnalysisVisitor;

public:

  RemoveUnusedFunction(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      AnalysisVisitor(NULL),
      TheFunctionDecl(NULL)
  { }

  ~RemoveUnusedFunction(void);

private:
  
  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void removeFunctionDecl(void);

  RUFAnalysisVisitor *AnalysisVisitor;

  clang::FunctionDecl *TheFunctionDecl;

  // Unimplemented
  RemoveUnusedFunction(void);

  RemoveUnusedFunction(const RemoveUnusedFunction &);

  void operator=(const RemoveUnusedFunction &);
};
#endif
