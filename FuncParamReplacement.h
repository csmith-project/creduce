#ifndef FUNC_PARAM_REPLACEMENT_H
#define FUNC_PARAM_REPLACEMENT_H

#include <string>
#include "Transformation.h"

class FPRASTConsumer;

class FuncParamReplacement : public Transformation {

public:

  explicit FuncParamReplacement(const char *TransName);

  ~FuncParamReplacement(void);

  virtual bool doTransformation(void);

  virtual void initializeTransformation(void);

private:
  
  FPRASTConsumer *TransformationASTConsumer;
  
  // Unimplemented
  FuncParamReplacement(void);

  FuncParamReplacement(const FuncParamReplacement &);

  void operator=(const FuncParamReplacement &);
};

#endif
