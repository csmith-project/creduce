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

const Expr *
Transformation::ignoreSubscriptExprParenCasts(const Expr *E)
{
  const Expr *NewE = E->IgnoreParenCasts();
  const ArraySubscriptExpr *ASE;
  while (true) {
    ASE = dyn_cast<ArraySubscriptExpr>(NewE);
    if (!ASE)
      break;
    NewE = ASE->getBase()->IgnoreParenCasts();
  }
  TransAssert(NewE && "NULL NewE!");
  return NewE;
}

const Expr *Transformation::getInitExprByIndex(IndexVector &Idxs,
                                                    const InitListExpr *ILE)
{
  const InitListExpr *SubILE = ILE;
  const Expr *Exp = NULL;
  unsigned int Count = 0;
  for (IndexVector::const_reverse_iterator I = Idxs.rbegin(),
       E = Idxs.rend(); I != E; ++I) {
    Count++;
    unsigned int Idx;

    const Type *T = SubILE->getType().getTypePtr();
    if (T->isUnionType())
      Idx = 0;
    else
      Idx = (*I);

    // Incomplete initialization list
    if (Idx >= SubILE->getNumInits())
      return NULL;

    Exp = SubILE->getInit(Idx);
    TransAssert(Exp && "NULL Exp!");

    SubILE = dyn_cast<InitListExpr>(Exp);
    if (!SubILE)
      break;
  }

  TransAssert(Exp && "Exp cannot be NULL");
  TransAssert(Count == Idxs.size());
  return Exp;
}

const Expr *Transformation::getArrayBaseExprAndIdxs(
        const ArraySubscriptExpr *ASE, IndexVector &Idxs)
{
  const Expr *BaseE;
  while (ASE) {
    const Expr *IdxE = ASE->getIdx();
    unsigned int Idx = 0;
    llvm::APSInt Result;
    if (IdxE && IdxE->EvaluateAsInt(Result, *Context)) {
      std::string IntStr = Result.toString(10);
      std::stringstream TmpSS(IntStr);
      if (!(TmpSS >> Idx))
        TransAssert(0 && "Non-integer value!");
    }
    BaseE = ASE->getBase()->IgnoreParenCasts();
    ASE = dyn_cast<ArraySubscriptExpr>(BaseE);
    Idxs.push_back(Idx);
  }
  return BaseE;
}

const Expr *Transformation::getInitExprFromBase(const Expr *BaseE,
                                                    IndexVector &Idxs)
{
  TransAssert(BaseE && "Bad Array Base Expression!");
  const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(BaseE);
  TransAssert(DRE && "Bad DeclRefExpr!");
  const ValueDecl *OrigDecl = DRE->getDecl();
  const VarDecl *VD = dyn_cast<VarDecl>(OrigDecl);
  TransAssert(VD && "Bad VarDecl!");
  const Expr *InitE = VD->getAnyInitializer();

  if (!InitE)
    return NULL;

  const InitListExpr *ILE = dyn_cast<InitListExpr>(InitE);
  TransAssert(ILE && "Bad InitListExpr!");
  return getInitExprByIndex(Idxs, ILE);
}

const Expr *Transformation::getArraySubscriptElem(
              const ArraySubscriptExpr *ASE)
{
  IndexVector ArrayDims;
  const Expr *BaseE = getArrayBaseExprAndIdxs(ASE, ArrayDims);
  return getInitExprFromBase(BaseE, ArrayDims);
}

const Expr *Transformation::getMemberExprBaseExprAndIdxs(
        const MemberExpr *ME, IndexVector &Idxs)
{
  const Expr *BaseE = NULL;
  while (ME) {
    ValueDecl *VD = ME->getMemberDecl();
    FieldDecl *FD = dyn_cast<FieldDecl>(VD);
    TransAssert(FD && "Bad FD!\n");
    unsigned int Idx = FD->getFieldIndex();
    Idxs.push_back(Idx);
  
    BaseE = ME->getBase()->IgnoreParenCasts();
    const ArraySubscriptExpr *ASE = dyn_cast<ArraySubscriptExpr>(BaseE);
    if (ASE) {
      BaseE = getArrayBaseExprAndIdxs(ASE, Idxs);
      if (!BaseE)
        return NULL;
    }
    ME = dyn_cast<MemberExpr>(BaseE);
  }
  return BaseE;
}

const Expr *Transformation::getMemberExprElem(const MemberExpr *ME)
{
  IndexVector Idxs;
  const Expr *BaseE = getMemberExprBaseExprAndIdxs(ME, Idxs);
  return getInitExprFromBase(BaseE, Idxs);
}

unsigned int Transformation::getArrayDimension(const ArrayType *ArrayTy)
{
  unsigned int Dim = 1;
  const Type *ArrayElemTy = ArrayTy->getElementType().getTypePtr();
  while (ArrayElemTy->isArrayType()) {
    const ArrayType *AT = dyn_cast<ArrayType>(ArrayElemTy);
    ArrayElemTy = AT->getElementType().getTypePtr();
    Dim++;
  }
  return Dim;
}

const Type *Transformation::getArrayBaseElemType(const ArrayType *ArrayTy)
{
  const Type *ArrayElemTy = ArrayTy->getElementType().getTypePtr();
  while (ArrayElemTy->isArrayType()) {
    const ArrayType *AT = dyn_cast<ArrayType>(ArrayElemTy);
    ArrayElemTy = AT->getElementType().getTypePtr();
  }
  TransAssert(ArrayElemTy && "Bad Array Element Type!");
  return ArrayElemTy;
}

