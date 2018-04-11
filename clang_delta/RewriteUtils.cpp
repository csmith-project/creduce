//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2014, 2015, 2016, 2017, 2018 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "RewriteUtils.h"

#include <cctype>
#include <sstream>
#include "clang/Basic/SourceManager.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/Expr.h"
#include "clang/AST/TypeLoc.h"
#include "clang/AST/ExprCXX.h"

using namespace clang;

static const char *DefaultIndentStr = "    ";

RewriteUtils *RewriteUtils::Instance;

const char *RewriteUtils::TmpVarNamePrefix = "__trans_tmp_";

RewriteUtils *RewriteUtils::GetInstance(Rewriter *RW)
{
  if (RewriteUtils::Instance) {
    RewriteUtils::Instance->TheRewriter = RW;
    RewriteUtils::Instance->SrcManager = &(RW->getSourceMgr());
    return RewriteUtils::Instance;
  }

  RewriteUtils::Instance = new RewriteUtils();
  assert(RewriteUtils::Instance);

  RewriteUtils::Instance->TheRewriter = RW;
  RewriteUtils::Instance->SrcManager = &(RW->getSourceMgr());
  return RewriteUtils::Instance;
}

void RewriteUtils::Finalize(void)
{
  if (RewriteUtils::Instance) {
    delete RewriteUtils::Instance;
    RewriteUtils::Instance = NULL;
  }
}

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

unsigned RewriteUtils::getOffsetBetweenLocations(SourceLocation StartLoc,
                                            SourceLocation EndLoc,
                                            SourceManager *SrcManager)
{
  FileID FID;
  unsigned StartOffset = 
    getLocationOffsetAndFileID(StartLoc, FID, SrcManager);
  unsigned EndOffset = 
    getLocationOffsetAndFileID(EndLoc, FID, SrcManager);
  TransAssert((EndOffset >= StartOffset) && "Bad locations!");
  return (EndOffset - StartOffset);
}

SourceLocation RewriteUtils::getEndLocationFromBegin(SourceRange Range)
{
  SourceLocation StartLoc = Range.getBegin();
  SourceLocation EndLoc = Range.getEnd();
  if (StartLoc.isInvalid())
    return StartLoc;
  if (EndLoc.isInvalid())
    return EndLoc;

  if (StartLoc.isMacroID())
    StartLoc = SrcManager->getFileLoc(StartLoc);
  if (EndLoc.isMacroID())
    EndLoc = SrcManager->getFileLoc(EndLoc);

  SourceRange NewRange(StartLoc, EndLoc);
  int LocRangeSize = TheRewriter->getRangeSize(NewRange);
  if (LocRangeSize == -1)
    return NewRange.getEnd();

  return StartLoc.getLocWithOffset(LocRangeSize);
}

int RewriteUtils::getOffsetUntil(const char *Buf, char Symbol)
{
  int Offset = 0;
  while (*Buf != Symbol) {
    Buf++;
    if (*Buf == '\0')
      break;
    Offset++;
  }
  return Offset;
}

int RewriteUtils::getSkippingOffset(const char *Buf, char Symbol)
{
  int Offset = 0;
  while (*Buf == Symbol) {
    Buf++;
    if (*Buf == '\0')
      break;
    Offset++;
  }
  return Offset;
}

SourceLocation RewriteUtils::getEndLocationUntil(SourceRange Range, 
                                                 char Symbol)
{
  SourceLocation EndLoc = getEndLocationFromBegin(Range);
  if (EndLoc.isInvalid())
    return EndLoc;
    
  const char *EndBuf = SrcManager->getCharacterData(EndLoc);
  int Offset = getOffsetUntil(EndBuf, Symbol);
  return EndLoc.getLocWithOffset(Offset);
}

SourceLocation RewriteUtils::getLocationUntil(SourceLocation Loc, 
                                              char Symbol)
{
  const char *Buf = SrcManager->getCharacterData(Loc);
  int Offset = getOffsetUntil(Buf, Symbol);
  return Loc.getLocWithOffset(Offset);
}

SourceLocation RewriteUtils::getEndLocationAfter(SourceRange Range, 
                                                char Symbol)
{
  SourceLocation EndLoc = getEndLocationFromBegin(Range);
  if (EndLoc.isInvalid())
    return EndLoc;
    
  const char *EndBuf = SrcManager->getCharacterData(EndLoc);
  int Offset = getOffsetUntil(EndBuf, Symbol);
  Offset++;
  return EndLoc.getLocWithOffset(Offset);
}

SourceLocation RewriteUtils::getLocationAfter(SourceLocation Loc, 
                                                char Symbol)
{
  const char *Buf = SrcManager->getCharacterData(Loc);
  int Offset = getOffsetUntil(Buf, Symbol);
  Offset++;
  return Loc.getLocWithOffset(Offset);
}

SourceLocation RewriteUtils::getLocationAfterSkiping(SourceLocation StartLoc, 
                                              char Symbol)
{
  const char *StartBuf = SrcManager->getCharacterData(StartLoc);
  int Offset = getSkippingOffset(StartBuf, Symbol);
  return StartLoc.getLocWithOffset(Offset);
}

SourceLocation RewriteUtils::getParamSubstringLocation(SourceLocation StartLoc,
                                              size_t Size,
                                              const std::string &Substr)
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
                                           int ParamPos)
{
  SourceRange ParamLocRange = PV->getSourceRange();
  int RangeSize;
 
  SourceLocation StartLoc = ParamLocRange.getBegin();
  SourceLocation EndLoc = ParamLocRange.getEnd();
  if (StartLoc.isInvalid() && EndLoc.isInvalid()) {
    return false;
  }
  else if (StartLoc.isInvalid()) {
    StartLoc = EndLoc;
    RangeSize = PV->getNameAsString().size();
  }
  else if (EndLoc.isInvalid()) {
    const char *Buf = SrcManager->getCharacterData(StartLoc);
    if ((ParamPos == 0) && (NumParams == 1)) {
      RangeSize = getOffsetUntil(Buf, ')');
    }
    else {
      RangeSize = getOffsetUntil(Buf, ',');
    }
  }
  else {
    RangeSize = TheRewriter->getRangeSize(ParamLocRange);
    if (RangeSize == -1)
      return false;
  }

  // The param is the only parameter of the function declaration.
  // Replace it with void
  if ((ParamPos == 0) && (NumParams == 1)) {
    return !(TheRewriter->ReplaceText(StartLoc,
                                      RangeSize, "void"));
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

    return !(TheRewriter->RemoveText(NewStartLoc, RangeSize - Offset));
  }
 
  // We cannot use the code below:
  //   SourceLocation EndLoc = ParamLocRange.getEnd();
  //   const char *EndBuf =
  //     ConsumerInstance->SrcManager->getCharacterData(EndLoc);
  // Because getEnd() returns the start of the last token if this
  // is a token range. For example, in the above example,
  // getEnd() points to the start of "x"
  // See the comments on getRangeSize in clang/lib/Rewriter/Rewriter.cpp
  int NewRangeSize = 0;
  const char *StartBuf = SrcManager->getCharacterData(StartLoc);

  while (NewRangeSize < RangeSize) {
    StartBuf++;
    NewRangeSize++;
  }

  TransAssert(StartBuf && "Invalid start buffer!");
  // FIXME: This isn't really correct for processing old-style function
  // declarations, but just let's live with it for now.
  while (*StartBuf != ',' && *StartBuf != ';') {
    StartBuf++;
    NewRangeSize++;
  }

  return !(TheRewriter->RemoveText(StartLoc, NewRangeSize + 1));
}

// Handle CXXConstructExpr and CallExpr.
// These two do not inherit each other, and we need a couple of 
// `common` member functions from them. 
// Is this too ugly? Any better way to do this? 
const Expr *RewriteUtils::getArgWrapper(const Expr *E,
                                        int ParamPos)
{
  const CXXConstructExpr *CtorE = dyn_cast<CXXConstructExpr>(E);
  if (CtorE)
    return CtorE->getArg(ParamPos);

  const CallExpr *CE = dyn_cast<CallExpr>(E);
  if (CE)
    return CE->getArg(ParamPos);

  TransAssert(0 && "Invalid Expr!");
  return NULL;
}

