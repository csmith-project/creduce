#include "Transformation.h"

#include <cassert>

#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

void Transformation::outputTransformedSource(void)
{
  FileID MainFileID = SrcManager->getMainFileID();
  const RewriteBuffer *RWBuf = TheRewriter.getRewriteBufferFor(MainFileID);
  assert(RWBuf && "Empty RewriteBuffer!");
  llvm::outs() << std::string(RWBuf->begin(), RWBuf->end());
}

void Transformation::outputOriginalSource(void)
{
  llvm::outs() << "No Change!\n";
}

