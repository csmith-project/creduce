#include "RewriteUtils.h"

#include <cctype>
#include "clang/Basic/SourceManager.h"
#include "clang/Rewrite/Rewriter.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/TypeLoc.h"

// FIXME: This macro is copied from Transformation.h.
// I don't want to include Transformation.h 
// and hence incease dependency. Probably need to 
// put this macro into a common header file? 
#ifndef ENABLE_TRANS_ASSERT
  #define TransAssert(x) {if (!(x)) exit(0);}
#else
  #define TransAssert(x) assert(x)
#endif

using namespace clang;

static const char *DefaultIndentStr = "    ";

const char *RewriteUtils::TmpVarNamePrefix = "__trans_tmp_";

// copied from Rewriter.cpp
unsigned RewriteUtils::getLocationOffsetAndFileID(SourceLocation Loc,
                                                  FileID &FID,
                                                  SourceManager *SrcManager)
{
  assert(Loc.isValid() && "Invalid location");
  std::pair<FileID,unsigned> V = SrcManager->getDecomposedLoc(Loc);
  FID = V.first;
  return V.second;
}

SourceLocation RewriteUtils::getEndLocationFromBegin(SourceRange Range,
                                                     Rewriter *TheRewriter)
{
  int LocRangeSize = TheRewriter->getRangeSize(Range);
  if (LocRangeSize == -1)
    return Range.getEnd();

  SourceLocation StartLoc = Range.getBegin();

  return StartLoc.getLocWithOffset(LocRangeSize);
}

int RewriteUtils::getOffsetUntil(const char *Buf, char Symbol)
{
  int Offset = 0;
  while (*Buf != Symbol) {
    Buf++;
    Offset++;
  }
  return Offset;
}

int RewriteUtils::getSkippingOffset(const char *Buf, char Symbol)
{
  int Offset = 0;
  while (*Buf == Symbol) {
    Buf++;
    Offset++;
  }
  return Offset;
}

SourceLocation RewriteUtils::getEndLocationUntil(SourceRange Range, 
                                                char Symbol,
                                                Rewriter *TheRewriter,
                                                SourceManager *SrcManager)
{
  SourceLocation EndLoc = getEndLocationFromBegin(Range, TheRewriter);
    
  const char *EndBuf = SrcManager->getCharacterData(EndLoc);
  int Offset = getOffsetUntil(EndBuf, Symbol);
  return EndLoc.getLocWithOffset(Offset);
}

SourceLocation RewriteUtils::getEndLocationAfter(SourceRange Range, 
                                                char Symbol,
                                                Rewriter *TheRewriter,
                                                SourceManager *SrcManager)
{
  SourceLocation EndLoc = getEndLocationFromBegin(Range, TheRewriter);
    
  const char *EndBuf = SrcManager->getCharacterData(EndLoc);
  int Offset = getOffsetUntil(EndBuf, Symbol);
  Offset++;
  return EndLoc.getLocWithOffset(Offset);
}

SourceLocation RewriteUtils::getLocationAfter(SourceLocation StartLoc, 
                                              char Symbol,
                                              Rewriter *TheRewriter,
                                              SourceManager *SrcManager)
{
  const char *StartBuf = SrcManager->getCharacterData(StartLoc);
  int Offset = getSkippingOffset(StartBuf, Symbol);
  return StartLoc.getLocWithOffset(Offset);
}

SourceLocation RewriteUtils::getSubstringLocation(SourceLocation StartLoc,
                                              size_t Size,
                                              const std::string &Substr,
                                              Rewriter *TheRewriter,
                                              SourceManager *SrcManager)
{
  const char *StartBuf = SrcManager->getCharacterData(StartLoc);
  std::string TmpStr(StartBuf, Size);

  size_t Pos = TmpStr.find(Substr);
  TransAssert((Pos != std::string::npos) && "Bad Name Position!");

  if (Pos == 0)
    return StartLoc;
  else 
    return StartLoc.getLocWithOffset(Pos);
}

