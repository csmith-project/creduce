#include <cstdlib>
#include <string>
#include <iostream>
#include <sstream>

#include "TransformationManager.h"

static TransformationManager *TransMgr;

static void PrintHelpMessage(void)
{
  std::cout << "Usage: \n";
  std::cout << "  clang_delta --transformation=name --counter=number filename\n\n";
  std::cout << "clang_delta options:\n";
  std::cout << "  --help: print this message\n";
  std::cout << "  --transformation=<name>: specify the transformation\n";
  std::cout << "  --counter=<number>: specify the instance of the transformation to perform\n";
  std::cout << "\n";
}

static void DieOnBadCmdArg(const std::string &ArgStr)
{
  std::cout << "Error: Bad command line option `" << ArgStr << "`\n";
  std::cout << "\n";
  PrintHelpMessage();
  TransformationManager::Finalize();
  exit(-1);
}

static void Die(const std::string &Message)
{
  std::cout << "Error: " << Message << "\n";
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
  else if (!ArgName.compare("counter")) {
    int Val;
    std::stringstream TmpSS(ArgValue);

    if (!(TmpSS >> Val))
      DieOnBadCmdArg("--" + ArgValueStr);

    TransMgr->setTransformationCounter(Val);
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

  if (!TransMgr->doTransformation()) {
    // fail to do transformation
    Die("Failed to do transformations");
  }

  TransformationManager::Finalize();
  return 0;
}

