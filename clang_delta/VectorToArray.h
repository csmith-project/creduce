#ifndef VECTOR_TO_ARRAY_H
#define VECTOR_TO_ARRAY_H

#include "Transformation.h"

class VectorToArrayCollectionVisitor;
class VectorToArrayRewriteVisitor;

class VectorToArray : public Transformation {

  friend class VectorToArrayCollectionVisitor;
  friend class VectorToArrayRewriteVisitor;

public:
  VectorToArray(const char *TransName, const char *Desc)
      : Transformation(TransName, Desc) {}

private:
  virtual void Initialize(clang::ASTContext &context);
  virtual void HandleTranslationUnit(clang::ASTContext &ctx);
  void simplifyVectorToArray();

  VectorToArrayCollectionVisitor *CollectionVisitor;
  VectorToArrayRewriteVisitor *RewriteVisitor;

  clang::ClassTemplateDecl *TheVectorDecl;
  clang::VarDecl *TheVarDecl;

  // Unimplemented
  VectorToArray();
  VectorToArray(const VectorToArray &);
  void operator=(const VectorToArray &);
};

#endif
