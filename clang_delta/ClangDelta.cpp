//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2016 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string>
#include <sstream>
#include <cstdlib>

#include "llvm/Support/raw_ostream.h"
#include "TransformationManager.h"
#include "git_version.h"

static TransformationManager *TransMgr;
static int ErrorCode = -1;

static void PrintVersion()
{
  llvm::outs() << "clang_delta " << PACKAGE_VERSION << "\n";
  llvm::outs() << "Git version: " << git_version << "\n";
  // XXX print copyright, contact info, etc.?
}

static void PrintHelpMessage()
{
  PrintVersion();
  llvm::outs() << "\n";
  llvm::outs() << "Usage: \n";
  llvm::outs() << "  clang_delta ";
  llvm::outs() << "--transformation=<name> ";
  llvm::outs() << "--counter=<number> ";
  llvm::outs() << "--output=<output_filename> ";
  llvm::outs() << "<source_filename>\n\n";

  llvm::outs() << "clang_delta options:\n";

  llvm::outs() << "  --help: ";
  llvm::outs() << "print this message\n";
  llvm::outs() << "  --version: ";
  llvm::outs() << "print the program version number\n";

  llvm::outs() << "  --verbose-transformations: ";
  llvm::outs() << "print verbose description messages for all ";
  llvm::outs() << "transformations\n";

  llvm::outs() << "  --transformation=<name>: ";
  llvm::outs() << "specify the transformation\n";

  llvm::outs() << "  --transformations: ";
  llvm::outs() << "print the names of all available transformations\n";

  llvm::outs() << "  --query-instances=<name>: ";
  llvm::outs() << "query available transformation instances for a given ";
  llvm::outs() << "transformation\n";

  llvm::outs() << "  --counter=<number>: ";
  llvm::outs() << "specify the instance of the transformation to perform\n";

  llvm::outs() << "  --to-counter=<number>: ";
  llvm::outs() << "specify the ending instance of the transformation to ";
  llvm::outs() << "perform (when this option is given, clang_delta will ";
  llvm::outs() << "rewrite multiple instances [counter,to-counter] ";
  llvm::outs() << "simultaneously. Note that currently only ";
  llvm::outs() << "replace-function-def-with-decl supports this feature.)\n";

  llvm::outs() << "  --replacement=<string>: ";
  llvm::outs() << "instead of performing normal rewriting, the candidate ";
  llvm::outs() << "pointed by the counter will be replaced by the passed ";
  llvm::outs() << "\"string\". Currently, this option works only with ";
  llvm::outs() << "transformation expression-detector.\n";

  llvm::outs() << "  --check-reference=<value>: ";
  llvm::outs() << "insert code to check if the candidate designated by the ";
  llvm::outs() << "counter equals to the reference value or not. Currently, ";
  llvm::outs() << "this option works only with transformation ";
  llvm::outs() << "expression-detector.\n";

  llvm::outs() << "  --output=<filename>: ";
  llvm::outs() << "specify where to output the transformed source code ";
  llvm::outs() << "(default: stdout)\n";
  llvm::outs() << "\n";
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
  exit(ErrorCode);
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

    if (!(TmpSS >> Val)) {
      ErrorCode = TransformationManager::ErrorInvalidCounter;
      Die("Invalid counter[" + ArgValueStr + "]");
    }

    TransMgr->setTransformationCounter(Val);
  }
  else if (!ArgName.compare("to-counter")) {
    int Val;
    std::stringstream TmpSS(ArgValue);

    if (!(TmpSS >> Val)) {
      ErrorCode = TransformationManager::ErrorInvalidCounter;
      Die("Invalid to-counter[" + ArgValueStr + "]");
    }

    TransMgr->setToCounter(Val);
  }
  else if (!ArgName.compare("output")) {
    TransMgr->setOutputFileName(ArgValue);
  }
  else if (!ArgName.compare("replacement")) {
    TransMgr->setReplacement(ArgValue);
  }
  else if (!ArgName.compare("check-reference")) {
    TransMgr->setReferenceValue(ArgValue);
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
  else if (!ArgStr.compare("version")) {
    PrintVersion();
    exit(0);
  }
  else if (!ArgStr.compare("transformations")) {
    TransMgr->printTransformationNames();
    exit(0);
  }
  else if (!ArgStr.compare("verbose-transformations")) {
    TransMgr->printTransformations();
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
  if (!TransMgr->verify(ErrorMsg, ErrorCode))
    Die(ErrorMsg);

  if (!TransMgr->initializeCompilerInstance(ErrorMsg))
    Die(ErrorMsg);

  if (!TransMgr->doTransformation(ErrorMsg, ErrorCode)) {
    // fail to do transformation
    Die(ErrorMsg);
  }

  if (TransMgr->getQueryInstanceFlag()) 
    TransMgr->outputNumTransformationInstances();

  TransformationManager::Finalize();
  return 0;
}

