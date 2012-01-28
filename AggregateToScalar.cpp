#include "AggregateToScalar.h"

#include <sstream>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"

#include "RewriteUtils.h"
#include "TransformationManager.h"

using namespace clang;
using namespace llvm;

static const char *DescriptionMsg =
"Replace a struct/union member with scalar variables. \
In more detail, the transformation creates \
scalar variables for a referenced struct/union member, \
assigns the initial value of the struct/union member to \
the scalars, and substitutes all accesses to the struct/union \
member with the accesses to the corresponding scalar variables. \
(Note that this transformation is unsound). ";

static RegisterTransformation<AggregateToScalar>
         Trans("aggregate-to-scalar", DescriptionMsg);

class ATSCollectionVisitor : public RecursiveASTVisitor<ATSCollectionVisitor> {
public:
  typedef RecursiveASTVisitor<ATSCollectionVisitor> Inherited;

  explicit ATSCollectionVisitor(AggregateToScalar *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitMemberExpr(MemberExpr *ME);

  bool VisitDeclStmt(DeclStmt *DS);

private:

  AggregateToScalar *ConsumerInstance;

};

bool ATSCollectionVisitor::VisitMemberExpr(MemberExpr *ME)
{
  ValueDecl *OrigDecl = ME->getMemberDecl();
  FieldDecl *FD = dyn_cast<FieldDecl>(OrigDecl);

  // in C++, getMemberDecl returns a CXXMethodDecl.
  TransAssert(FD && "Bad FD!\n");

  const Type *T = FD->getType().getTypePtr();
  if (!T->isScalarType())
    return false;

  RecordDecl *RD = FD->getParent();
  TransAssert(RD && "NULL RecordDecl!");
  if (!RD->isStruct() && !RD->isUnion())
    return false;

  FieldDecl *CanonicalDecl = dyn_cast<FieldDecl>(FD->getCanonicalDecl());
  TransAssert(CanonicalDecl);
  ConsumerInstance->addVarRefExpr(CanonicalDecl, ME);
  return false;
}

bool ATSCollectionVisitor::VisitDeclStmt(DeclStmt *DS)
{
  for (DeclStmt::decl_iterator I = DS->decl_begin(),
       E = DS->decl_end(); I != E; ++I) {
    VarDecl *CurrDecl = dyn_cast<VarDecl>(*I);
    if (CurrDecl)
      ConsumerInstance->VarDeclToDeclStmtMap[CurrDecl] = DS;
  }
  return true;
}

void AggregateToScalar::Initialize(ASTContext &context) 
{
  Context = &context;
  SrcManager = &Context->getSourceManager();
  AggregateAccessVisitor = new ATSCollectionVisitor(this);
  TheRewriter.setSourceMgr(Context->getSourceManager(), 
                           Context->getLangOptions());
}

void AggregateToScalar::HandleTopLevelDecl(DeclGroupRef D) 
{
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    VarDecl *VD = dyn_cast<VarDecl>(*I);
    if (VD)
      VarDeclToDeclGroupMap[VD] = D;

    AggregateAccessVisitor->TraverseDecl(*I);
  }
}
 
