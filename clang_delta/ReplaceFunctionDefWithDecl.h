//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2014 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef REPLACE_FUNCTION_DEF_WITH_DECL_H
#define REPLACE_FUNCTION_DEF_WITH_DECL_H

#include <string>
#include "llvm/ADT/SmallVector.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class FunctionDecl;
  class CXXConstructorDecl;
  class FunctionTemplateDecl;
  class SourceLocation;
}

class ReplaceFunctionDefWithDeclCollectionVisitor;

class ReplaceFunctionDefWithDecl : public Transformation {
friend class ReplaceFunctionDefWithDeclCollectionVisitor;

public:

  // enable rewriting multiple instances
  ReplaceFunctionDefWithDecl(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc, /*MultipleRewrites=*/true),
      CollectionVisitor(NULL),
      TheFunctionDef(NULL)
  { }

  ~ReplaceFunctionDefWithDecl();

private:
  
  typedef llvm::SmallVector<const clang::FunctionDecl *, 500>
            FunctionDeclVector;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  bool isMacroExpansion(const clang::FunctionDecl *FD);

  void addOneFunctionDef(const clang::FunctionDecl *FD);

  void rewriteOneFunctionDef(const clang::FunctionDecl *FD);

  void removeCtorInitializers(const clang::CXXConstructorDecl *Ctor);

  unsigned getNumWrittenInitializers(const clang::CXXConstructorDecl *Ctor);

  bool hasValidOuterLocStart(const clang::FunctionTemplateDecl *FTD, 
                             const clang::FunctionDecl *FD);

  void removeStringBeforeTypeIdentifier(const clang::SourceLocation &StartLoc,
                                        const clang::SourceLocation &EndLoc);
 
  bool removeOneInlineKeyword(const std::string &LeadingInlineStr,
                              const std::string &InlineStr, 
                              const std::string &Str,
                              const clang::SourceLocation &StartLoc);

  bool removeInlineKeyword(const std::string &InlineStr, 
                           const std::string &Str,
                           const clang::SourceLocation &StartLoc);

  void removeInlineKeywordFromOneFunctionDecl(const clang::FunctionDecl *FD);

  void removeInlineKeywordFromFunctionDecls(const clang::FunctionDecl *FD);

  void doRewriting();

  FunctionDeclVector AllValidFunctionDefs;

  ReplaceFunctionDefWithDeclCollectionVisitor *CollectionVisitor;
  
  const clang::FunctionDecl *TheFunctionDef;

  // Unimplemented
  ReplaceFunctionDefWithDecl(void);

  ReplaceFunctionDefWithDecl(const ReplaceFunctionDefWithDecl &);

  void operator=(const ReplaceFunctionDefWithDecl &);
};
#endif

