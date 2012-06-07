//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef MOVE_GLOBAL_VAR_H
#define MOVE_GLOBAL_VAR_H

#include <string>
#include "llvm/ADT/DenseMap.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class FunctionDecl;
}

class MoveGlobalVar : public Transformation {

public:

  MoveGlobalVar(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      TheFirstFunctionDecl(NULL),
      ThePrintfDecl(NULL),
      TheFirstDecl(NULL),
      TheDGRPointer(NULL)
  { }

  ~MoveGlobalVar(void);

private:
  
  virtual void Initialize(clang::ASTContext &context);

  virtual bool HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  bool isSpecialDecl(const std::string &Name);

  void handleFunctionDecl(const clang::FunctionDecl *FD);

  void handleOtherDecl(clang::DeclGroupRef DGR);

  void liftPrintfDecl(void);

  void liftOtherDecl(void);

  // The first FunctionDecl which is not printf
  const clang::FunctionDecl *TheFirstFunctionDecl;

  const clang::FunctionDecl *ThePrintfDecl;

  const clang::Decl *TheFirstDecl;

  void *TheDGRPointer;

  // Unimplemented
  MoveGlobalVar(void);

  MoveGlobalVar(const MoveGlobalVar &);

  void operator=(const MoveGlobalVar &);
};
#endif
