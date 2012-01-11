#ifndef TRANSFORMATION_H
#define TRANSFORMATION_H

#include <string>
#include <cassert>

namespace clang {
  class CompilerInstance;
  class ASTConsumer;
}

class Transformation {

public:

  virtual bool doTransformation(void) = 0;

  virtual void initializeTransformation(void) = 0;

  void doInitialization(clang::CompilerInstance *Instance, int Counter) {
    assert(!Initialized && "Double initialization!");

    ClangInstance = Instance;
    TransformationCounter = Counter;
    Initialized = true;
    initializeTransformation();
  };

  explicit Transformation(const char *TransName)
    : ClangInstance(NULL),
      TransformationCounter(-1),
      Name(TransName),
      Initialized(0)
  {
    // Nothing to do
  }

  virtual ~Transformation(void) { };

protected:
  Transformation(void);

  clang::CompilerInstance *ClangInstance;

  int TransformationCounter;

  const std::string &Name;

  bool Initialized;
  
};

#endif
