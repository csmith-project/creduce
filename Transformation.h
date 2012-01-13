#ifndef TRANSFORMATION_H
#define TRANSFORMATION_H

#include <string>
#include "clang/AST/ASTConsumer.h"
#include "clang/Rewrite/Rewriter.h"

namespace clang {
  class CompilerInstance;
  class ASTContext;
  class SourceManager;
}

typedef enum {
  TransSuccess = 0,
  TransInternalError,
  TransMaxInstanceError
} TransformationError;

class Transformation : public clang::ASTConsumer {
public:

  Transformation(const char *TransName, const char *Desc)
    : Name(TransName),
      TransformationCounter(-1),
      ValidInstanceNum(0),
      Context(NULL),
      SrcManager(NULL),
      TransError(TransSuccess),
      DescriptionString(Desc)
  {
    // Nothing to do
  }

  virtual ~Transformation(void) { }

  void outputOriginalSource(void);

  void outputTransformedSource(void);

  void setTransformationCounter(int Counter) {
    TransformationCounter = Counter;
  }

  bool transSuccess(void) {
    return (TransError == TransSuccess);
  }
  
  bool transInternalError(void) {
    return (TransError == TransInternalError);
  }

protected:

  const std::string &Name;

  int TransformationCounter;

  int ValidInstanceNum;

  clang::ASTContext *Context;

  clang::SourceManager *SrcManager;

  clang::Rewriter TheRewriter;

  TransformationError TransError;
  
  std::string DescriptionString;

};

#endif
