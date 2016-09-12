//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2016 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef REMOVE_ARRAY_H
#define REMOVE_ARRAY_H

#include <utility>
#include "Transformation.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/SmallPtrSet.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class VarDecl;
  class DeclRefExpr;
}

class RemoveArrayCollectionVisitor;
class InvalidArraySubscriptExprVisitor;

class RemoveArray : public Transformation {
friend class RemoveArrayCollectionVisitor;
friend class InvalidArraySubscriptExprVisitor;

public:

  RemoveArray(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      TheArrayVarDecl(NULL),
      TheASEVec(NULL)
  { }

  ~RemoveArray(void);

private:

  typedef llvm::SmallVector<clang::ArraySubscriptExpr *, 10> 
    ArraySubscriptExprVector;

  typedef llvm::MapVector<const clang::VarDecl *, ArraySubscriptExprVector *> 
    VarDeclToArraySubscriptExprMap;

  typedef std::pair<clang::SourceLocation, clang::SourceLocation>
            BracketLocPair;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void doAnalysis(void);

  void doRewriting(void);

  void addOneArraySubscriptExpr(clang::ArraySubscriptExpr *ASE, 
                                const clang::DeclRefExpr *DRE);

  void deleteOneVarDecl(const clang::DeclRefExpr *DRE);

  void handleOneVarDecl(const clang::VarDecl *VD);

  void getBracketLocPair(const clang::VarDecl *VD, BracketLocPair &LocPair);

  const clang::VarDecl *getVarDeclFromArraySubscriptExpr(
                          const clang::ArraySubscriptExpr *ASE);

  VarDeclToArraySubscriptExprMap ValidVarToASEMap;
   
  RemoveArrayCollectionVisitor *CollectionVisitor;

  const clang::VarDecl *TheArrayVarDecl;

  ArraySubscriptExprVector *TheASEVec;

  // Unimplemented
  RemoveArray(void);

  RemoveArray(const RemoveArray &);

  void operator=(const RemoveArray &);
};

#endif
