//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef RENAME_CLASS_H
#define RENAME_CLASS_H

#include <string>
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
}

class RenameClassASTVisitor;
class RenameClassRewriteVisitor;

class RenameClass : public Transformation {
friend class RenameClassASTVisitor;
friend class RenameClassRewriteVisitor;

public:

  RenameClass(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      RewriteVisitor(NULL),
      TheCXXRecordDecl(NULL),
      ConflictingRD(NULL),
      NewName('A'),
      NewNameStr(""),
      BackupName(""),
      CurrentName('A'),
      MaxInheritanceLevel(0)
  { }

  ~RenameClass(void);

private:
  
  typedef llvm::DenseMap<const clang::CXXRecordDecl *, unsigned> 
            RecordToInheritanceLevelMap;

  typedef llvm::SmallPtrSet<const clang::CXXRecordDecl *, 5> CXXRecordDeclSet;

  typedef llvm::DenseMap<unsigned, CXXRecordDeclSet *> 
            InheritanceLevelToRecordsMap;

  typedef llvm::DenseMap<char, const clang::CXXRecordDecl *> 
            NameToRecordDeclMap;

  typedef llvm::SmallSet<std::string, 20> ClassNameSet;

  virtual void Initialize(clang::ASTContext &context);

  virtual bool HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  bool doAnalysis(void);

  bool isSpecialRecordDecl(const clang::CXXRecordDecl *CXXRD);

  void analyzeOneRecordDecl(const clang::CXXRecordDecl *CXXRD);

  bool isValidName(const std::string &Name);

  bool isReservedName(char C);

  void incCurrentName(void);

  void incValidInstance(const clang::CXXRecordDecl *CXXRD);

  bool matchCurrentName(const std::string &Name);

  void setBackupName(ClassNameSet &AllClassNames);

  bool getNewName(const clang::CXXRecordDecl *CXXRD, std::string &NewName);

  bool getNewNameByName(const std::string &Name, std::string &NewName);

  const clang::CXXRecordDecl *getBaseDeclFromType(const clang::Type *Ty);

  void rewriteClassName(const clang::CXXRecordDecl *RD,
                        clang::NestedNameSpecifier *NNS,
                        clang::NestedNameSpecifierLoc Loc);

  RecordToInheritanceLevelMap RecordToLevel;

  InheritanceLevelToRecordsMap LevelToRecords;

  NameToRecordDeclMap NameToRecord;

  RenameClassASTVisitor *CollectionVisitor;

  RenameClassRewriteVisitor *RewriteVisitor;

  const clang::CXXRecordDecl *TheCXXRecordDecl;

  const clang::CXXRecordDecl *ConflictingRD;

  char NewName;

  std::string NewNameStr;

  // used to replace the conflicting class name
  std::string BackupName;

  char CurrentName;

  unsigned MaxInheritanceLevel;

  // Unimplemented
  RenameClass(void);

  RenameClass(const RenameClass &);

  void operator=(const RenameClass &);
};
#endif
