#include "Transformation.h"

#include <cassert>

#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

void Transformation::outputTransformedSource(llvm::raw_ostream &OutStream)
{
  FileID MainFileID = SrcManager->getMainFileID();
  const RewriteBuffer *RWBuf = TheRewriter.getRewriteBufferFor(MainFileID);
  assert(RWBuf && "Empty RewriteBuffer!");
  OutStream << std::string(RWBuf->begin(), RWBuf->end());
  OutStream.flush();
}

void Transformation::outputOriginalSource(llvm::raw_ostream &OutStream)
{
  OutStream << "No Change!\n";
}

