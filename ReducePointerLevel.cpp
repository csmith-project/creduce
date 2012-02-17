#include "ReducePointerLevel.h"

#include <sstream>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"

#include "RewriteUtils.h"
#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Reduce pointer indirect level for a global/local variable. \
All valid variables are sorted by their indirect levels. \
The pass will ensure to first choose a valid variable \
with the largest indirect level. This mechanism could \
reduce the complexity of our implementation, because \
we don't have to consider the case where the chosen variable \
with the largest indirect level would be address-taken. \
Variables at non-largest-indirect-level are ineligible \
if they: \n\
  * being address-taken \n\
  * OR being used as LHS in any form (e.g., *p, **p), \n\
    while the RHS is a CallExpr. \n";

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

  unsigned int getPointerIndirectLevel(const Type *Ty);

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

unsigned int 
PointerLevelCollectionVisitor::getPointerIndirectLevel(const Type *Ty)
{
  unsigned int IndirectLevel = 0;
  const Type *PointeeTy;
  while ((PointeeTy = Ty->getType().getTypePtr())) {
    if (!Ty->isPointerType())
      return IndirectLevel;
    IndirectLevel++;
    Ty = PointeeTy;
  }
  TransAssert(0 && "Unreachable code!");
  return 0;
}
              
bool PointerLevelCollectionVisitor::VisitVarDecl(VarDecl *VD)
{
  // Skip parameters
  if (dyn_cast<ParmVarDecl>(VD))
    return true;

  const Type *Ty = VD->getType().getTypePtr();
  if (!Ty->isPointerType())
    return true;

  VarDecl *CanonicalVD = VD->getCanonicalDecl();
  if (VisitedVarDecls.count(CanonicalVD))
    return true;

  VisitedVarDecls.insert(CanonicalVD);
  unsigned int IndirectLevel = getPointerIndirectLevel(Ty);
  TransAssert((IndirectLevel > 0) && "Bad indirect level!");

  // TODO: add to map
  return true;
}

bool PointerLevelAnalysisVisitor::VisitUnaryOperator(UnaryOperator *UO)
{
  // TODO: handle UO_AddrOf and UO_Deref here
}