unsigned RewriteUtils::getNumArgsWrapper(const Expr *E)
{
  const CXXConstructExpr *CtorE = dyn_cast<CXXConstructExpr>(E);
  if (CtorE)
    return CtorE->getNumArgs();

  const CallExpr *CE = dyn_cast<CallExpr>(E);
  if (CE)
    return CE->getNumArgs();

  TransAssert(0 && "Invalid Expr!");
  return 0;
}

bool RewriteUtils::removeArgFromExpr(const Expr *E,
                                     int ParamPos)
{
  
  if (ParamPos >= static_cast<int>(getNumArgsWrapper(E)))
    return true;

  const Expr *Arg = getArgWrapper(E, ParamPos);
  TransAssert(Arg && "Null arg!");
  if (dyn_cast<CXXDefaultArgExpr>(Arg->IgnoreParenCasts()))
    return true;

  SourceRange ArgRange = Arg->getSourceRange();
  int RangeSize = TheRewriter->getRangeSize(ArgRange);

  if (RangeSize == -1)
    return false;

  SourceLocation StartLoc = ArgRange.getBegin();
  unsigned int NumArgs = getNumArgsWrapper(E);

  if ((ParamPos == 0) && ((NumArgs == 1) ||
                          ((NumArgs > 1) && 
                           dyn_cast<CXXDefaultArgExpr>(
                           getArgWrapper(E, 1)->IgnoreParenCasts())))) {
    // Note that ')' is included in ParamLocRange
    return !(TheRewriter->RemoveText(ArgRange));
  }

  int LastArgPos = static_cast<int>(NumArgs - 1);
  // The param is the last non-default parameter
  if ((ParamPos == LastArgPos) ||
      ((ParamPos < LastArgPos) &&
        dyn_cast<CXXDefaultArgExpr>(
          getArgWrapper(E, ParamPos+1)->IgnoreParenCasts()))) {
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

  // We cannot use SrcManager->getCharacterData(StartLoc) to get the buffer,
  // because it returns the unmodified string. I've tried to use 
  // getEndlocationUntil(ArgRange, ",", ...) call, but still failed. 
  // Seems in some cases, it returns bad results for a complex case like:
  //  foo(...foo(...), ...)
  // So I ended up with this ugly way - get the end loc from the next arg.
  const Expr *NextArg = getArgWrapper(E, ParamPos+1);
  SourceRange NextArgRange = NextArg->getSourceRange();
  SourceLocation NextStartLoc = NextArgRange.getBegin();
  const char *NextStartBuf = SrcManager->getCharacterData(NextStartLoc);
  int Offset = 0;
  while (*NextStartBuf != ',') {
      NextStartBuf--;
      Offset--;
  }

  SourceLocation NewEndLoc = NextStartLoc.getLocWithOffset(Offset);
  return !TheRewriter->RemoveText(SourceRange(StartLoc, NewEndLoc));
}

bool RewriteUtils::removeArgFromCXXConstructExpr(const CXXConstructExpr *CE,
                                                 int ParamPos)
{
  return removeArgFromExpr(CE, ParamPos);
}

bool RewriteUtils::removeArgFromCallExpr(const CallExpr *CallE,
                                         int ParamPos)
{
  return removeArgFromExpr(CallE, ParamPos);
}

void RewriteUtils::skipRangeByType(const std::string &BufStr, 
                                   const Type *Ty,
                                   int &Offset)
{
  Offset = 0;
  int BufSz = static_cast<int>(BufStr.size());
  size_t Pos;
  while (Offset < BufSz) {
    if (isspace(BufStr[Offset])) {
      Offset++;
      continue;
    }

    Pos = BufStr.find("char", Offset);
    if (Pos != std::string::npos) {
      Offset += 4;
      continue;
    }

    Pos = BufStr.find("int", Offset);
    if (Pos != std::string::npos) {
      Offset += 3;
      continue;
    }

    Pos = BufStr.find("short", Offset);
    if (Pos != std::string::npos) {
      Offset += 5;
      continue;
    }

    Pos = BufStr.find("long", Offset);
    if (Pos != std::string::npos) {
      Offset += 4;
      continue;
    }

    return;
  }
}

SourceLocation RewriteUtils::skipPossibleTypeRange(const Type *Ty,
                                                   SourceLocation OrigEndLoc,
                                                   SourceLocation VarStartLoc)
{
  if (!Ty->isIntegerType())
    return OrigEndLoc;

  int Offset;
  std::string BufStr;
  getStringBetweenLocs(BufStr, OrigEndLoc, VarStartLoc);
  skipRangeByType(BufStr, Ty, Offset);

  return OrigEndLoc.getLocWithOffset(Offset);
}

SourceLocation RewriteUtils::getVarDeclTypeLocBegin(const VarDecl *VD)
{
  TypeLoc VarTypeLoc = VD->getTypeSourceInfo()->getTypeLoc();

  TypeLoc NextTL = VarTypeLoc.getNextTypeLoc();
  while (!NextTL.isNull()) {
    VarTypeLoc = NextTL;
    NextTL = NextTL.getNextTypeLoc();
  }

  return VarTypeLoc.getLocStart();
}

SourceLocation RewriteUtils::getVarDeclTypeLocEnd(const VarDecl *VD)
{
  TypeLoc VarTypeLoc = VD->getTypeSourceInfo()->getTypeLoc();
  const IdentifierInfo *Id = VD->getType().getBaseTypeIdentifier();

  // handle a special case shown as below:
  // x;
  // *y[];
  // (*z)[];
  // void foo(void) {...}
  // where x implicitly has type of int, whereas y has type of int *
  if (!Id) {
    SourceLocation EndLoc = VD->getLocation();
    const char *Buf = SrcManager->getCharacterData(EndLoc);
    int Offset = -1;
    SourceLocation NewEndLoc = EndLoc.getLocWithOffset(Offset);
    if (!NewEndLoc.isValid())
      return EndLoc;

    Buf--;
    while (isspace(*Buf) || (*Buf == '*') || (*Buf == '(')) {
      Offset--;
      NewEndLoc = EndLoc.getLocWithOffset(Offset);
      if (!NewEndLoc.isValid())
        return EndLoc.getLocWithOffset(Offset+1);

      Buf--;
    }
    return EndLoc.getLocWithOffset(Offset+1);
  }

  TypeLoc NextTL = VarTypeLoc.getNextTypeLoc();
  while (!NextTL.isNull()) {
    VarTypeLoc = NextTL;
    NextTL = NextTL.getNextTypeLoc();
  }

  SourceRange TypeLocRange = VarTypeLoc.getSourceRange();
  SourceLocation EndLoc = getEndLocationFromBegin(TypeLocRange);
  TransAssert(EndLoc.isValid() && "Invalid EndLoc!");

  const Type *Ty = VarTypeLoc.getTypePtr();

  // I am not sure why, but for a declaration like below:
  //   unsigned int a; (or long long a;)
  // TypeLoc.getBeginLoc() returns the position of 'u'
  // TypeLoc.getEndLoc() also returns the position of 'u'
  // The size of TypeLoc.getSourceRange() is 8, which is the 
  // length of "unsigned"
  // Then we are getting trouble, because now EndLoc is right 
  // after 'd', but we need it points to the location after "int".
  // skipPossibleTypeRange corrects the above deviation
  // Or am I doing something horrible here?
  EndLoc = skipPossibleTypeRange(Ty, EndLoc, VD->getLocation());
  return EndLoc;
}

bool RewriteUtils::removeVarFromDeclStmt(DeclStmt *DS,
                                         const VarDecl *VD,
                                         Decl *PrevDecl,
                                         bool IsFirstDecl,
                                         bool *StmtRemoved)
{
  SourceRange StmtRange = DS->getSourceRange();

  // VD is the the only declaration, so it is safe to remove the entire stmt
  if (DS->isSingleDecl()) {
    return !(TheRewriter->RemoveText(StmtRange));
  }

  // handle the case where we could have implicit declaration of RecordDecl
  // e.g., 
  // foo (void) {
  //   struct S0 *s;
  //   ...;
  // }
  // in this case, struct S0 is implicitly declared
  if (PrevDecl) {
    if ( RecordDecl *RD = dyn_cast<RecordDecl>(PrevDecl) ) {
      DeclGroup DGroup = DS->getDeclGroup().getDeclGroup();
      IsFirstDecl = true;
      if ((!RD->getDefinition() || RD->getNameAsString() == "") &&
          DGroup.size() == 2) {
        *StmtRemoved = true;
        return !(TheRewriter->RemoveText(StmtRange));
      }
    }
  }

  SourceRange VarRange = VD->getSourceRange();

  // VD is the first declaration in a declaration group.
  // We keep the leading type string
  if (IsFirstDecl) {
    // We need to get the outermost TypeLocEnd instead of the StartLoc of
    // a var name, because we need to handle the case below:
    //   int *x, *y;
    // If we rely on the StartLoc of a var name, then we will make bad
    // transformation like:
    //   int * *y;
    SourceLocation NewStartLoc = getVarDeclTypeLocEnd(VD);
    if (NewStartLoc.isMacroID()) {
      NewStartLoc = SrcManager->getSpellingLoc(NewStartLoc);
      const char *StartBuf = SrcManager->getCharacterData(NewStartLoc);
      // Make sure we have at least one space before the name.
      if (*StartBuf == ' ')
        NewStartLoc = NewStartLoc.getLocWithOffset(1);
    }

    SourceLocation NewEndLoc = getEndLocationUntil(VarRange, ',');
    if (NewEndLoc.isMacroID())
      NewEndLoc = SrcManager->getSpellingLoc(NewEndLoc);
    
    return 
      !(TheRewriter->RemoveText(SourceRange(NewStartLoc, NewEndLoc)));
  }

  TransAssert(PrevDecl && "PrevDecl cannot be NULL!");
  SourceLocation VarEndLoc = VarRange.getEnd();
  SourceRange PrevDeclRange = PrevDecl->getSourceRange();

  SourceLocation PrevDeclEndLoc = getEndLocationUntil(PrevDeclRange, ',');

  if (VarEndLoc.isMacroID())
    VarEndLoc = SrcManager->getSpellingLoc(VarEndLoc);
  if (PrevDeclEndLoc.isMacroID())
    PrevDeclEndLoc = SrcManager->getSpellingLoc(PrevDeclEndLoc);
  return !(TheRewriter->RemoveText(SourceRange(PrevDeclEndLoc, VarEndLoc)));
}

bool RewriteUtils::getExprString(const Expr *E, 
                                 std::string &ES)
{
  SourceRange ExprRange = E->getSourceRange();
  SourceLocation StartLoc = ExprRange.getBegin();
  if (StartLoc.isInvalid() && !StartLoc.isMacroID()) {
    ES = "<invalid-expr>";
    return false;
  }

  int RangeSize = TheRewriter->getRangeSize(ExprRange);
  if (RangeSize == -1) {
    if (StartLoc.isMacroID()) {
      StartLoc = SrcManager->getFileLoc(StartLoc);
      SourceLocation EndLoc = SrcManager->getFileLoc(ExprRange.getEnd());
      RangeSize = TheRewriter->getRangeSize(SourceRange(StartLoc, EndLoc));
    }
    else {
      ES = "<invalid-expr>";
      return false;
    }
  }

  const char *StartBuf = SrcManager->getCharacterData(StartLoc);

  ES.assign(StartBuf, RangeSize);

  const BinaryOperator *BinOp = dyn_cast<BinaryOperator>(E);

  // Keep the semantics of Comma operator
  if (BinOp && (BinOp->getOpcode() == BO_Comma))
    ES = "(" + ES + ")";

  return true;
}

bool RewriteUtils::getStmtString(const Stmt *S, 
                                 std::string &Str)
{
  SourceRange StmtRange = S->getSourceRange();
   
  int RangeSize = TheRewriter->getRangeSize(StmtRange);
  if (RangeSize == -1)
    return false;

  SourceLocation StartLoc = StmtRange.getBegin();
  const char *StartBuf = SrcManager->getCharacterData(StartLoc);

  Str.assign(StartBuf, RangeSize);

  return true;
}

SourceLocation RewriteUtils::getExpansionEndLoc(SourceLocation EndLoc)
{
    FileID FID = SrcManager->getFileID(EndLoc);
    const SrcMgr::SLocEntry *Entry = &SrcManager->getSLocEntry(FID);

    while (Entry->getExpansion().getExpansionLocStart().isMacroID()) {
      EndLoc = Entry->getExpansion().getExpansionLocStart();
      FID = SrcManager->getFileID(EndLoc);
      Entry = &SrcManager->getSLocEntry(FID);
    }

    return Entry->getExpansion().getExpansionLocEnd();
}

bool RewriteUtils::replaceExpr(const Expr *E, 
                               const std::string &ES)
{
  SourceRange ExprRange = E->getSourceRange();
   
  int RangeSize = TheRewriter->getRangeSize(ExprRange);
  if (RangeSize == -1) {
    SourceLocation StartLoc = ExprRange.getBegin();
    if (!SrcManager->isMacroBodyExpansion(StartLoc))
      return false;
    StartLoc = SrcManager->getFileLoc(StartLoc);
    SourceLocation EndLoc = ExprRange.getEnd();
    if (SrcManager->isMacroBodyExpansion(EndLoc)) {
      // FIXME: handle cases below:
      // #define macro bar(1,2);
      // int bar(int p1, int p2) { return p1 + p2; }
      // void foo(void) { int x = macro }
      EndLoc = getExpansionEndLoc(EndLoc);
    }
    return !(TheRewriter->ReplaceText(SourceRange(StartLoc, EndLoc), ES));
  }

  return !(TheRewriter->ReplaceText(ExprRange, ES));
}

bool RewriteUtils::replaceExprNotInclude(const Expr *E, 
                               const std::string &ES)
{
  SourceRange ExprRange = E->getSourceRange();
  SourceLocation StartLoc = ExprRange.getBegin();
  if (StartLoc.isMacroID()) {
    StartLoc = SrcManager->getFileLoc(StartLoc);
    SourceLocation EndLoc = ExprRange.getEnd();
    TransAssert(EndLoc.isMacroID() && "EndLoc is not from a macro!");
    ExprRange = SourceRange(StartLoc, SrcManager->getFileLoc(EndLoc));
  }
  TransAssert((TheRewriter->getRangeSize(ExprRange) != -1) && 
              "Bad expr range!");

  Rewriter::RewriteOptions Opts;
  // We don't want to include the previously inserted string
  Opts.IncludeInsertsAtBeginOfRange = false;

  TheRewriter->RemoveText(ExprRange, Opts);
  return !(TheRewriter->InsertText(StartLoc, ES));
}

std::string RewriteUtils::getStmtIndentString(Stmt *S,
                                              SourceManager *SrcManager)
{
  SourceLocation StmtStartLoc = S->getLocStart();

  if (StmtStartLoc.isMacroID()) {
    StmtStartLoc = SrcManager->getFileLoc(StmtStartLoc);
  }

  FileID FID;
  unsigned StartOffset = 
    getLocationOffsetAndFileID(StmtStartLoc, FID, SrcManager);

  StringRef MB = SrcManager->getBufferData(FID);
 
  unsigned lineNo = SrcManager->getLineNumber(FID, StartOffset) - 1;
  const SrcMgr::ContentCache *
      Content = SrcManager->getSLocEntry(FID).getFile().getContentCache();
  unsigned lineOffs = Content->SourceLineCache[lineNo];
 
  // Find the whitespace at the start of the line.
  StringRef indentSpace;

  unsigned I = lineOffs;
  while (isspace(MB[I]))
    ++I;
  indentSpace = MB.substr(lineOffs, I-lineOffs);

  return indentSpace;
}

bool RewriteUtils::addLocalVarToFunc(const std::string &VarStr,
                                     FunctionDecl *FD)
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
                                          bool NeedParen)
{
  std::string IndentStr = 
    RewriteUtils::getStmtIndentString(BeforeStmt, SrcManager);

  if (NeedParen) {
    SourceRange StmtRange = BeforeStmt->getSourceRange();
    SourceLocation LocEnd = 
      RewriteUtils::getEndLocationFromBegin(StmtRange);
    TransAssert(LocEnd.isValid() && "Invalid LocEnd!");

    std::string PostStr = "\n" + IndentStr + "}";
    if (TheRewriter->InsertTextAfterToken(LocEnd, PostStr))
      return false;
  }

  SourceLocation StmtLocStart = BeforeStmt->getLocStart();
  if (StmtLocStart.isMacroID()) {
    StmtLocStart = SrcManager->getFileLoc(StmtLocStart);
  }

  std::string ExprStr;
  RewriteUtils::getExprString(RHS, ExprStr);

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

void RewriteUtils::indentAfterNewLine(StringRef Str,
                                      std::string &NewStr,
                                      const std::string &IndentStr)
{
  SmallVector<StringRef, 20> StrVec;
  Str.split(StrVec, "\n"); 
  NewStr = "";
  for(SmallVector<StringRef, 20>::iterator I = StrVec.begin(), 
      E = StrVec.end(); I != E; ++I) {
    NewStr += ((*I).str() + "\n" + IndentStr);
  }
}

void RewriteUtils::addOpenParenBeforeStmt(Stmt *S, const std::string &IndentStr)
{
  SourceRange StmtRange = S->getSourceRange();
  SourceLocation LocEnd =
    RewriteUtils::getEndLocationFromBegin(StmtRange);
  TransAssert(LocEnd.isValid() && "Invalid LocEnd!");

  std::string PostStr = "\n" + IndentStr + "}";
  TheRewriter->InsertTextAfterToken(LocEnd, PostStr);
}

bool RewriteUtils::addStringBeforeStmtInternal(Stmt *S,
                                   const std::string &Str,
                                   const std::string &IndentStr,
                                   bool NeedParen)
{
  std::string NewStr;

  if (NeedParen) {
    NewStr = "{\n";
  }
  NewStr += Str;
  NewStr += "\n";
  
  std::string IndentedStr;
  indentAfterNewLine(NewStr, IndentedStr, IndentStr);

  return !(TheRewriter->InsertText(S->getLocStart(), 
           IndentedStr, /*InsertAfter=*/false));
}

bool RewriteUtils::addStringBeforeStmt(Stmt *BeforeStmt,
                                   const std::string &Str,
                                   bool NeedParen)
{
  std::string IndentStr =
    RewriteUtils::getStmtIndentString(BeforeStmt, SrcManager);

  if (NeedParen) {
    addOpenParenBeforeStmt(BeforeStmt, IndentStr);
  }
  return addStringBeforeStmtInternal(BeforeStmt, Str,
                                     IndentStr, NeedParen);
}

// Note that we can't use addStringBeforeStmt because
// we need to modify an expression in BeforeStmt. We have
// to do rewrite from end to begin to avoid crash.
bool RewriteUtils::addStringBeforeStmtAndReplaceExpr(Stmt *BeforeStmt,
                                   const std::string &StmtStr,
                                   const Expr *E, const std::string &ExprStr,
                                   bool NeedParen)
{
  std::string IndentStr =
    RewriteUtils::getStmtIndentString(BeforeStmt, SrcManager);

  if (NeedParen) {
    addOpenParenBeforeStmt(BeforeStmt, IndentStr);
  }
  replaceExpr(E, ExprStr);
  return addStringBeforeStmtInternal(BeforeStmt, StmtStr,
                                     IndentStr, NeedParen);
}

bool RewriteUtils::addStringAfterStmt(Stmt *AfterStmt, 
                                      const std::string &Str)
{
  std::string IndentStr = 
    RewriteUtils::getStmtIndentString(AfterStmt, SrcManager);

  std::string NewStr = "\n" + IndentStr + Str;
  SourceRange StmtRange = AfterStmt->getSourceRange();
  SourceLocation LocEnd = 
    RewriteUtils::getEndLocationFromBegin(StmtRange);
  TransAssert(LocEnd.isValid() && "Invalid LocEnd!");
  
  return !(TheRewriter->InsertText(LocEnd, NewStr));
}

bool RewriteUtils::addStringAfterVarDecl(const VarDecl *VD,
                                         const std::string &Str)
{
  SourceRange VarRange = VD->getSourceRange();
  SourceLocation LocEnd = getEndLocationAfter(VarRange, ';');
  
  return !(TheRewriter->InsertText(LocEnd, "\n" + Str));
}

bool RewriteUtils::addStringAfterFuncDecl(const FunctionDecl *FD,
                                          const std::string &Str)
{
  SourceRange FDRange = FD->getSourceRange();
  SourceLocation LocEnd = getEndLocationAfter(FDRange, ';');
  
  return !(TheRewriter->InsertText(LocEnd, "\n" + Str));
}

// This function is an experimental one. It doesn't work
// if ND is a class of FunctionDecl, but I am not sure
// how it works for other types of NamedDecls
bool RewriteUtils::replaceNamedDeclName(const NamedDecl *ND,
                                        const std::string &NameStr)
{
  TransAssert(!isa<FunctionDecl>(ND) && 
    "Please use replaceFunctionDeclName for renaming a FunctionDecl!");
  TransAssert(!isa<UsingDirectiveDecl>(ND) && 
    "Cannot use this function for renaming UsingDirectiveDecl");
  SourceLocation NameLocStart = ND->getLocation();
  return !(TheRewriter->ReplaceText(NameLocStart, 
             ND->getNameAsString().size(), NameStr));
}

bool RewriteUtils::replaceValueDecl(const ValueDecl *VD, const std::string &Str)
{
  SourceRange Range = VD->getSourceRange();
  unsigned RangeSize = TheRewriter->getRangeSize(Range);
  return !(TheRewriter->ReplaceText(Range.getBegin(), RangeSize, Str));
}

bool RewriteUtils::replaceVarDeclName(VarDecl *VD,
                                      const std::string &NameStr)
{
  SourceLocation NameLocStart = VD->getLocation();
  return !(TheRewriter->ReplaceText(NameLocStart, 
             VD->getNameAsString().size(), NameStr));
}

bool RewriteUtils::replaceFunctionDeclName(const FunctionDecl *FD,
                                      const std::string &NameStr)
{
  // We cannot naively use FD->getNameAsString() here. 
  // For example, for a template class
  // template<typename T>
  // class SomeClass {
  // public:
  //   SomeClass() {}
  // };
  // applying getNameAsString() on SomeClass() gives us SomeClass<T>.

  DeclarationNameInfo NameInfo = FD->getNameInfo();
  DeclarationName DeclName = NameInfo.getName();
  DeclarationName::NameKind K = DeclName.getNameKind();
  TransAssert((K != DeclarationName::CXXDestructorName) &&
              "Cannot rename CXXDestructorName here!");

  std::string FDName = FD->getNameAsString();
  size_t FDNameLen = FD->getNameAsString().length();
  if (K == DeclarationName::CXXConstructorName) {
    const Type *Ty = DeclName.getCXXNameType().getTypePtr();
    if (Ty->getTypeClass() == Type::InjectedClassName) {
      const CXXRecordDecl *CXXRD = Ty->getAsCXXRecordDecl();
      std::string RDName = CXXRD->getNameAsString();
      FDNameLen = FDName.find(RDName);
      TransAssert((FDNameLen != std::string::npos) && 
                  "Cannot find RecordDecl Name!");
      FDNameLen += RDName.length();
    }
  }

  return !TheRewriter->ReplaceText(NameInfo.getLoc(),
                                   FDNameLen,
                                   NameStr);
}

bool RewriteUtils::replaceCXXDestructorDeclName(
       const CXXDestructorDecl *DtorDecl,
       const std::string &Name)
{
  SourceLocation StartLoc = DtorDecl->getLocation();
  const char *StartBuf = SrcManager->getCharacterData(StartLoc);
  TransAssert((*StartBuf == '~') && "Invalid Destructor Location");
  // FIXME: it's quite ugly, better to use clang's Lexer
  unsigned Off = 0;
  StartBuf++;
  while (isspace(*StartBuf)) {
    StartBuf++;
    Off++;
  }

  std::string DName = DtorDecl->getNameAsString();
  DeclarationNameInfo NameInfo = DtorDecl->getNameInfo();
  DeclarationName DeclName = NameInfo.getName();
  const Type *Ty = DeclName.getCXXNameType().getTypePtr();
  size_t NameLen;
  if (Ty->getTypeClass() == Type::InjectedClassName) {
    const CXXRecordDecl *CXXRD = Ty->getAsCXXRecordDecl();
    std::string RDName = CXXRD->getNameAsString();
    NameLen = DName.find(RDName);
    TransAssert((NameLen != std::string::npos) && 
                "Cannot find RecordDecl Name!");
    NameLen += RDName.length();
  }
  else {
    NameLen = DName.length();
  }
  NameLen += Off;
 
  return !TheRewriter->ReplaceText(StartLoc,
                                   NameLen,
                                   "~" + Name);
}

bool RewriteUtils::replaceRecordDeclName(const RecordDecl *RD,
                                         const std::string &NameStr)
{
  SourceLocation LocStart = RD->getLocation();
  return !TheRewriter->ReplaceText(LocStart,
                                   RD->getNameAsString().length(),
                                   NameStr);
}

bool RewriteUtils::replaceVarTypeName(const VarDecl *VD,
                                      const std::string &NameStr)
{
  const IdentifierInfo *TypeId = VD->getType().getBaseTypeIdentifier();

  SourceLocation LocStart = getVarDeclTypeLocBegin(VD);
  return !TheRewriter->ReplaceText(LocStart,
                                   TypeId->getLength(),
                                   NameStr);
}

void RewriteUtils::getStringBetweenLocs(std::string &Str, 
                                        SourceLocation LocStart,
                                        SourceLocation LocEnd)
{
  const char *StartBuf = SrcManager->getCharacterData(LocStart);
  const char *EndBuf = SrcManager->getCharacterData(LocEnd);
  TransAssert(StartBuf < EndBuf);
  size_t Off = EndBuf - StartBuf;
  Str.assign(StartBuf, Off);
}

void RewriteUtils::getStringBetweenLocsAfterStart(std::string &Str, 
                                                  SourceLocation LocStart,
                                                  SourceLocation LocEnd)
{
  const char *StartBuf = SrcManager->getCharacterData(LocStart);
  const char *EndBuf = SrcManager->getCharacterData(LocEnd);
  StartBuf++;
  TransAssert(StartBuf <= EndBuf);
  size_t Off = EndBuf - StartBuf;
  Str.assign(StartBuf, Off);
}

bool RewriteUtils::getEntireDeclGroupStrAndRemove(DeclGroupRef DGR,
                                                  std::string &Str)
{
  Decl *FirstD, *LastD;
  if (DGR.isSingleDecl()) {
    FirstD = DGR.getSingleDecl();
    LastD = FirstD;
  }
  else {
    DeclGroupRef::iterator I = DGR.begin();
    FirstD = (*I);

    DeclGroupRef::iterator E = DGR.end();
    --E;
    LastD = (*E);
  }

  SourceRange FirstRange = FirstD->getSourceRange();
  SourceLocation StartLoc = FirstRange.getBegin();
  SourceRange LastRange = LastD->getSourceRange();
  SourceLocation EndLoc = getEndLocationUntil(LastRange, ';');

  // This isn't really good, but if EndLoc is invalid, what can we do? 
  if (EndLoc.isInvalid()) {
    unsigned Off = 0;
    const char *StartBuf = SrcManager->getCharacterData(StartLoc);
    while ((*StartBuf != '\n') && (*StartBuf != ';') && (*StartBuf != '\0')) {
      Off++;
      StartBuf++;
    }
    assert(Off && "Zero offset!");
    EndLoc = StartLoc.getLocWithOffset(Off);
  }
  getStringBetweenLocs(Str, StartLoc, EndLoc);
  return !TheRewriter->RemoveText(SourceRange(StartLoc, EndLoc));
}

// This function skips type specifiers
bool RewriteUtils::getDeclGroupStrAndRemove(DeclGroupRef DGR, 
                                   std::string &Str)
{
  if (DGR.isSingleDecl()) {
    Decl *D = DGR.getSingleDecl();
    VarDecl *VD = dyn_cast<VarDecl>(D);
    TransAssert(VD && "Bad VarDecl!");

    // We need to get the outermost TypeLocEnd instead of the StartLoc of
    // a var name, because we need to handle the case below:
    //   int *x;
    //   int *y;
    // If we rely on the StartLoc of a var name, then we will make bad
    // transformation like:
    //   int *x, y;
    SourceLocation TypeLocEnd = getVarDeclTypeLocEnd(VD);
    if (TypeLocEnd.isMacroID())
      TypeLocEnd = SrcManager->getFileLoc(TypeLocEnd);
    SourceRange VarRange = VD->getSourceRange();

    SourceLocation LocEnd = getEndLocationUntil(VarRange, ';');

    getStringBetweenLocs(Str, TypeLocEnd, LocEnd);

    SourceLocation StartLoc = VarRange.getBegin();
    if (StartLoc.isMacroID())
      StartLoc = SrcManager->getFileLoc(StartLoc);
    SourceLocation NewEndLoc = getLocationAfterSkiping(LocEnd, ';');
    return !(TheRewriter->RemoveText(SourceRange(StartLoc, NewEndLoc)));
  }

  TransAssert(DGR.getDeclGroup().size() > 1);

  DeclGroupRef::iterator I = DGR.begin();
  DeclGroupRef::iterator E = DGR.end();
  --E;

  Decl *FirstD = (*I);
  VarDecl *FirstVD = dyn_cast<VarDecl>(FirstD);
  Decl *LastD = (*E);
  VarDecl *LastVD = dyn_cast<VarDecl>(LastD);

  TransAssert(FirstVD && "Bad First VarDecl!");
  TransAssert(LastVD && "Bad First VarDecl!");

  SourceLocation TypeLocEnd = getVarDeclTypeLocEnd(FirstVD);
  SourceRange LastVarRange = LastVD->getSourceRange();
  SourceLocation LastEndLoc = getEndLocationUntil(LastVarRange, ';');
  getStringBetweenLocs(Str, TypeLocEnd, LastEndLoc);

  SourceLocation StartLoc = FirstVD->getLocStart();
  SourceLocation NewLastEndLoc = getLocationAfterSkiping(LastEndLoc, ';');
  return !(TheRewriter->RemoveText(SourceRange(StartLoc, NewLastEndLoc)));
}

SourceLocation RewriteUtils::getDeclGroupRefEndLoc(DeclGroupRef DGR)
{
  Decl *LastD;

  if (DGR.isSingleDecl()) {
    LastD = DGR.getSingleDecl();
  }
  else {
    DeclGroupRef::iterator E = DGR.end();
    --E;
    LastD = (*E);
  }

#if 0
  VarDecl *VD = dyn_cast<VarDecl>(LastD);
  TransAssert(VD && "Bad VD!");
  SourceRange VarRange = VD->getSourceRange();
  return getEndLocationFromBegin(VarRange);
#endif

  // The LastD could be a RecordDecl
  SourceRange Range = LastD->getSourceRange();
  SourceLocation EndLoc = getEndLocationFromBegin(Range);
  TransAssert(EndLoc.isValid() && "Invalid EndLoc!");
  return EndLoc;
}

SourceLocation RewriteUtils::getDeclStmtEndLoc(DeclStmt *DS)
{
  DeclGroupRef DGR = DS->getDeclGroup();
  return getDeclGroupRefEndLoc(DGR);
}

bool RewriteUtils::getDeclStmtStrAndRemove(DeclStmt *DS, 
                                   std::string &Str)
{
  DeclGroupRef DGR = DS->getDeclGroup();
  return getDeclGroupStrAndRemove(DGR, Str);
}

bool RewriteUtils::removeAStarBefore(const Decl *D)
{
  SourceLocation LocStart = D->getLocation();
  const char *StartBuf = SrcManager->getCharacterData(LocStart);
  int Offset = 0;
  while (*StartBuf != '*') {
    StartBuf--;
    Offset--;
  }
  SourceLocation StarLoc =  LocStart.getLocWithOffset(Offset);
  return !TheRewriter->RemoveText(StarLoc, 1);
}

bool RewriteUtils::removeASymbolAfter(const Expr *E,
                                    char Symbol)
{
  SourceRange ExprRange = E->getSourceRange();
  SourceLocation LocStart = ExprRange.getBegin();
  const char *StartBuf = SrcManager->getCharacterData(LocStart);
  int Offset = 0;
  while (*StartBuf != Symbol) {
    StartBuf++;
    Offset++;
  }
  SourceLocation StarLoc =  LocStart.getLocWithOffset(Offset);
  return !TheRewriter->RemoveText(StarLoc, 1);
}

bool RewriteUtils::removeAStarAfter(const Expr *E)
{
  return removeASymbolAfter(E, '*');
}

bool RewriteUtils::removeAnAddrOfAfter(const Expr *E)
{
  return removeASymbolAfter(E, '&');
}

bool RewriteUtils::insertAnAddrOfBefore(const Expr *E)
{
  SourceRange ExprRange = E->getSourceRange();
  SourceLocation LocStart = ExprRange.getBegin();
  return !TheRewriter->InsertTextBefore(LocStart, "&");
}

bool RewriteUtils::insertAStarBefore(const Expr *E)
{
  SourceRange ExprRange = E->getSourceRange();
  SourceLocation LocStart = ExprRange.getBegin();
  return !TheRewriter->InsertTextBefore(LocStart, "*");
}

bool RewriteUtils::removeVarInitExpr(const VarDecl *VD)
{
  TransAssert(VD->hasInit() && "VarDecl doesn't have an Init Expr!");
  SourceLocation NameStartLoc = VD->getLocation();

  SourceLocation InitStartLoc = getLocationUntil(NameStartLoc, '=');

  const Expr *Init = VD->getInit();
  SourceRange ExprRange = Init->getSourceRange();
  SourceLocation InitEndLoc = ExprRange.getEnd();
  // handle macro, e.g.:
  // #define NULL 0
  // void foo(void)
  // {
    // int *p = NULL;
  // }
  if (SrcManager->isMacroBodyExpansion(InitEndLoc)) {
    InitEndLoc = SrcManager->getFileLoc(InitEndLoc);
  }
  return !TheRewriter->RemoveText(SourceRange(InitStartLoc, InitEndLoc));
}

SourceLocation RewriteUtils::getMacroExpansionLoc(SourceLocation Loc) {
  if (SrcManager->isMacroBodyExpansion(Loc))
    return SrcManager->getFileLoc(Loc);
  return Loc;
}

bool RewriteUtils::removeVarDecl(const VarDecl *VD,
                                 DeclGroupRef DGR)
{
  SourceRange VarRange = VD->getSourceRange();

  if (DGR.isSingleDecl()) {
    SourceLocation StartLoc = getMacroExpansionLoc(VarRange.getBegin());
    SourceLocation EndLoc = getEndLocationUntil(VarRange, ';');
    // in case the previous EndLoc is invalid, for example,
    // VarRange could have bad EndLoc value
    if (EndLoc.isInvalid())
      EndLoc = getLocationUntil(StartLoc, ';');
    return !(TheRewriter->RemoveText(SourceRange(StartLoc, EndLoc)));
  }

  DeclGroupRef::const_iterator I = DGR.begin();
  const VarDecl *FirstVD = dyn_cast<VarDecl>(*I);
  // dyn_cast (*I) to VarDecl could fail, because it could be a struct decl,
  // e.g., struct S1 { int f1; } s2 = {1}, where FirstDecl is
  // struct S1 {int f1;}. We need to skip it
  if (!FirstVD) {
    // handle the case where we could have implicit declaration of RecordDecl
    // e.g., 
    //   struct S0 *s;
    //   ...;
    // in this case, struct S0 is implicitly declared
    if ( RecordDecl *RD = dyn_cast<RecordDecl>(*I) ) {
      if (!RD->getDefinition() && DGR.getDeclGroup().size() == 2) {
        SourceLocation StartLoc = VarRange.getBegin();
        SourceLocation EndLoc = getEndLocationUntil(VarRange, ';');
        return !(TheRewriter->RemoveText(SourceRange(StartLoc, EndLoc)));
      }
    }

    ++I;
    TransAssert((I != DGR.end()) && "Bad Decl!");
    FirstVD = dyn_cast<VarDecl>(*I);
    TransAssert(FirstVD && "Invalid Var Decl!");
    if (VD == FirstVD) {
      SourceLocation StartLoc = VD->getLocation();
      SourceLocation EndLoc = 
        getEndLocationUntil(VarRange, ',');

      return !(TheRewriter->RemoveText(SourceRange(StartLoc, EndLoc)));
    }
  }
  else if (VD == FirstVD) {
    SourceLocation StartLoc = getVarDeclTypeLocEnd(VD);

    SourceLocation EndLoc = 
      getEndLocationUntil(VarRange, ',');

    return !(TheRewriter->RemoveText(SourceRange(StartLoc, EndLoc)));
  }

  const Decl *PrevDecl = FirstVD;
  const VarDecl *CurrVD = NULL;
  ++I;
  DeclGroupRef::const_iterator E = DGR.end();
  for (; I != E; ++I) {
    CurrVD = dyn_cast<VarDecl>(*I);
    if (CurrVD && VD == CurrVD)
      break;
    PrevDecl = *I;
  }

  TransAssert((VD == CurrVD) && "Cannot find VD!");

  SourceLocation VarEndLoc = VarRange.getEnd();
  SourceRange PrevDeclRange = PrevDecl->getSourceRange();

  SourceLocation PrevDeclEndLoc = 
    getEndLocationUntil(PrevDeclRange, ',');

  return !(TheRewriter->RemoveText(SourceRange(PrevDeclEndLoc, VarEndLoc)));
}

void RewriteUtils::getTmpTransName(unsigned Postfix, std::string &Name)
{
  std::stringstream SS;
  SS << getTmpVarNamePrefix() << Postfix;
  Name = SS.str();
}

bool RewriteUtils::insertStringBeforeFunc(const FunctionDecl *FD,
                                          const std::string &Str)
{
  SourceRange FuncRange;
  if (FunctionTemplateDecl *TmplD = FD->getDescribedFunctionTemplate()) {
    FuncRange = TmplD->getSourceRange();
  }
  else {
    FuncRange = FD->getSourceRange();
  }
  SourceLocation StartLoc = FuncRange.getBegin();
  return !TheRewriter->InsertTextBefore(StartLoc, Str);
}

bool RewriteUtils::insertStringBeforeTemplateDecl(const TemplateDecl *D,
                                                  const std::string &Str)
{
  SourceRange Range = D->getSourceRange();
  SourceLocation StartLoc = Range.getBegin();
  TransAssert(StartLoc.isValid() && "Invalid template decl StartLoc!");
  return !TheRewriter->InsertTextBefore(StartLoc, Str);
}

bool RewriteUtils::replaceUnionWithStruct(const NamedDecl *ND)
{
  SourceRange NDRange = ND->getSourceRange();
  int RangeSize = TheRewriter->getRangeSize(NDRange);
  TransAssert((RangeSize != -1) && "Bad Range!");

  SourceLocation StartLoc = NDRange.getBegin();
  const char *StartBuf = SrcManager->getCharacterData(StartLoc);
  std::string TmpStr(StartBuf, RangeSize);
  std::string UStr = "union";
  size_t Pos = TmpStr.find(UStr);
  if (Pos == std::string::npos)
    return true;

  if (Pos != 0)
    StartLoc = StartLoc.getLocWithOffset(Pos);
  return !TheRewriter->ReplaceText(StartLoc, UStr.size(), "struct");
}

bool RewriteUtils::removeIfAndCond(const IfStmt *IS)
{
  SourceLocation IfLoc = IS->getIfLoc();
  const Stmt *ThenStmt = IS->getThen();
  TransAssert(ThenStmt && "NULL ThenStmt!");

  SourceLocation ThenLoc = ThenStmt->getLocStart();
  SourceLocation EndLoc =  ThenLoc.getLocWithOffset(-1);

  Rewriter::RewriteOptions Opts;
  // We don't want to include the previously inserted string
  Opts.IncludeInsertsAtBeginOfRange = false;
  return !TheRewriter->RemoveText(SourceRange(IfLoc, EndLoc), Opts);
}

bool RewriteUtils::removeArraySubscriptExpr(const Expr *E)
{
  SourceRange ERange = E->getSourceRange();
  SourceLocation StartLoc = ERange.getBegin();
  const char *StartBuf = SrcManager->getCharacterData(StartLoc);
  int Offset = 0;
  while (*StartBuf != '[') {
    StartBuf--;
    Offset--;
  }
  StartLoc = StartLoc.getLocWithOffset(Offset);

  SourceLocation EndLoc = ERange.getEnd();
  EndLoc = EndLoc.getLocWithOffset(1);
  if (EndLoc.isInvalid())
    return !TheRewriter->RemoveText(SourceRange(StartLoc, ERange.getEnd()));

  SourceLocation RBLoc = getLocationUntil(EndLoc, ']');
  if (RBLoc.isInvalid())
    return !TheRewriter->RemoveText(SourceRange(StartLoc, EndLoc));
  return !TheRewriter->RemoveText(SourceRange(StartLoc, RBLoc));
}

bool RewriteUtils::getFunctionDefStrAndRemove(const FunctionDecl *FD,
                                              std::string &Str)
{
  SourceRange FDRange = FD->getSourceRange();

  int RangeSize = TheRewriter->getRangeSize(FDRange);
  if (RangeSize == -1)
    return false;

  SourceLocation StartLoc = FDRange.getBegin();
  const char *StartBuf = SrcManager->getCharacterData(StartLoc);

  Str.assign(StartBuf, RangeSize);
  TheRewriter->RemoveText(FDRange);
  return true;
}

bool RewriteUtils::getFunctionDeclStrAndRemove(const FunctionDecl *FD,
                                               std::string &Str)
{
  TransAssert(!FD->isThisDeclarationADefinition() && 
              "FD cannot be a definition!");

  SourceRange FDRange = FD->getSourceRange();
  SourceLocation StartLoc = FDRange.getBegin();
  SourceLocation EndLoc = getEndLocationUntil(FDRange, ';');

  getStringBetweenLocs(Str, StartLoc, EndLoc);
  return !TheRewriter->RemoveText(SourceRange(StartLoc, EndLoc));
}

bool RewriteUtils::replaceFunctionDefWithStr(const FunctionDecl *FD,
                                             const std::string &Str)
{
  const Stmt *Body = FD->getBody();
  TransAssert(Body && "FunctionDecl is not a definition!");
  return !TheRewriter->ReplaceText(Body->getSourceRange(), Str);
}

// FIXME: probably we don't need this function, because we could use 
//        removeDecl insteadly
bool RewriteUtils::removeFieldDecl(const FieldDecl *FD)
{
  SourceRange Range = FD->getSourceRange();
  SourceLocation StartLoc = Range.getBegin();
  SourceLocation EndLoc = getEndLocationUntil(Range, ';');
  SourceLocation CurlyEndLoc = getEndLocationUntil(Range, '}');

  // handle cases like:
  // struct {
  //   int f <- no semicolon here
  // };
  const char *SemiPos = SrcManager->getCharacterData(EndLoc);
  const char *CurlyPos = SrcManager->getCharacterData(CurlyEndLoc);
  if (SemiPos > CurlyPos) {
    EndLoc = CurlyEndLoc.getLocWithOffset(-1);
  }

  // If EndLoc is invalid, just remove one char to avoid crash
  if (EndLoc.isInvalid()) {
    EndLoc = StartLoc;
  }
  return !(TheRewriter->RemoveText(SourceRange(StartLoc, EndLoc)));
}

bool RewriteUtils::removeDecl(const Decl *D)
{
  SourceRange Range = D->getSourceRange();
  TransAssert((TheRewriter->getRangeSize(Range) != -1) && 
              "Bad UsingDecl SourceRange!");
  SourceLocation StartLoc = Range.getBegin();
  SourceLocation EndLoc = getEndLocationUntil(Range, ';');
  return !(TheRewriter->RemoveText(SourceRange(StartLoc, EndLoc)));
}

bool RewriteUtils::replaceCXXDtorCallExpr(const CXXMemberCallExpr *CE,
                                          std::string &Name)
{
  const CXXMethodDecl *MD = CE->getMethodDecl();
  const CXXDestructorDecl *DtorDecl = dyn_cast<CXXDestructorDecl>(MD);
  if (!DtorDecl)
    return true;

  Name = "~" + Name;

  std::string ExprStr;
  getExprString(CE, ExprStr);
  std::string OldDtorName = DtorDecl->getNameAsString();
  size_t Pos = ExprStr.find(OldDtorName);
  TransAssert((Pos != std::string::npos) && "Bad Name Position!");
  if (Pos == 0)
    return true;

  SourceLocation StartLoc = CE->getLocStart();
  StartLoc = StartLoc.getLocWithOffset(Pos);

  return !(TheRewriter->ReplaceText(StartLoc, OldDtorName.size(), Name));
}

SourceRange RewriteUtils::getFileLocSourceRange(SourceRange LocRange)
{
  SourceLocation StartLoc = LocRange.getBegin();
  if (StartLoc.isMacroID()) {
    StartLoc = SrcManager->getSpellingLoc(StartLoc);
    SourceLocation EndLoc = LocRange.getEnd();
    TransAssert(EndLoc.isMacroID() && "EndLoc is not from a macro!");
    LocRange = SourceRange(StartLoc, SrcManager->getSpellingLoc(EndLoc));
  }
  return LocRange;
}

bool RewriteUtils::removeSpecifier(NestedNameSpecifierLoc Loc)
{
  SourceRange LocRange = getFileLocSourceRange(Loc.getLocalSourceRange());
  TransAssert((TheRewriter->getRangeSize(LocRange) != -1) && 
              "Bad NestedNameSpecifierLoc Range!");
  return !(TheRewriter->RemoveText(LocRange));
}

bool RewriteUtils::replaceSpecifier(NestedNameSpecifierLoc Loc,
                                    const std::string &Name)
{
  SourceRange LocRange = getFileLocSourceRange(Loc.getLocalSourceRange());
  TransAssert((TheRewriter->getRangeSize(LocRange) != -1) && 
              "Bad NestedNameSpecifierLoc Range!");
  return !(TheRewriter->ReplaceText(LocRange, Name + "::"));
}

void RewriteUtils::getQualifierAsString(NestedNameSpecifierLoc Loc,
                                        std::string &Str)
{
  SourceLocation StartLoc = Loc.getBeginLoc();
  TransAssert(StartLoc.isValid() && "Bad StartLoc for NestedNameSpecifier!");
  SourceRange Range = Loc.getSourceRange();
  int Len = TheRewriter->getRangeSize(Range);
  const char *StartBuf = SrcManager->getCharacterData(StartLoc);
  Str.assign(StartBuf, Len);
}

void RewriteUtils::getSpecifierAsString(NestedNameSpecifierLoc Loc,
                                        std::string &Str)
{
  SourceLocation StartLoc = Loc.getBeginLoc();
  TransAssert(StartLoc.isValid() && "Bad StartLoc for NestedNameSpecifier!");
  const char *StartBuf = SrcManager->getCharacterData(StartLoc);
  const char *OrigBuf = StartBuf;
  unsigned int Len = 0;
  while (!isspace(*StartBuf) && (*StartBuf != ':')) {
    StartBuf++;
    Len++;
  }
  
  Str.assign(OrigBuf, Len);
}

bool RewriteUtils::replaceRecordType(RecordTypeLoc &RTLoc,
                                     const std::string &Name)
{
  const IdentifierInfo *TypeId = RTLoc.getType().getBaseTypeIdentifier();
  if (!TypeId)
    return true;

  SourceLocation LocStart = RTLoc.getLocStart();

  // Loc could be invalid, for example:
  // class AAA { };
  // class BBB:AAA {
  // public:
  //   BBB () { }
  // };
  // In Clang's internal representation, BBB's Ctor is BBB() : AAA() {}
  // The implicit AAA() will be visited here 
  // This is the only case where RTLoc is invalid, so the question is -
  // Is the guard below too strong? It is possible it could mask other 
  // potential bugs?
  if (LocStart.isInvalid())
    return true;

  return !(TheRewriter->ReplaceText(LocStart, TypeId->getLength(), Name));
}

bool RewriteUtils::isTheFirstDecl(const VarDecl *VD)
{
  SourceRange Range = VD->getSourceRange();
  SourceLocation StartLoc = Range.getBegin();
  SourceLocation NameStartLoc = VD->getLocation();
  
  const char *StartBuf = SrcManager->getCharacterData(StartLoc);
  const char *NameStartBuf = SrcManager->getCharacterData(NameStartLoc);
  
  while (StartBuf != NameStartBuf) {
    if (*StartBuf == ',')
      return false;
    StartBuf++;
  }
  return true;
}

bool RewriteUtils::isSingleDecl(const VarDecl *VD)
{
  if (!isTheFirstDecl(VD))
    return false;

  SourceRange Range = VD->getSourceRange();
  SourceLocation StartLoc = Range.getBegin();
  int RangeSize = TheRewriter->getRangeSize(Range);
  SourceLocation EndLoc = StartLoc.getLocWithOffset(RangeSize);
  const char *EndBuf = SrcManager->getCharacterData(EndLoc);
  while (isspace(*EndBuf))
    EndBuf++;

  return (*EndBuf == ';');
}

// In case we don't know if VD is in a single decl group,
// also we don't know if VD is the first decl or not.
// once this version is well-tested, probably we should remove 
// bool RewriteUtils::removeVarDecl(const VarDecl *VD,
//                                  DeclGroupRef DGR)
bool RewriteUtils::removeVarDecl(const VarDecl *VD)
{
  if (isSingleDecl(VD)) {
    return removeDecl(VD);
  }

  SourceRange VarRange = VD->getSourceRange();

  // VD is the first declaration in a declaration group.
  // We keep the leading type string
  if (isTheFirstDecl(VD)) {
    // We need to get the outermost TypeLocEnd instead of the StartLoc of
    // a var name, because we need to handle the case below:
    //   int *x, *y;
    // If we rely on the StartLoc of a var name, then we will make bad
    // transformation like:
    //   int * *y;
    SourceLocation NewStartLoc = getVarDeclTypeLocEnd(VD);

    SourceLocation NewEndLoc = getEndLocationUntil(VarRange, ',');
    
    return 
      !(TheRewriter->RemoveText(SourceRange(NewStartLoc, NewEndLoc)));
  }

  SourceLocation NameLoc = VD->getLocation();
  SourceLocation VarStartLoc = VarRange.getBegin();
  const char *NameStartBuf = SrcManager->getCharacterData(NameLoc);
  const char *VarStartBuf = SrcManager->getCharacterData(VarStartLoc);
  int Offset = 0;
  TransAssert((VarStartBuf < NameStartBuf) && "Bad Name Location!");
  while (NameStartBuf != VarStartBuf) {
    if (*NameStartBuf == ',')
      break;
    Offset--;
    NameStartBuf--;
  }
  TransAssert((VarStartBuf < NameStartBuf) && "Cannot find comma!");
  SourceLocation PrevDeclEndLoc = NameLoc.getLocWithOffset(Offset);
  SourceLocation VarEndLoc = VarRange.getEnd();

  return !(TheRewriter->RemoveText(SourceRange(PrevDeclEndLoc, VarEndLoc)));
}

bool RewriteUtils::removeTextFromLeftAt(SourceRange Range, char C, 
                                        SourceLocation EndLoc)
{
  SourceLocation StartLoc = Range.getBegin();
  const char *StartBuf = SrcManager->getCharacterData(StartLoc);
  int Offset = 0;
  while (*StartBuf != C) {
    StartBuf--;
    Offset--;
  }
  StartLoc = StartLoc.getLocWithOffset(Offset);
  return !TheRewriter->RemoveText(SourceRange(StartLoc, EndLoc));
}

SourceLocation RewriteUtils::getLocationFromLeftUntil(SourceLocation StartLoc,
                                                      char C)
{
  const char *StartBuf = SrcManager->getCharacterData(StartLoc);
  int Offset = 0;
  while (*StartBuf != C) {
    StartBuf--;
    Offset--;
  }
  return StartLoc.getLocWithOffset(Offset);
}

bool RewriteUtils::removeTextUntil(SourceRange Range, char C)
{
  SourceLocation StartLoc = Range.getBegin();

  // I don't know the reason, but seems Clang treats the following two
  // cases differently:
  // (1) template<bool, typename>
  //    in this case, the RangeSize is 5, which includes the ','
  // (2) template<typename, typename>
  //    in this case, the RangeSize is 8, which excludes the comma
  SourceLocation EndLoc = Range.getEnd();
  const char *EndBuf = SrcManager->getCharacterData(EndLoc);
  if (*EndBuf != C)
    EndLoc = getEndLocationUntil(Range, C);
  return !TheRewriter->RemoveText(SourceRange(StartLoc, EndLoc));
}

bool RewriteUtils::removeCXXCtorInitializer(const CXXCtorInitializer *Init,
                                            unsigned Index, unsigned NumInits)
{
  SourceRange Range = Init->getSourceRange();
  SourceLocation EndLoc = Init->getRParenLoc();
  if (Index == 0) {
    if (NumInits == 1)
      return removeTextFromLeftAt(Range, ':', EndLoc);
    else
      return removeTextUntil(Range, ',');
  }
  else {
    return removeTextFromLeftAt(Range, ',', EndLoc);
  }
}

bool RewriteUtils::removeClassDecls(const CXXRecordDecl *CXXRD)
{
  for (CXXRecordDecl::redecl_iterator I = CXXRD->redecls_begin(),
      E = CXXRD->redecls_end(); I != E; ++I) {
    SourceRange Range = (*I)->getSourceRange();
    SourceLocation LocEnd;
    if ((*I)->isThisDeclarationADefinition()) {
      LocEnd = (*I)->getBraceRange().getEnd();
      if (LocEnd.isValid())
        LocEnd = getLocationUntil(LocEnd, ';');
      else
        LocEnd = getEndLocationUntil(Range, ';');
    }
    else {
      LocEnd = getEndLocationUntil(Range, ';');
    }
    TheRewriter->RemoveText(SourceRange(Range.getBegin(), LocEnd));
  }
  return true;
}

bool RewriteUtils::removeClassTemplateDecls(const ClassTemplateDecl *TmplD)
{
  for (ClassTemplateDecl::redecl_iterator I = TmplD->redecls_begin(),
      E = TmplD->redecls_end(); I != E; ++I) {
    const CXXRecordDecl *CXXRD = 
      dyn_cast<CXXRecordDecl>((*I)->getTemplatedDecl());
    TransAssert(CXXRD && "Invalid class template!");

    SourceRange Range = (*I)->getSourceRange();
    SourceLocation LocEnd;
    if (CXXRD->isThisDeclarationADefinition()) {
      LocEnd = CXXRD->getBraceRange().getEnd();
      LocEnd = getLocationUntil(LocEnd, ';');
    }
    else {
      LocEnd = getEndLocationUntil(Range, ';');
    }
    TheRewriter->RemoveText(SourceRange(Range.getBegin(), LocEnd));
  }
  return true;
}

bool RewriteUtils::replaceCXXMethodNameAfterQualifier(
       const NestedNameSpecifierLoc *QualLoc,
       const CXXMethodDecl *MD,
       const std::string &NewName)
{
  SourceLocation EndLoc = QualLoc->getEndLoc();
  const char *EndBuf = SrcManager->getCharacterData(EndLoc);
  unsigned int Offset = 0;
  while (isspace(*EndBuf) || (*EndBuf == ':')) {
    EndBuf++;
    Offset++;
  }
  EndLoc = EndLoc.getLocWithOffset(Offset);
  TheRewriter->ReplaceText(EndLoc, MD->getNameAsString().size(), NewName);
  return true;
}

