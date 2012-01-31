#include "Transformation.h"

#include <sstream>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

class TransNameQueryVisitor : public 
        RecursiveASTVisitor<TransNameQueryVisitor> {

public:
  TransNameQueryVisitor(TransNameQueryWrap *Instance, 
                        const std::string &Prefix)
    : WrapInstance(Instance),
      NamePrefix(Prefix)
  { }

  bool VisitVarDecl(VarDecl *VD);

private:
  TransNameQueryWrap *WrapInstance;

  const std::string NamePrefix;

};

bool TransNameQueryVisitor::VisitVarDecl(VarDecl *VD)
{
  std::string Name = VD->getNameAsString();
  size_t Sz = NamePrefix.size();

  if (Name.compare(0, Sz, NamePrefix))
    return true;

  std::string PostfixStr = Name.substr(Sz);
  TransAssert((PostfixStr.size() > 0) && "Bad trans tmp name!");

  std::stringstream TmpSS(PostfixStr);
  unsigned int PostfixV;
  if (!(TmpSS >> PostfixV))
    TransAssert(0 && "Non-integer trans tmp name!");

  if (PostfixV > WrapInstance->MaxPostfix)
    WrapInstance->MaxPostfix = PostfixV;

  return true;
}

TransNameQueryWrap::TransNameQueryWrap(const std::string &Prefix)
  : NamePrefix(Prefix),
    MaxPostfix(0)
{
  NameQueryVisitor = new TransNameQueryVisitor(this, Prefix);
}

TransNameQueryWrap::~TransNameQueryWrap(void)
{
  delete NameQueryVisitor;
}

bool TransNameQueryWrap::TraverseDecl(Decl *D)
{
  return NameQueryVisitor->TraverseDecl(D);
}

void Transformation::outputTransformedSource(llvm::raw_ostream &OutStream)
{
  FileID MainFileID = SrcManager->getMainFileID();
  const RewriteBuffer *RWBuf = TheRewriter.getRewriteBufferFor(MainFileID);

  // RWBuf is non-empty upon any rewrites
  TransAssert(RWBuf && "Empty RewriteBuffer!");
  OutStream << std::string(RWBuf->begin(), RWBuf->end());
  OutStream.flush();
}

void Transformation::outputOriginalSource(llvm::raw_ostream &OutStream)
{
  FileID MainFileID = SrcManager->getMainFileID();
  const llvm::MemoryBuffer *MainBuf = SrcManager->getBuffer(MainFileID);
  TransAssert(MainBuf && "Empty MainBuf!");
  OutStream << MainBuf->getBufferStart(); 
  OutStream.flush();
}

void Transformation::getTransErrorMsg(std::string &ErrorMsg)
{
  if (TransError == TransSuccess) {
    ErrorMsg = "";
  }
  else if (TransError == TransInternalError) {
    ErrorMsg = "Internal transformation error!";
  }
  else if (TransError == TransMaxInstanceError) {
    ErrorMsg = "The counter value exceeded the number of transformation instances!";
  }
  else if (TransError == TransMaxVarsError) {
    ErrorMsg = "Too many variables!";
  }
  else if (TransError == TransNoValidVarsError) {
    ErrorMsg = "No variables need to be renamed!";
  }
  else if (TransError == TransNoValidFunsError) {
    ErrorMsg = "No valid function declarations exist!";
  }
  else if (TransError == TransNoValidParamsError) {
    ErrorMsg = "No valid parameters declarations exist!";
  }
  else {
    TransAssert(0 && "Unknown transformation error!");
  }
}