bool RewriteUtils::removeParamFromFuncDecl(const ParmVarDecl *PV,
                                           unsigned int NumParams,
                                           int ParamPos,
                                           Rewriter *TheRewriter,
                                           SourceManager *SrcManager)
{
  SourceRange ParamLocRange = PV->getSourceRange();
  SourceLocation StartLoc = ParamLocRange.getBegin();
  int RangeSize = 
    TheRewriter->getRangeSize(ParamLocRange);

  if (RangeSize == -1)
    return false;

  // The param is the only parameter of the function declaration.
  // Replace it with void
  if ((ParamPos == 0) && (NumParams == 1)) {
    // Note that ')' is included in ParamLocRange for unnamed parameter
    if (PV->getDeclName())
      return !(TheRewriter->ReplaceText(StartLoc,
                                        RangeSize, "void"));
    else
      return !(TheRewriter->ReplaceText(StartLoc,
                                        RangeSize - 1, "void"));
  }

  // The param is the last parameter
  if (ParamPos == static_cast<int>(NumParams - 1)) {
    int Offset = 0;
    const char *StartBuf = 
      SrcManager->getCharacterData(StartLoc);

    TransAssert(StartBuf && "Invalid start buffer!");
    while (*StartBuf != ',') {
      StartBuf--;
      Offset--;
    }

    SourceLocation NewStartLoc = StartLoc.getLocWithOffset(Offset);

    // Note that ')' is included in ParamLocRange for unnamed parameter
    if (PV->getDeclName())
      return !(TheRewriter->RemoveText(NewStartLoc, 
                                       RangeSize - Offset));
    else
      return !(TheRewriter->RemoveText(NewStartLoc, 
                                       RangeSize - Offset - 1));
  }
 
  // Clang gives inconsistent RangeSize for named and unnamed parameter decls.
  // For example, for the first parameter, 
  //   foo(int, int);  -- RangeSize is 4, i.e., "," is counted
  //   foo(int x, int);  -- RangeSize is 5, i.e., ","is not included
  if (PV->getDeclName()) {
    // We cannot use the code below:
    //   SourceLocation EndLoc = ParamLocRange.getEnd();
    //   const char *EndBuf = 
    //     ConsumerInstance->SrcManager->getCharacterData(EndLoc);
    // Because getEnd() returns the start of the last token if this
    // is a token range. For example, in the above example, 
    // getEnd() points to the start of "x"
    // See the comments on getRangeSize in clang/lib/Rewriter/Rewriter.cpp
    int NewRangeSize = 0;
    const char *StartBuf = 
      SrcManager->getCharacterData(StartLoc);

    while (NewRangeSize < RangeSize) {
      StartBuf++;
      NewRangeSize++;
    }

    TransAssert(StartBuf && "Invalid start buffer!");
    while (*StartBuf != ',') {
      StartBuf++;
      NewRangeSize++;
    }

    return !(TheRewriter->RemoveText(StartLoc, 
                                     NewRangeSize + 1));
  }
  else {
    return !(TheRewriter->RemoveText(StartLoc, RangeSize));
  }
}

bool RewriteUtils::removeArgFromCallExpr(CallExpr *CallE,
                                        int ParamPos,
                                        Rewriter *TheRewriter,
                                        SourceManager *SrcManager)
{
  Expr *Arg = CallE->getArg(ParamPos);
  TransAssert(Arg && "Null arg!");

  SourceRange ArgRange = Arg->getSourceRange();
  int RangeSize = TheRewriter->getRangeSize(ArgRange);

  if (RangeSize == -1)
    return false;

  SourceLocation StartLoc = ArgRange.getBegin();
  unsigned int NumArgs = CallE->getNumArgs();

  if ((ParamPos == 0) && (NumArgs == 1)) {
    // Note that ')' is included in ParamLocRange
    return !(TheRewriter->RemoveText(ArgRange));
  }

  // The param is the last parameter
  if (ParamPos == static_cast<int>(NumArgs - 1)) {
    int Offset = 0;
    const char *StartBuf = SrcManager->getCharacterData(StartLoc);

    TransAssert(StartBuf && "Invalid start buffer!");
    while (*StartBuf != ',') {
      StartBuf--;
      Offset--;
    }

    SourceLocation NewStartLoc = StartLoc.getLocWithOffset(Offset);
    return !(TheRewriter->RemoveText(NewStartLoc,
                                     RangeSize - Offset));
  }

  int NewRangeSize = 0;
  const char *StartBuf = SrcManager->getCharacterData(StartLoc);

  TransAssert(StartBuf && "Invalid start buffer!");
  while (NewRangeSize < RangeSize) {
    StartBuf++;
    NewRangeSize++;
  }

  TransAssert(StartBuf && "Invalid start buffer!");
  while (*StartBuf != ',') {
    StartBuf++;
    NewRangeSize++;
  }

  TheRewriter->RemoveText(StartLoc, NewRangeSize + 1);
  return true;
}

SourceLocation RewriteUtils::getVarDeclTypeLocEnd(VarDecl *VD,
                                                  Rewriter *TheRewriter)
{
  TypeLoc VarTypeLoc = VD->getTypeSourceInfo()->getTypeLoc();

  TypeLoc NextTL = VarTypeLoc.getNextTypeLoc();
  while (!NextTL.isNull()) {
    VarTypeLoc = NextTL;
    NextTL = NextTL.getNextTypeLoc();
  }

  SourceRange TypeLocRange = VarTypeLoc.getSourceRange();
  SourceLocation EndLoc = 
    getEndLocationFromBegin(TypeLocRange, TheRewriter);
  return EndLoc;
}

