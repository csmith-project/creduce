//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef REPLACE_DEPENDENT_TYPEDEF_H
#define REPLACE_DEPENDENT_TYPEDEF_H

#include <string>
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class QualType;
  class Type;
  class TypedefDecl;
  class StringRef;
  class CXXRecordDecl;
  class TemplateArgument;
  class TemplateTypeParmType;
}

class ReplaceDependentTypedefCollectionVisitor;

class ReplaceDependentTypedef : public Transformation {
friend class ReplaceDependentTypedefCollectionVisitor;

public:
  ReplaceDependentTypedef(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      TheTyName(""),
      TheTypedefDecl(NULL),
      NeedTypenameKeyword(false)
  {}

  ~ReplaceDependentTypedef();

private:

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void handleOneTypedefDecl(const clang::TypedefDecl *D);

  bool getTypeString(const clang::QualType &QT, 
                     std::string &Str,
                     bool &Typename);

  bool isValidType(const clang::QualType &QT);

  bool getTypedefString(const llvm::StringRef &Name,
                        const clang::CXXRecordDecl *CXXRD,
                        const clang::TemplateArgument *Args,
                        unsigned NumArgs,
                        std::string &Str,
                        bool &Typename);

  bool getDependentNameString(const clang::Type *Ty,
                              const clang::TemplateArgument *Args,
                              unsigned NumArgs,
                              std::string &Str,
                              bool &Typename);

  bool getTemplateTypeParmString(const clang::TemplateTypeParmType *ParmTy,
                                const clang::TemplateArgument *Args,
                                unsigned NumArgs,
                                std::string &Str);

  void rewriteTypedefDecl();

  ReplaceDependentTypedefCollectionVisitor *CollectionVisitor;

  std::string TheTyName;

  const clang::TypedefDecl *TheTypedefDecl;

  bool NeedTypenameKeyword;

  // Unimplemented
  ReplaceDependentTypedef();

  ReplaceDependentTypedef(const ReplaceDependentTypedef &);

  void operator=(const ReplaceDependentTypedef &);
};

#endif

