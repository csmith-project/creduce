#include "TransformationManager.h"

#include "Transformation.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Frontend/CompilerInstance.h"

#include "llvm/Config/config.h"

using namespace clang;

TransformationManager* TransformationManager::Instance;

TransformationManager *TransformationManager::GetInstance(void)
{
  if (TransformationManager::Instance)
    return TransformationManager::Instance;

  TransformationManager::Instance = new TransformationManager();
  assert(TransformationManager::Instance);

  TransformationManager::Instance->initializeCompilerInstance();
  return TransformationManager::Instance;
}

void TransformationManager::initializeCompilerInstance(void)
{
  ClangInstance = new CompilerInstance();
  assert(ClangInstance);
  
  ClangInstance->createDiagnostics(0, NULL);
  //ClangInstance->getDiagnostics().setIgnoreAllWarnings(true);
  ClangInstance->getLangOpts().C99 = 1;
  ClangInstance->getLangOpts().CPlusPlus = 1;
  TargetOptions &TargetOpts = ClangInstance->getTargetOpts();
  TargetOpts.Triple = LLVM_HOSTTRIPLE;
  TargetInfo *Target = 
    TargetInfo::CreateTargetInfo(ClangInstance->getDiagnostics(),
                                 TargetOpts);
  ClangInstance->setTarget(Target);
  ClangInstance->createFileManager();
  ClangInstance->createSourceManager(ClangInstance->getFileManager());
  ClangInstance->createPreprocessor();

  DiagnosticConsumer &DgClient = ClangInstance->getDiagnosticClient();
  DgClient.BeginSourceFile(ClangInstance->getLangOpts(),
                           &ClangInstance->getPreprocessor());
  ClangInstance->createASTContext();
}

void TransformationManager::Finalize(void)
{
  assert(TransformationManager::Instance);
  
  std::map<std::string, Transformation *>::iterator I, E;
  for (I = Instance->TransformationsMap.begin(), 
       E = Instance->TransformationsMap.end();
       I != E; ++I) {
    delete (*I).second;
  }
  delete Instance->ClangInstance;

  delete Instance;
  Instance = NULL;
}

bool TransformationManager::doTransformation(void)
{
  assert(CurrentTransformationImpl && "Bad transformation instance!");
  CurrentTransformationImpl->doInitialization(ClangInstance, TransformationCounter);
  return CurrentTransformationImpl->doTransformation();
}

bool TransformationManager::verify(std::string &ErrorMsg)
{
  if (!CurrentTransformationImpl) {
    ErrorMsg = "Empty transformation instance!";
    return false;
  }

  if (!ClangInstance) {
    ErrorMsg = "Empty clang instance!";
    return false;
  }

  if (TransformationCounter <= 0) {
    ErrorMsg = "Invalid transformation counter!";
    return false;
  }

  if (!ClangInstance->InitializeSourceManager(SrcFileName)) {
    ErrorMsg = "Cannot open source file!";
    return false;
  }

  return true;
}

void TransformationManager::registerTransformation(
       const char *TransName, 
       Transformation *TransImpl)
{
  assert((TransImpl != NULL) && "NULL Transformation!");
  assert((TransformationsMap.find(TransName) == TransformationsMap.end()) &&
         "Duplicated transformation!");

  TransformationsMap[TransName] = TransImpl;
}

TransformationManager::TransformationManager(void)
  : CurrentTransformationImpl(NULL),
    TransformationCounter(-1),
    SrcFileName(""),
    ClangInstance(NULL)
{
  // Nothing to do
}

TransformationManager::~TransformationManager(void)
{
  // Nothing to do
}

