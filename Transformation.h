#ifndef TRANSFORMATION_H
#define TRANSFORMATION_H

#include <string>
#include <cstdlib>
#include <cassert>
#include "clang/AST/ASTConsumer.h"
#include "clang/Rewrite/Rewriter.h"

#ifndef ENABLE_TRANS_ASSERT
  #define TransAssert(x) {if (!(x)) exit(0);}
#else
  #define TransAssert(x) assert(x)
#endif

namespace clang {
  class CompilerInstance;
  class ASTContext;
  class SourceManager;
  class Decl;
}

typedef enum {
  TransSuccess = 0,
  TransInternalError,
  TransMaxInstanceError,
  TransMaxVarsError,
  TransNoValidVarsError,
} TransformationError;

class Transformation : public clang::ASTConsumer {

public:

  Transformation(const char *TransName, const char *Desc)
    : Name(TransName),
      TransformationCounter(-1),
      ValidInstanceNum(0),
      QueryInstanceOnly(false),
      Context(NULL),
      SrcManager(NULL),
      TransError(TransSuccess),
      DescriptionString(Desc)
  {
    // Nothing to do
  }

  virtual ~Transformation(void) { }

  void outputOriginalSource(llvm::raw_ostream &OutStream);

  void outputTransformedSource(llvm::raw_ostream &OutStream);

  void setTransformationCounter(int Counter) {
    TransformationCounter = Counter;
  }

  void setQueryInstanceFlag(bool Flag) {
    QueryInstanceOnly = Flag;
  }

  bool transSuccess(void) {
    return (TransError == TransSuccess);
  }
  
  bool transMaxInstanceError(void) {
    return (TransError == TransMaxInstanceError);
  }

  bool transInternalError(void) {
    return (TransError == TransInternalError);
  }

  std::string &getDescription(void) {
    return DescriptionString;
  }

  void getTransErrorMsg(std::string &ErrorMsg);

  int getNumTransformationInstances(void) {
    return ValidInstanceNum;
  }

  virtual bool skipCounter(void) {
    return false;
  }

protected:

  const std::string &Name;

  int TransformationCounter;

  int ValidInstanceNum;

  bool QueryInstanceOnly;

  clang::ASTContext *Context;

  clang::SourceManager *SrcManager;

  clang::Rewriter TheRewriter;

  TransformationError TransError;
  
  std::string DescriptionString;

};

class TransNameQueryVisitor;

class TransNameQueryWrap {
friend class TransNameQueryVisitor;

public:
  explicit TransNameQueryWrap(const std::string &Prefix);

  ~TransNameQueryWrap(void);

  unsigned int getMaxNamePostfix(void) {
    return MaxPostfix;
  }

  bool TraverseDecl(clang::Decl *D);

private:

  std::string NamePrefix;

  unsigned int MaxPostfix;

  TransNameQueryVisitor *NameQueryVisitor;

  // Unimplemented
  TransNameQueryWrap(void);

  TransNameQueryWrap(const TransNameQueryWrap &);

  void operator=(const TransNameQueryWrap &);
};

#endif
