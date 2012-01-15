#include "ReturnVoid.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Make a function return void. \
Only change the prototype of the function and \
delete all return statements in the function, \
but skip the call sites of this function.";
 
static RegisterTransformation<ReturnVoid> 
         Trans("return-void", DescriptionMsg);

class RVASTVisitor : public RecursiveASTVisitor<RVASTVisitor> {
public:
  typedef RecursiveASTVisitor<RVASTVisitor> Inherited;

  RVASTVisitor(ReturnVoid *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitFunctionDecl(FunctionDecl *FD);

  bool VisitReturnStmt(ReturnStmt *RS);

private:

  ReturnVoid *ConsumerInstance;

  bool rewriteFuncDecl(FunctionDecl *FP);

  bool rewriteReturnStmt(ReturnStmt *RS);

};

void ReturnVoid::Initialize(ASTContext &context) 
{
  Context = &context;
  SrcManager = &Context->getSourceManager();
  TransformationASTVisitor = new RVASTVisitor(this);
  TheRewriter.setSourceMgr(Context->getSourceManager(), 
                           Context->getLangOptions());

  TheFuncDecl = NULL;
  FuncDefStartPos = NULL;
  FuncDefEndPos = NULL;
}

bool ReturnVoid::isNonVoidReturnFunction(FunctionDecl *FD)
{
  // Avoid duplications
  if (std::find(ValidFuncDecls.begin(), 
                ValidFuncDecls.end(), FD) != 
      ValidFuncDecls.end())
    return false;

  QualType RVType = FD->getResultType();
  return !(RVType.getTypePtr()->isVoidType());
}

void ReturnVoid::keepFuncDefRange(FunctionDecl *FD)
{
  assert(!FuncDefStartPos && !FuncDefEndPos && 
         "Duplicated function definition?");

  SourceRange FuncDefRange = FD->getSourceRange();

  SourceLocation StartLoc = FuncDefRange.getBegin();
  FuncDefStartPos = 
      SrcManager->getCharacterData(StartLoc);

  SourceLocation EndLoc = FuncDefRange.getEnd();
  FuncDefEndPos = 
      SrcManager->getCharacterData(EndLoc);
}

bool ReturnVoid::isInTheFuncDef(ReturnStmt *RS)
{
  // The candidate function doesn't have a body
  if (!FuncDefStartPos)
    return false;

  SourceRange RSRange = RS->getSourceRange();

  SourceLocation StartLoc = RSRange.getBegin();
  SourceLocation EndLoc = RSRange.getEnd();
  const char *StartPos =
      SrcManager->getCharacterData(StartLoc);
  const char *EndPos =   
      SrcManager->getCharacterData(EndLoc);
  
  if ((StartPos > FuncDefStartPos) && (StartPos < FuncDefEndPos)) {
    assert((EndPos > FuncDefStartPos) && (EndPos < FuncDefEndPos) && 
            "Bad return statement range!");
    return true;
  }

  return false;
}

void ReturnVoid::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    FunctionDecl *FD = dyn_cast<FunctionDecl>(*I);
    if (!FD)
      continue;

    FunctionDecl *CanonicalDecl = FD->getCanonicalDecl();
    if (isNonVoidReturnFunction(CanonicalDecl)) {
      ValidInstanceNum++;
      ValidFuncDecls.push_back(CanonicalDecl);

      if (ValidInstanceNum == TransformationCounter)
        TheFuncDecl = CanonicalDecl;
    }

    if ((TheFuncDecl == CanonicalDecl) && FD->isThisDeclarationADefinition())
      keepFuncDefRange(FD);
  }
}
 
void ReturnVoid::HandleTranslationUnit(ASTContext &Ctx)
{
  if (TransformationCounter > ValidInstanceNum) {
      TransError = TransMaxInstanceError;
      return;
  }

  assert(TransformationASTVisitor && "NULL TransformationASTVisitor!");
  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);
  assert(TheFuncDecl && "NULL TheFuncDecl!");

  TransformationASTVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

bool RVASTVisitor::rewriteFuncDecl(FunctionDecl *FD)
{
  DeclarationNameInfo NameInfo = FD->getNameInfo();
  SourceLocation NameInfoStartLoc = NameInfo.getBeginLoc();

  SourceRange FuncDefRange = FD->getSourceRange();
  SourceLocation FuncStartLoc = FuncDefRange.getBegin();
  
  const char *NameInfoStartBuf =
      ConsumerInstance->SrcManager->getCharacterData(NameInfoStartLoc);
  const char *FuncStartBuf =
      ConsumerInstance->SrcManager->getCharacterData(FuncStartLoc);
  int Offset = NameInfoStartBuf - FuncStartBuf;

  assert(Offset >= 0);

  return !(ConsumerInstance->TheRewriter.ReplaceText(FuncStartLoc, 
                 Offset, "void "));
}

bool RVASTVisitor::rewriteReturnStmt(ReturnStmt *RS)
{
  SourceRange RSRange = RS->getSourceRange();

  return !(ConsumerInstance->TheRewriter.ReplaceText(RSRange, "return"));
}

bool RVASTVisitor::VisitFunctionDecl(FunctionDecl *FD)
{
  FunctionDecl *CanonicalFD = FD->getCanonicalDecl();

  if (CanonicalFD == ConsumerInstance->TheFuncDecl)
    return rewriteFuncDecl(FD);

  return true;
}

bool RVASTVisitor::VisitReturnStmt(ReturnStmt *RS)
{
  if (ConsumerInstance->isInTheFuncDef(RS))
    return rewriteReturnStmt(RS);

  return true;
}

ReturnVoid::~ReturnVoid(void)
{
  delete TransformationASTVisitor;
}

