#include "ReducePointerLevel.h"

#include <sstream>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"

#include "RewriteUtils.h"
#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Reduce pointer indirect levels.\n";

static RegisterTransformation<ReducePointerLevel>
         Trans("reduce-pointer-level", DescriptionMsg);

class PointerLevelCollectionVisitor : public 
  RecursiveASTVisitor<PointerLevelCollectionVisitor> {

public:

  explicit PointerLevelCollectionVisitor(ReducePointerLevel *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitVarDecl(VarDecl *VD);

private:

  ReducePointerLevel *ConsumerInstance;
};

class PointerLevelAnalysisVisitor : public 
  RecursiveASTVisitor<PointerLevelAnalysisVisitor> {

public:

  explicit PointerLevelAnalysisVisitor(ReducePointerLevel *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitAssignmentStmt(AssignmentStmt *AS);

  bool VisitUnaryOperator(UnaryOperator *UO);

private:

  ReducePointerLevel *ConsumerInstance;
};

bool PointerLevelCollectionVisitor::VisitVarDecl(VarDecl *VD)
{
  const Type *Tp = VarDecl->getType().getTypePtr();
  if (Tp->isPointerType())
    ConsumerInstance->ValidInstanceNum++;
  return true;
}

bool PointerLevelAnalysisVisitor::VisitUnaryOperator(UnaryOperator *UO)
{
  // TODO: handle UO_AddrOf and UO_Deref here
}
