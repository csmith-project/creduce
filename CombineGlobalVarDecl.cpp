#include "CombineGlobalVarDecl.h"

#include <algorithm>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"
#include "RewriteUtils.h"

using namespace clang;
using namespace llvm;

static const char *DescriptionMsg =
"Combine global variable declarations with the same type. \
Each iteration only does one combination, i.e., \
for the code below: \
  int a, b; \
  int c; \
  int d; \
We will need to invoke this transformation twice \
to achieve a complete combination. \
Note that this transformation always tries to combine \
the rest of declarations with the very first one, \
so it is an unsound transformation and could result \
in compilation failures. \n";

static RegisterTransformation<CombineGlobalVarDecl>
         Trans("combine-global-var", DescriptionMsg);

void CombineGlobalVarDecl::Initialize(ASTContext &context) 
{
  Context = &context;
  SrcManager = &Context->getSourceManager();
  TheRewriter.setSourceMgr(Context->getSourceManager(), 
                           Context->getLangOptions());
}

void CombineGlobalVarDecl::HandleTopLevelDecl(DeclGroupRef DGR) 
{
  DeclGroupRef::iterator DI = DGR.begin();
  VarDecl *VD = dyn_cast<VarDecl>(*DI);
  if (!VD)
    return;

  const Type *T = VD->getType().getTypePtr();
  const Type *CanonicalT = Context->getCanonicalType(T);
  
  DeclGroupVector *DV;
  TypeToDeclMap::iterator TI = AllDeclGroups.find(CanonicalT);
  if (TI == AllDeclGroups.end()) {
    DV = new DeclGroupVector::SmallVector();
    AllDeclGroups[CanonicalT] = DV;
  }
  else {
    ValidInstanceNum++;
    DV = (*TI).second;

    if (ValidInstanceNum == TransformationCounter) {
      if (DV->size() >= 1) {
        void* DP1 = *(DV->begin());
        TheDeclGroupRefs.push_back(DP1);
        TheDeclGroupRefs.push_back(DGR.getAsOpaquePtr());
      }
    }
  }
  DV->push_back(DGR.getAsOpaquePtr());
}
 
void CombineGlobalVarDecl::HandleTranslationUnit(ASTContext &Ctx)
{
  if (QueryInstanceOnly) {
    return;
  }

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  doCombination();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void CombineGlobalVarDecl::doCombination(void)
{
  TransAssert(TheDeclGroupRefs.size() == 2);
  void *P2 = TheDeclGroupRefs.pop_back_val();
  void *P1 = TheDeclGroupRefs.pop_back_val();

  DeclGroupRef FirstDGR = DeclGroupRef::getFromOpaquePtr(P1);
  DeclGroupRef SecondDGR = DeclGroupRef::getFromOpaquePtr(P2);

  SourceLocation EndLoc = 
    RewriteUtils::getDeclGroupRefEndLoc(FirstDGR, &TheRewriter, SrcManager);

  std::string DStr;
  RewriteUtils::getDeclGroupStrAndRemove(SecondDGR, DStr, 
                                         &TheRewriter, SrcManager);
  TheRewriter.InsertText(EndLoc, ", " + DStr, /*InsertAfter=*/false);
}

CombineGlobalVarDecl::~CombineGlobalVarDecl(void)
{
  for(TypeToDeclMap::iterator I = AllDeclGroups.begin(), 
      E = AllDeclGroups.end(); I != E; ++I) {
    delete (*I).second;
  }
}

