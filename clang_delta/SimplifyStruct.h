//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef SIMPLIFY_STRUCT_H
#define SIMPLIFY_STRUCT_H

#include "llvm/ADT/SmallPtrSet.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class RecordDecl;
  class FieldDecl;
}

class SimplifyStructCollectionVisitor;
class SimplifyStructRewriteVisitor;

class SimplifyStruct : public Transformation {

friend class SimplifyStructCollectionVisitor;
friend class SimplifyStructRewriteVisitor;

public:

  SimplifyStruct(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      RewriteVisitor(NULL),
      TheRecordDecl(NULL),
      ReplacingRecordDecl(NULL),
      VolatileField(false),
      ConstField(false)
  { }

  ~SimplifyStruct(void);

private:
  
  typedef llvm::SmallPtrSet<void *, 10> LocPtrSet;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void setQualifierFlags(const clang::FieldDecl *FD);

  LocPtrSet VisitedLocs;

  LocPtrSet VisitedVarDeclLocs;

  SimplifyStructCollectionVisitor *CollectionVisitor;

  SimplifyStructRewriteVisitor *RewriteVisitor;

  const clang::RecordDecl *TheRecordDecl;

  const clang::RecordDecl *ReplacingRecordDecl;

  bool VolatileField;

  bool ConstField;

  // Unimplemented
  SimplifyStruct(void);

  SimplifyStruct(const SimplifyStruct &);

  void operator=(const SimplifyStruct &);
};
#endif
