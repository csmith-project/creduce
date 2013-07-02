//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef INSTANTIATE_TEMPLATE_PARAM_H
#define INSTANTIATE_TEMPLATE_PARAM_H

#include "llvm/ADT/SmallPtrSet.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class FunctionTemplateDecl;
  class ClassTemplateDecl;
  class TemplateDecl;
  class TemplateArgumentList;
  class TemplateArgument;
  class QualType;
  class NamedDecl;
  class Type;
  class RecordDecl;
}

class InstantiateTemplateParamASTVisitor;
class InstantiateTemplateParamRewriteVisitor;

class InstantiateTemplateParam : public Transformation {
friend class InstantiateTemplateParamASTVisitor;
friend class InstantiateTemplateParamRewriteVisitor;

public:
  InstantiateTemplateParam(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      ParamRewriteVisitor(NULL),
      TheParameter(NULL),
      TheTemplateDecl(NULL),
      TheInstantiationString(""),
      TheForwardDeclString("")
  {}

  ~InstantiateTemplateParam();

private:

  typedef llvm::SmallPtrSet<const clang::RecordDecl *, 10> RecordDeclSet;

  typedef llvm::SmallPtrSet<void *, 10> LocPtrSet;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void handleOneFunctionTemplateDecl(const clang::FunctionTemplateDecl *D);

  void handleOneClassTemplateDecl(const clang::ClassTemplateDecl *D);

  void handleOneTemplateSpecialization(
         const clang::TemplateDecl *D, 
         const clang::TemplateArgumentList & ArgList);

  bool getTemplateArgumentString(const clang::TemplateArgument &Arg, 
                                 std::string &Str,
                                 std::string &ForwardStr);

  bool getTypeString(const clang::QualType &QT, 
                     std::string &ArgStr,
                     std::string &ForwardStr);

  void getForwardDeclStr(const clang::Type *Ty, 
                         std::string &ForwardStr,
                         RecordDeclSet &TempAvailableRecordDecls);

  void addOneForwardDeclStr(const clang::RecordDecl *RD,
                            std::string &ForwardStr,
                            RecordDeclSet &TempAvailableRecordDecls);

  void addForwardTemplateDeclStr(const clang::ClassTemplateDecl *ClassTD,
                                 std::string &ForwardStr,
                                 RecordDeclSet &TempAvailableRecordDecls);

  void removeTemplateKeyword();

  void addForwardDecl();

  RecordDeclSet AvailableRecordDecls;

  LocPtrSet VisitedLocs;

  InstantiateTemplateParamASTVisitor *CollectionVisitor;

  InstantiateTemplateParamRewriteVisitor *ParamRewriteVisitor;

  const clang::NamedDecl *TheParameter;

  const clang::TemplateDecl *TheTemplateDecl;

  std::string TheInstantiationString;

  std::string TheForwardDeclString;

  // Unimplemented
  InstantiateTemplateParam();

  InstantiateTemplateParam(const InstantiateTemplateParam &);

  void operator=(const InstantiateTemplateParam &);
};

#endif