bool RewriteUtils::removeVarFromDeclStmt(DeclStmt *DS,
                                         VarDecl *VD,
                                         Decl *PrevDecl,
                                         bool IsFirstDecl,
                                         Rewriter *TheRewriter,
                                         SourceManager *SrcManager)
{
  SourceRange StmtRange = DS->getSourceRange();

  // VD is the the only declaration, so it is safe to remove the entire stmt
  if (DS->isSingleDecl()) {
    return !(TheRewriter->RemoveText(StmtRange));
  }

  SourceRange VarRange = VD->getSourceRange();

  // VD is the first declaration in a declaration group.
  // We keep the leading type string
  if (IsFirstDecl) {
    SourceLocation NewStartLoc = getVarDeclTypeLocEnd(VD, TheRewriter);

    SourceLocation NewEndLoc = 
      getEndLocationUntil(VarRange, ',', TheRewriter, SrcManager);
    
    return 
      !(TheRewriter->RemoveText(SourceRange(NewStartLoc, NewEndLoc)));
  }

  TransAssert(PrevDecl && "PrevDecl cannot be NULL!");
  SourceLocation VarEndLoc = VarRange.getEnd();
  SourceRange PrevDeclRange = PrevDecl->getSourceRange();
  SourceLocation PrevDeclEndLoc = 
    getEndLocationUntil(PrevDeclRange, ',', TheRewriter, SrcManager);

  return !(TheRewriter->RemoveText(SourceRange(PrevDeclEndLoc, VarEndLoc)));
}

bool RewriteUtils::getExprString(const Expr *E, 
                                 std::string &ES,
                                 Rewriter *TheRewriter,
                                 SourceManager *SrcManager)
{
  SourceRange ExprRange = E->getSourceRange();
   
  int RangeSize = TheRewriter->getRangeSize(ExprRange);
  if (RangeSize == -1)
    return false;

  SourceLocation StartLoc = ExprRange.getBegin();
  const char *StartBuf = SrcManager->getCharacterData(StartLoc);

  ES.assign(StartBuf, RangeSize);

  const BinaryOperator *BinOp = dyn_cast<BinaryOperator>(E);

  // Keep the semantics of Comma operator
  if (BinOp && (BinOp->getOpcode() == BO_Comma))
    ES = "(" + ES + ")";

  return true;
}

bool RewriteUtils::replaceExpr(const Expr *E, 
                               const std::string &ES,
                               Rewriter *TheRewriter,
                               SourceManager *SrcManager)
{
  SourceRange ExprRange = E->getSourceRange();
   
  int RangeSize = TheRewriter->getRangeSize(ExprRange);
  if (RangeSize == -1)
    return false;

  return !(TheRewriter->ReplaceText(ExprRange, ES));
}

std::string RewriteUtils::getStmtIndentString(Stmt *S,
                                          SourceManager *SrcManager)
{
  SourceLocation StmtStartLoc = S->getLocStart();

  FileID FID;
  unsigned StartOffet = 
    getLocationOffsetAndFileID(StmtStartLoc, FID, SrcManager);

  StringRef MB = SrcManager->getBufferData(FID);
 
  unsigned lineNo = SrcManager->getLineNumber(FID, StartOffet) - 1;
  const SrcMgr::ContentCache *
      Content = SrcManager->getSLocEntry(FID).getFile().getContentCache();
  unsigned lineOffs = Content->SourceLineCache[lineNo];
 
  // Find the whitespace at the start of the line.
  StringRef indentSpace;

  unsigned i = lineOffs;
  while (isspace(MB[i]))
    ++i;
  indentSpace = MB.substr(lineOffs, i-lineOffs);

  return indentSpace;
}

bool RewriteUtils::addLocalVarToFunc(const std::string &VarStr,
                                     FunctionDecl *FD,
                                     Rewriter *TheRewriter,
                                     SourceManager *SrcManager)
{
  Stmt *Body = FD->getBody();
  TransAssert(Body && "NULL body for a function definition!");

  std::string IndentStr;
  StmtIterator I = Body->child_begin();

  if (I == Body->child_end())
    IndentStr = DefaultIndentStr;
  else
    IndentStr = getStmtIndentString((*I), SrcManager);

  std::string NewVarStr = "\n" + IndentStr + VarStr;
  SourceLocation StartLoc = Body->getLocStart();
  return !(TheRewriter->InsertTextAfterToken(StartLoc, NewVarStr));
}

const char *RewriteUtils::getTmpVarNamePrefix(void)
{
  return TmpVarNamePrefix;
}

