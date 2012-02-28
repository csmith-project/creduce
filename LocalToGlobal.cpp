//===----------------------------------------------------------------------===//
// 
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "LocalToGlobal.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;
using namespace llvm;

static const char *DescriptionMsg =
"Move the declaraion of a non-static local variable from \
a function to the global scope. Also rename the moved local variable \
to avoid possible name conflicts. \n";

static RegisterTransformation<LocalToGlobal>
         Trans("local-to-global", DescriptionMsg);

class CollectionVisitor : public RecursiveASTVisitor<CollectionVisitor> {
public:

  explicit CollectionVisitor(LocalToGlobal *Instance)
    : ConsumerInstance(Instance),
      CurrentFuncDecl(NULL)
  { }

  bool VisitVarDecl(VarDecl *VD);

  void setCurrentFuncDecl(FunctionDecl *FD) {
    CurrentFuncDecl = FD;
  }

private:

  LocalToGlobal *ConsumerInstance;

  FunctionDecl *CurrentFuncDecl;
};

class LToGASTVisitor : public RecursiveASTVisitor<LToGASTVisitor> {
public:
  typedef RecursiveASTVisitor<LToGASTVisitor> Inherited;

  explicit LToGASTVisitor(LocalToGlobal *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitDeclStmt(DeclStmt *DS);
 
  bool VisitDeclRefExpr(DeclRefExpr *VarRefExpr);

private:

  LocalToGlobal *ConsumerInstance;

  bool makeLocalAsGlobalVar(FunctionDecl *FP,
                            VarDecl *VD);

};

void LocalToGlobal::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  LocalVarCollectionVisitor = new CollectionVisitor(this);
  TransformationASTVisitor = new LToGASTVisitor(this);
}

void LocalToGlobal::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    FunctionDecl *FD = dyn_cast<FunctionDecl>(*I);
    if (FD && FD->isThisDeclarationADefinition()) {
      LocalVarCollectionVisitor->setCurrentFuncDecl(FD);
      LocalVarCollectionVisitor->TraverseDecl(FD);
      LocalVarCollectionVisitor->setCurrentFuncDecl(NULL);
    }
  }
}
 
void LocalToGlobal::HandleTranslationUnit(ASTContext &Ctx)
{
  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  TransAssert(TransformationASTVisitor && "NULL TransformationASTVisitor!");
  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);
  TransAssert(TheFuncDecl && "NULL TheFuncDecl!");
  TransAssert(TheVarDecl && "NULL TheVarDecl!");

  TransformationASTVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void LocalToGlobal::setNewName(FunctionDecl *FD,
                               const VarDecl *VD)
{
  std::string NewName;
  NewName = FD->getNameInfo().getAsString();
  NewName += "_";
  NewName += VD->getNameAsString();

  // also backup the new name
  TheNewDeclName = NewName;
}

LocalToGlobal::~LocalToGlobal(void)
{
  if (LocalVarCollectionVisitor)
    delete LocalVarCollectionVisitor;
  if (TransformationASTVisitor)
    delete TransformationASTVisitor;
}

bool CollectionVisitor::VisitVarDecl(VarDecl *VD)
{
  TransAssert(CurrentFuncDecl && "NULL CurrentFuncDecl!");

  if (!VD->isLocalVarDecl() || VD->isStaticLocal() || VD->hasExternalStorage())
    return true;

  ConsumerInstance->ValidInstanceNum++;
  if (ConsumerInstance->ValidInstanceNum == 
      ConsumerInstance->TransformationCounter) {
    ConsumerInstance->TheVarDecl = VD;
    ConsumerInstance->TheFuncDecl = CurrentFuncDecl;
    ConsumerInstance->setNewName(CurrentFuncDecl, VD);
  }
  return true;
}

bool LToGASTVisitor::makeLocalAsGlobalVar(FunctionDecl *FP,
                                          VarDecl *LV)
{
  std::string GlobalVarStr;
  std::string NewName = ConsumerInstance->getNewName();

  QualType T = LV->getType();
  T.getAsStringInternal(NewName, 
                        ConsumerInstance->Context->getPrintingPolicy());

  GlobalVarStr = NewName;

  if (LV->hasInit()) {
    const Expr *InitExpr = LV->getInit();
    std::string InitStr("");
    ConsumerInstance->RewriteHelper->getExprString(InitExpr, 
                                                   InitStr);
    GlobalVarStr += " = ";
    GlobalVarStr += InitStr; 
  }

  GlobalVarStr += ";\n";

  SourceRange FuncRange = FP->getSourceRange();
  SourceLocation StartLoc = FuncRange.getBegin();
  return !(ConsumerInstance->TheRewriter.InsertText(StartLoc, 
                                                    GlobalVarStr,
                                                    false));
}

bool LToGASTVisitor::VisitDeclStmt(DeclStmt *DS)
{
  Decl *PrevDecl = NULL;
  VarDecl *VD = NULL;
  int NumDecls = 0;
  int VarPos = 0;

  for (DeclStmt::const_decl_iterator I = DS->decl_begin(),
       E = DS->decl_end(); I != E; ++I) {
    NumDecls++;

    // we have got the var
    if (VD)
      continue;

    Decl *CurrDecl = (*I);
    if (CurrDecl != ConsumerInstance->TheVarDecl) {
      PrevDecl = (*I);
      continue;
    }
   
    TransAssert(!VD && "Duplicated Definition?");
    VD = dyn_cast<VarDecl>(CurrDecl);
    TransAssert(VD && "Bad VarDecl!"); 
    VarPos = NumDecls - 1;
  }

  if (!VD)
    return true;

  bool IsFirstDecl = (!VarPos);
  ConsumerInstance->RewriteHelper->removeVarFromDeclStmt
    (DS, VD, PrevDecl, IsFirstDecl);

  return makeLocalAsGlobalVar(ConsumerInstance->TheFuncDecl, VD);
}

bool LToGASTVisitor::VisitDeclRefExpr(DeclRefExpr *VarRefExpr)
{
  if (!ConsumerInstance->TheVarDecl)
    return true;

  const ValueDecl *OrigDecl = VarRefExpr->getDecl();

  if (OrigDecl != ConsumerInstance->TheVarDecl)
    return true;

  SourceRange ExprRange = VarRefExpr->getSourceRange();
  return 
    !(ConsumerInstance->TheRewriter.ReplaceText(ExprRange,
        ConsumerInstance->TheNewDeclName));
}

