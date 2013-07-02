//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef RENAME_FUN_H
#define RENAME_FUN_H

#include <string>
#include <set>
#include "llvm/ADT/DenseMap.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class FunctionDecl;
}

class RNFunCollectionVisitor;
class RenameFunVisitor;

class RenameFun : public Transformation {
friend class RNFunCollectionVisitor;
friend class RenameFunVisitor;

public:

  RenameFun(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      FunCollectionVisitor(NULL),
      RenameVisitor(NULL),
      FunNamePrefix("fn"),
      FunNamePostfix(0),
      HasValidFuns(false)
  { }

  ~RenameFun(void);

  virtual bool skipCounter(void) {
    return true;
  }

private:
  
  virtual void Initialize(clang::ASTContext &context);

  virtual bool HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  bool hasValidPostfix(const std::string &Name);

  void addFun(const clang::FunctionDecl *FD);

  bool isConsecutiveNumbersFromOne(void);

  bool isSpecialFun(const std::string &Name);

  bool hasValidFuns(void);

  RNFunCollectionVisitor *FunCollectionVisitor;

  RenameFunVisitor *RenameVisitor;

  llvm::DenseMap<const clang::FunctionDecl *, std::string> FunToNameMap;

  std::set<unsigned int> AllValidNumbers;

  const std::string FunNamePrefix;

  unsigned int FunNamePostfix;

  bool HasValidFuns;

  // Unimplemented
  RenameFun(void);

  RenameFun(const RenameFun &);

  void operator=(const RenameFun &);
};
#endif
