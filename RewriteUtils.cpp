#include "RewriteUtils.h"

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

SourceLocation RewriteUtils::getEndLocationFromBegin(SourceRange Range,
                                                     Rewriter *TheRewriter)
{
  int LocRangeSize = TheRewriter->getRangeSize(Range);
  if (LocRangeSize == -1)
    return Range.getEnd();

  SourceLocation StartLoc = Range.getBegin();

  return StartLoc.getLocWithOffset(LocRangeSize);
}

SourceLocation RewriteUtils::getEndLocationUtil(SourceRange Range, 
                                                char Symbol,
                                                Rewriter *TheRewriter,
                                                SourceManager *SrcManager)
{
  SourceLocation EndLoc = getEndLocationFromBegin(Range, TheRewriter);
    
  const char *EndBuf = SrcManager->getCharacterData(EndLoc);
  int Offset = 0;
  while (*EndBuf != ',') {
    EndBuf++;
    Offset++;
  }
  return EndLoc.getLocWithOffset(Offset);
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
    TypeLoc VarTypeLoc = VD->getTypeSourceInfo()->getTypeLoc();

    TypeLoc NextTL = VarTypeLoc.getNextTypeLoc();
    while (!NextTL.isNull()) {
      VarTypeLoc = NextTL;
      NextTL = NextTL.getNextTypeLoc();
    }

    SourceRange TypeLocRange = VarTypeLoc.getSourceRange();
    SourceLocation NewStartLoc = 
      getEndLocationFromBegin(TypeLocRange, TheRewriter);

    SourceLocation NewEndLoc = 
      getEndLocationUtil(VarRange, ',', TheRewriter, SrcManager);
    
    return 
      !(TheRewriter->RemoveText(SourceRange(NewStartLoc, NewEndLoc)));
  }

  TransAssert(PrevDecl && "PrevDecl cannot be NULL!");
  SourceLocation VarEndLoc = VarRange.getEnd();
  SourceRange PrevDeclRange = PrevDecl->getSourceRange();
  SourceLocation PrevDeclEndLoc = 
    getEndLocationUtil(PrevDeclRange, ',', TheRewriter, SrcManager);

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
  return true;
}

bool RewriteUtils::addLocalVarToFunc(const std::string &VarStr,
                                     FunctionDecl *FD,
                                     Rewriter *TheRewriter)
{
  Stmt *Body = FD->getBody();
  TransAssert(Body && "NULL body for a function definition!");

  SourceLocation StartLoc = Body->getLocStart();
  return !(TheRewriter->InsertTextAfterToken(StartLoc, 
                                            VarStr));
}