void AggregateToScalar::HandleTranslationUnit(ASTContext &Ctx)
{
  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  TransAssert(TheFieldDecl && "NULL TheFieldDecl!");

  handleTheFieldDecl(Ctx);

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

Expr *AggregateToScalar::ignoreSubscriptExprImpCasts(Expr *E,
        FieldIdxVector &FieldIdxs)
{
  ArraySubscriptExpr *ASE = dyn_cast<ArraySubscriptExpr>(E);
  if (!ASE)
    return E;

  Expr *IE = ASE->getIdx();
  unsigned int Idx = 0;
  llvm::APSInt Result;
  if (IE && IE->EvaluateAsInt(Result, *Context)) {
    std::string IntStr = Result.toString(10);
    std::stringstream TmpSS(IntStr);
    if (!(TmpSS >> Idx))
      TransAssert(0 && "Non-integer value!");
  }

  FieldIdxs.push_back(Idx);
  Expr *NewE = ASE->getBase()->IgnoreImpCasts();
  return ignoreSubscriptExprImpCasts(NewE, FieldIdxs);
}

VarDecl *AggregateToScalar::getRefVarDeclAndFieldIdxs(MemberExpr *ME,
           FieldIdxVector &FieldIdxs)
{
  ValueDecl *VD = ME->getMemberDecl();
  FieldDecl *FD = dyn_cast<FieldDecl>(VD);
  TransAssert(FD && "Bad FD!\n");
  unsigned int Idx = FD->getFieldIndex();
  FieldIdxs.push_back(Idx);
  
  Expr *E = ME->getBase()->IgnoreParens();

  Expr *NewE = ignoreSubscriptExprImpCasts(E, FieldIdxs);

  MemberExpr *M = dyn_cast<MemberExpr>(NewE);
  if (M) {
    return getRefVarDeclAndFieldIdxs(M, FieldIdxs);
  }

  DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(NewE);
  if (DRE) {
    ValueDecl *ValueD = DRE->getDecl();
    VarDecl *VarD = dyn_cast<VarDecl>(ValueD);
    TransAssert(VarD && "Invalid ref var!");
    return VarD->getCanonicalDecl();
  }

  TransAssert(0 && "Unreached code!");
}

bool AggregateToScalar::addTmpVar(VarDecl *VD,
                                  const std::string &VarName,
                                  const std::string *InitStr,
                                  ASTContext &Ctx)
{
  std::string VarStr(VarName);
  QualType QT = TheFieldDecl->getType();
  QT.getAsStringInternal(VarStr, Context->getPrintingPolicy());

  if (InitStr) {
    VarStr += " = ";
    VarStr += (*InitStr);
  }
  VarStr += ";";
   
  if (VD->getStorageClass() == SC_Static)
    VarStr = "static " + VarStr; 

  if (VD->isLocalVarDecl()) {
    DeclStmt *TheDeclStmt = VarDeclToDeclStmtMap[VD];
    TransAssert(TheDeclStmt && "NULL TheDeclStmt");
    return RewriteUtils::addStringAfterStmt(TheDeclStmt, VarStr, 
                                            &TheRewriter, SrcManager);
  }
  else {
    llvm::DenseMap<VarDecl *, DeclGroupRef>::iterator DI =
      VarDeclToDeclGroupMap.find(VD);
    TransAssert((DI != VarDeclToDeclGroupMap.end()) && 
                 "Cannot find VarDeclGroup!");
    VarDecl *LastVD = NULL;
    DeclGroupRef DR = (*DI).second;
    for (DeclGroupRef::iterator I = DR.begin(), E = DR.end(); I != E; ++I) {
      LastVD = dyn_cast<VarDecl>(*I);
    }
    TransAssert(LastVD && "Bad LastVD!");
    return RewriteUtils::addStringAfterVarDecl(LastVD, VarStr, 
                                               &TheRewriter, SrcManager);
  }
}

bool AggregateToScalar::replaceMemberExpr(MemberExpr *ME, const std::string &NewName)
{
  return RewriteUtils::replaceExpr(ME, NewName,
                                   &TheRewriter, SrcManager);
}

void AggregateToScalar::createNewVarName(VarDecl *VD,
                                         const FieldIdxVector &FieldIdxs,
                                         std::string &VarName)
{
  std::stringstream SS;
  SS << VD->getNameAsString();
  for (FieldIdxVector::const_reverse_iterator I = FieldIdxs.rbegin(),
       E = FieldIdxs.rend(); I != E; ++I) {
    SS << "_" << (*I);
  }

  VarName = SS.str();
}

void AggregateToScalar::getInitString(const FieldIdxVector &FieldIdxs,
                                      InitListExpr *ILE,
                                      std::string &InitStr)
{
  InitStr = "";
  InitListExpr *SubILE = ILE;
  Expr *Exp;
  unsigned int Count = 0;
  for (FieldIdxVector::const_reverse_iterator I = FieldIdxs.rbegin(),
       E = FieldIdxs.rend(); I != E; ++I) {
    Count++;
    unsigned int Idx;

    const Type *T = SubILE->getType().getTypePtr();
    if (T->isUnionType())
      Idx = 0;
    else
      Idx = (*I);

    TransAssert((Idx < SubILE->getNumInits()) && "Bad Init Index");
    Exp = SubILE->getInit(Idx);
    TransAssert(Exp && "NULL Exp!");

    SubILE = dyn_cast<InitListExpr>(Exp);
    if (!SubILE)
      break;
  }

  TransAssert(Count == FieldIdxs.size());
  RewriteUtils::getExprString(Exp, InitStr,
                              &TheRewriter, SrcManager);
}

bool AggregateToScalar::handleOneMemberExpr(MemberExpr *ME, ASTContext &Ctx)
{
  FieldIdxVector FieldIdxs;
  VarDecl *VD = getRefVarDeclAndFieldIdxs(ME, FieldIdxs);
  const Type *VarT = VD->getType().getTypePtr(); (void)VarT;
  TransAssert((VarT->isStructureType() || VarT->isUnionType() 
               || VarT->isArrayType()) && "Non-valid var type!");

  llvm::DenseMap<VarDecl *, std::string>::iterator I = 
    ProcessedVarDecls.find(VD);

  if (I != ProcessedVarDecls.end()) {
    return replaceMemberExpr(ME, (*I).second);
  }

  std::string VarName("");
  createNewVarName(VD, FieldIdxs, VarName);
  ProcessedVarDecls[VD] = VarName;

  replaceMemberExpr(ME, VarName);

  VarDecl *VDef = VD->getDefinition();
  if (VDef)
    VD = VDef;

  Expr *IE = VD->getInit();
  if (!IE)
    return addTmpVar(VD, VarName, NULL, Ctx);

  InitListExpr *ILE = dyn_cast<InitListExpr>(IE);
  if (!ILE) {
    TransAssert(dyn_cast<CXXConstructExpr>(IE));
    return addTmpVar(VD, VarName, NULL, Ctx);
  }

  std::string InitStr;
  getInitString(FieldIdxs, ILE, InitStr);
  return addTmpVar(VD, VarName, &InitStr, Ctx);
}

void AggregateToScalar::handleTheFieldDecl(ASTContext &Ctx)
{
  VarRefsSet *TheRefsSet = ValidFields[TheFieldDecl];
  TransAssert(!TheRefsSet->empty() && "VarRefsSet cannot be empty!");
  
  for (VarRefsSet::iterator I = TheRefsSet->begin(), E = TheRefsSet->end();
       I != E; ++I) {
    if (!handleOneMemberExpr(*I, Ctx))
      return;
  }
}

void AggregateToScalar::addVarRefExpr(FieldDecl *FD, MemberExpr *ME)
{
  ValidFieldsMap::iterator I = ValidFields.find(FD);
  VarRefsSet *VarRefs;

  if (I == ValidFields.end()) {
    ValidInstanceNum++;
    if (ValidInstanceNum == TransformationCounter) {
      TheFieldDecl = FD;
    }

    VarRefs = new VarRefsSet::set();
    ValidFields[FD] = VarRefs;
  }
  else {
    VarRefs = (*I).second;
  }

  VarRefs->insert(ME);
}

AggregateToScalar::~AggregateToScalar(void)
{
  if (AggregateAccessVisitor)
    delete AggregateAccessVisitor;

  for (ValidFieldsMap::iterator I = ValidFields.begin(),
       E = ValidFields.end(); I != E; ++I) {
    delete (*I).second;
  }
}

