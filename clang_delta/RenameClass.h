//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef RENAME_CLASS_H
#define RENAME_CLASS_H

#include <string>
#include <set>
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallSet.h"
#include "clang/AST/NestedNameSpecifier.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class CXXRecordDecl;
  class Type;
  class TemplateSpecializationType;
}

class RenameClassASTVisitor;
class RenameClassRewriteVisitor;

class RenameClass : public Transformation {
friend class RenameClassASTVisitor;

public:

  RenameClass(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      RewriteVisitor(NULL),
      TheCXXRecordDecl(NULL),
      NewNameStr(""),
      CurrentName('A'),
      MaxInheritanceLevel(0)
  { }

  ~RenameClass(void);

private:
  
  typedef llvm::DenseMap<const clang::CXXRecordDecl *, unsigned> 
            RecordToInheritanceLevelMap;

  typedef llvm::SmallPtrSet<const clang::CXXRecordDecl *, 15> CXXRecordDeclSet;

  typedef llvm::DenseMap<unsigned, CXXRecordDeclSet *> 
            InheritanceLevelToRecordsMap;

  typedef llvm::SmallSet<char, 20> NameCharSet;

  typedef std::set<std::string> ClassNameSet;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void doAnalysis(void);

  void analyzeOneRecordDecl(const clang::CXXRecordDecl *CXXRD);

  void addOneRecordDecl(const clang::CXXRecordDecl *CanonicalRD, 
                        unsigned Level);

  bool isValidName(const std::string &Name);

  bool isReservedName(char C);

  void incValidInstance(const clang::CXXRecordDecl *CXXRD);

  RecordToInheritanceLevelMap RecordToLevel;

  InheritanceLevelToRecordsMap LevelToRecords;

  CXXRecordDeclSet UsedNameDecls;

  NameCharSet UsedNames;

  RenameClassASTVisitor *CollectionVisitor;

  RenameClassRewriteVisitor *RewriteVisitor;

  const clang::CXXRecordDecl *TheCXXRecordDecl;

  std::string NewNameStr;

  char CurrentName;

  unsigned MaxInheritanceLevel;

  // Unimplemented
  RenameClass(void);

  RenameClass(const RenameClass &);

  void operator=(const RenameClass &);
};
#endif
