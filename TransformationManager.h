#ifndef TRANSFORMATION_MANAGER_H
#define TRANSFORMATION_MANAGER_H

#include <string>
#include <map>
#include <cassert>

class Transformation;
namespace clang {
  class CompilerInstance;
}

class TransformationManager {
public:

  static TransformationManager *GetInstance(void);

  static void Finalize(void);

  void registerTransformation(const char *TransName, 
                              Transformation *TransImpl);
  
  bool doTransformation(void);

  bool verify(std::string &ErrorMsg);

  int setTransformation(const std::string &Trans) {
    if (TransformationsMap.find(Trans.c_str()) == TransformationsMap.end())
      return -1;
    CurrentTransformationImpl = TransformationsMap[Trans.c_str()];
    return 0;
  }

  void setTransformationCounter(int Counter) {
    assert((Counter > 0) && "Bad Counter value!");
    TransformationCounter = Counter;
  }

  void setSrcFileName(const std::string &FileName) {
    assert(!SrcFileName.length() && "Could only process one file each time");
    SrcFileName = FileName;
  }

private:
  
  TransformationManager(void);

  ~TransformationManager(void);

  static TransformationManager *Instance;

  Transformation *CurrentTransformationImpl;

  int TransformationCounter;

  std::string SrcFileName;

  clang::CompilerInstance *ClangInstance;

  std::map<std::string, Transformation *> TransformationsMap;

  void initializeCompilerInstance(void);

  // Unimplemented
  TransformationManager(const TransformationManager &);

  void operator=(const TransformationManager&);

};

template<typename TransformationClass>
class RegisterTransformation {
public:
  explicit RegisterTransformation(const char *TransName) {
    Transformation *TransImpl = new TransformationClass(TransName);
    assert(TransImpl && "Fail to create TransformationClass");
    
    TransformationManager::GetInstance()->registerTransformation(TransName, TransImpl);
  }

private:
  // Unimplemented
  RegisterTransformation(const RegisterTransformation &);

  void operator=(const RegisterTransformation &);

};

#endif