bool RewriteUtils::addNewAssignStmtBefore(Stmt *BeforeStmt,
                                          const std::string &VarName,
                                          Expr *RHS,
                                          bool NeedParen,
                                          Rewriter *TheRewriter,
                                          SourceManager *SrcManager)
{
  std::string IndentStr = 
    RewriteUtils::getStmtIndentString(BeforeStmt, SrcManager);

  if (NeedParen) {
    SourceRange StmtRange = BeforeStmt->getSourceRange();
    SourceLocation LocEnd = 
      RewriteUtils::getEndLocationFromBegin(StmtRange, TheRewriter);

    std::string PostStr = "\n" + IndentStr + "}";
    if (TheRewriter->InsertTextAfterToken(LocEnd, PostStr))
      return false;
  }

  SourceLocation StmtLocStart = BeforeStmt->getLocStart();

  std::string ExprStr;
  RewriteUtils::getExprString(RHS, ExprStr,
                              TheRewriter, SrcManager);

  std::string AssignStmtStr;
  
  if (NeedParen) {
    AssignStmtStr = "{\n";
    AssignStmtStr += IndentStr + "  " + VarName + " = ";
    AssignStmtStr += ExprStr;
    AssignStmtStr += ";\n" + IndentStr + "  ";
  }
  else {
    AssignStmtStr = VarName + " = ";
    AssignStmtStr += ExprStr;
    AssignStmtStr += ";\n" + IndentStr;
  }
  
  return !(TheRewriter->InsertText(StmtLocStart, 
             AssignStmtStr, /*InsertAfter=*/false));
}

bool RewriteUtils::addStringAfterStmt(Stmt *AfterStmt, 
                                      const std::string &Str,
                                      Rewriter *TheRewriter,
                                      SourceManager *SrcManager)
{
  std::string IndentStr = 
    RewriteUtils::getStmtIndentString(AfterStmt, SrcManager);

  std::string NewStr = "\n" + IndentStr + Str;
  SourceRange StmtRange = AfterStmt->getSourceRange();
  SourceLocation LocEnd = 
    RewriteUtils::getEndLocationFromBegin(StmtRange, TheRewriter);
  
  return !(TheRewriter->InsertText(LocEnd, NewStr));
}

bool RewriteUtils::addStringAfterVarDecl(VarDecl *VD,
                                         const std::string &Str,
                                         Rewriter *TheRewriter,
                                         SourceManager *SrcManager)
{
  SourceRange VarRange = VD->getSourceRange();
  SourceLocation LocEnd = 
    RewriteUtils::getEndLocationAfter(VarRange, ';', TheRewriter, SrcManager);
  
  return !(TheRewriter->InsertText(LocEnd, "\n" + Str));
}

bool RewriteUtils::isTheFirstVarDecl(VarDecl *VD,
                                     SourceManager *SrcManager)
{
  SourceRange VarRange = VD->getSourceRange();
  SourceLocation VarStartLoc = VarRange.getBegin();
  const char *VarStartBuf = SrcManager->getCharacterData(VarStartLoc);

  TypeLoc VarTypeLoc = VD->getTypeSourceInfo()->getTypeLoc();
  SourceRange TypeRange = VarTypeLoc.getSourceRange();
  SourceLocation TypeStartLoc = TypeRange.getBegin();
  const char *TypeStartBuf = SrcManager->getCharacterData(TypeStartLoc);

  return (VarStartBuf == TypeStartBuf);
}

bool RewriteUtils::replaceVarDeclName(VarDecl *VD,
                                      const std::string &NameStr,
                                      Rewriter *TheRewriter,
                                      SourceManager *SrcManager)
{
  SourceRange VarRange = VD->getSourceRange();
  SourceLocation NameLocStart;

  if (isTheFirstVarDecl(VD, SrcManager)) {
    SourceLocation TypeLocEnd = getVarDeclTypeLocEnd(VD, TheRewriter);
    int VarRangeSize = TheRewriter->getRangeSize(VarRange);
    if (VarRangeSize == -1)
      return false;

    NameLocStart = 
      getSubstringLocation(TypeLocEnd, VarRangeSize, VD->getNameAsString(), 
                           TheRewriter, SrcManager);
  }
  else {
    NameLocStart = VarRange.getBegin();
  }

  return !(TheRewriter->ReplaceText(NameLocStart, 
             VD->getNameAsString().size(), NameStr));
}

bool RewriteUtils::replaceFunctionDeclName(FunctionDecl *FD,
                                      const std::string &NameStr,
                                      Rewriter *TheRewriter,
                                      SourceManager *SrcManager)
{
  return !TheRewriter->ReplaceText(FD->getNameInfo().getLoc(),
                                   FD->getNameAsString().length(),
                                   NameStr);
}

