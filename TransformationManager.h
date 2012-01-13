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
    assert(!SrcFileName.empty() && "Could only process one file each time");
    SrcFileName = FileName;
  }

  void setOutputFileName(const std::string &FileName) {
    OutputFileName = FileName;
  }

  void printTransformations();

private:
  
  TransformationManager(void);

  ~TransformationManager(void);

  static TransformationManager *Instance;

  Transformation *CurrentTransformationImpl;

  int TransformationCounter;

  std::string SrcFileName;

  std::string OutputFileName;

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
  RegisterTransformation(const char *TransName, const char *Desc) {
    Transformation *TransImpl = new TransformationClass(TransName, Desc);
    assert(TransImpl && "Fail to create TransformationClass");
    
    TransformationManager *M = TransformationManager::GetInstance();
    M->registerTransformation(TransName, TransImpl);
  }

private:
  // Unimplemented
  RegisterTransformation(const RegisterTransformation &);

  void operator=(const RegisterTransformation &);

};

#endif
