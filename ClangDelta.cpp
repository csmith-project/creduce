#include <string>
#include <sstream>
#include <cstdlib>

#include "llvm/Support/raw_ostream.h"
#include "TransformationManager.h"

static TransformationManager *TransMgr;

static void PrintHelpMessage(void)
{
  llvm::outs() << "Usage: \n";
  llvm::outs() << "  clang_delta ";
  llvm::outs() << "--transformation=<name> ";
  llvm::outs() << "--counter=<number> ";
  llvm::outs() << "--output=<output_filename> ";
  llvm::outs() << "<source_filename>\n\n";

  llvm::outs() << "clang_delta options:\n";

  llvm::outs() << "  --help: ";
  llvm::outs() << "print this message\n";

  llvm::outs() << "  --transformation=<name>: ";
  llvm::outs() << "specify the transformation\n";

  llvm::outs() << "  --transformations: ";
  llvm::outs() << "print the names of all available transformations\n";

  llvm::outs() << "  --query-instances=<name>: ";
  llvm::outs() << "query available transformation instances for a given transformation\n";

  llvm::outs() << "  --counter=<number>: ";
  llvm::outs() << "specify the instance of the transformation to perform\n";

  llvm::outs() << "  --output=<filename>: ";
  llvm::outs() << "specify where to output the transformed source code ";
  llvm::outs() << "(default: stdout)\n";
  llvm::outs() << "\n";

  TransMgr->printTransformations();
}

static void DieOnBadCmdArg(const std::string &ArgStr)
{
  llvm::outs() << "Error: Bad command line option `" << ArgStr << "`\n";
  llvm::outs() << "\n";
  PrintHelpMessage();
  TransformationManager::Finalize();
  exit(-1);
}

static void Die(const std::string &Message)
{
  llvm::outs() << "Error: " << Message << "\n";
  TransformationManager::Finalize();
  exit(-1);
}

static void HandleOneArgValue(const std::string &ArgValueStr, size_t SepPos)
{
  if ((SepPos < 1) || (SepPos >= ArgValueStr.length())) {
      DieOnBadCmdArg("--" + ArgValueStr);
  }

  std::string ArgName, ArgValue;

  ArgName = ArgValueStr.substr(0, SepPos);
  ArgValue = ArgValueStr.substr(SepPos+1);

  if (!ArgName.compare("transformation")) {
    if (TransMgr->setTransformation(ArgValue)) {
      Die("Invalid transformation[" + ArgValue + "]");
    }
  }
  else if (!ArgName.compare("query-instances")) {
    if (TransMgr->setTransformation(ArgValue)) {
      Die("Invalid transformation[" + ArgValue + "]");
    }
    TransMgr->setQueryInstanceFlag(true);
    TransMgr->setTransformationCounter(1);
  }
  else if (!ArgName.compare("counter")) {
    int Val;
    std::stringstream TmpSS(ArgValue);

    if (!(TmpSS >> Val))
      DieOnBadCmdArg("--" + ArgValueStr);

    TransMgr->setTransformationCounter(Val);
  }
  else if (!ArgName.compare("output")) {
    TransMgr->setOutputFileName(ArgValue);
  }
  else {
    DieOnBadCmdArg("--" + ArgValueStr);
  }
}

static void HandleOneNoneValueArg(const std::string &ArgStr)
{
  if (!ArgStr.compare("help")) {
    PrintHelpMessage();
    exit(0);
  }
  if (!ArgStr.compare("transformations")) {
    TransMgr->printTransformationNames();
    exit(0);
  }
  else {
    DieOnBadCmdArg(ArgStr);
  }
}

static void HandleOneArg(const char *Arg)
{
  std::string ArgStr(Arg);

  if (!ArgStr.compare(0, 2, "--")) {
    std::string SubArgStr = ArgStr.substr(2);
    if (!SubArgStr.length())
      DieOnBadCmdArg(ArgStr);

    size_t found;
    found = SubArgStr.find('=');
    if (found != std::string::npos) {
      HandleOneArgValue(SubArgStr, found);
    }
    else {
      HandleOneNoneValueArg(SubArgStr);
    }
  }
  else {
    TransMgr->setSrcFileName(ArgStr);
  }
}

int main(int argc, char **argv)
{
  TransMgr = TransformationManager::GetInstance();
  for (int i = 1; i < argc; i++) {
    HandleOneArg(argv[i]);
  }

  std::string ErrorMsg;
  if (!TransMgr->verify(ErrorMsg))
    Die(ErrorMsg);

  if (!TransMgr->doTransformation(ErrorMsg)) {
    // fail to do transformation
    Die(ErrorMsg);
  }

  if (TransMgr->getQueryInstanceFlag()) 
    TransMgr->outputNumTransformationInstances();

  TransformationManager::Finalize();
  return 0;
}

