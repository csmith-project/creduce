//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2016 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef REPLACE_UNDEFINED_FUNCTION_H
#define REPLACE_UNDEFINED_FUNCTION_H

#include <string>
#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/SetVector.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class FunctionDecl;
}

class ReplaceUndefFuncCollectionVisitor;
class ReplaceUndefFuncRewriteVisitor;

class ReplaceUndefinedFunction : public Transformation {
friend class ReplaceUndefFuncCollectionVisitor;
friend class ReplaceUndefFuncRewriteVisitor;

public:

  ReplaceUndefinedFunction(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      RewriteVisitor(NULL),
      ReplacingFunctionDecl(NULL),
      ReplacedFunctionDecl(NULL)
  { }

  ~ReplaceUndefinedFunction(void);

private:
  
  typedef llvm::SetVector<const clang::FunctionDecl *>
            FunctionDeclSet;

  typedef llvm::MapVector<const clang::FunctionDecl *, FunctionDeclSet *>
            FunctionSetMap;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void handleOneFunctionDecl(const clang::FunctionDecl *FD);

  void doAnalysis(void);

  FunctionSetMap ReplaceableFunctions;

  ReplaceUndefFuncCollectionVisitor *CollectionVisitor;

  ReplaceUndefFuncRewriteVisitor *RewriteVisitor;

  const clang::FunctionDecl *ReplacingFunctionDecl;
  
  const clang::FunctionDecl *ReplacedFunctionDecl;

  // Unimplemented
  ReplaceUndefinedFunction(void);

  ReplaceUndefinedFunction(const ReplaceUndefinedFunction &);

  void operator=(const ReplaceUndefinedFunction &);
};
#endif

