//===----------------------------------------------------------------------===//
// 
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef REWRITE_UTILS_H
#define REWRITE_UTILS_H

#include <string>
#include "clang/Basic/SourceLocation.h"

#ifndef ENABLE_TRANS_ASSERT
  #define TransAssert(x) {if (!(x)) exit(0);}
#else
  #define TransAssert(x) assert(x)
#endif

namespace clang {
  class ParmVarDecl;
  class VarDecl;
  class Decl;
  class DeclStmt;
  class Rewriter;
  class SourceManager;
  class CallExpr;
  class Expr;
  class FunctionDecl;
  class Stmt;
  class DeclGroupRef;
  class DeclRefExpr;
}

class RewriteUtils {
public:
  static RewriteUtils *GetInstance(clang::Rewriter *RW);

  static void Finalize(void);

  clang::SourceLocation getEndLocationFromBegin(clang::SourceRange Range);

  bool removeParamFromFuncDecl(const clang::ParmVarDecl *PV,
                                      unsigned int NumParams,
                                      int ParamPos);

  bool removeArgFromCallExpr(clang::CallExpr *CallE,
                                    int ParamPos);
                                    
  bool removeVarFromDeclStmt(clang::DeclStmt *DS,
                                    const clang::VarDecl *VD,
                                    clang::Decl *PrevDecl,
                                    bool IsFirstDecl);

  bool getExprString(const clang::Expr *E, 
                            std::string &ES);

  bool getStmtString(const clang::Stmt *S, 
                            std::string &Str);

  bool replaceExpr(const clang::Expr *E, 
                          const std::string &ES);

  bool replaceExprNotInclude(const clang::Expr *E, 
                          const std::string &ES);

  bool addLocalVarToFunc(const std::string &VarStr,
                                clang::FunctionDecl *FD);
                                 
  std::string getStmtIndentString(clang::Stmt *S,
                                         clang::SourceManager *SrcManager);

  bool addNewAssignStmtBefore(clang::Stmt *BeforeStmt,
                                     const std::string &VarName,
                                     clang::Expr *RHS,
                                     bool NeedParen);

  bool addStringBeforeStmt(clang::Stmt *BeforeStmt,
                                  const std::string &Str,
                                  bool NeedParen);

  bool addStringAfterStmt(clang::Stmt *AfterStmt, 
                                 const std::string &Str);

  bool addStringAfterVarDecl(clang::VarDecl *VD,
                                    const std::string &Str);

  bool replaceVarDeclName(clang::VarDecl *VD,
                              const std::string &NameStr);

  bool replaceFunctionDeclName(clang::FunctionDecl *FD,
                              const std::string &NameStr);

  const char *getTmpVarNamePrefix(void);

  void getStringBetweenLocs(std::string &Str, 
                                   clang::SourceLocation LocStart,
                                   clang::SourceLocation LocEnd);

  bool getDeclGroupStrAndRemove(clang::DeclGroupRef DGR, 
                                       std::string &Str);

  clang::SourceLocation getDeclGroupRefEndLoc(clang::DeclGroupRef DGR);

  bool getDeclStmtStrAndRemove(clang::DeclStmt *DS, 
                                      std::string &Str);

  clang::SourceLocation getDeclStmtEndLoc(clang::DeclStmt *DS);

  bool removeAStarBefore(const clang::Decl *D);

  bool removeAStarAfter(const clang::Expr *E);

  bool removeAnAddrOfAfter(const clang::Expr *E);

  bool removeASymbolAfter(const clang::Expr *E, char Symbol);

  bool insertAnAddrOfBefore(const clang::DeclRefExpr *DRE);

  bool insertAStarBefore(const clang::Expr *E);

  bool removeVarInitExpr(const clang::VarDecl *VD);

  bool removeVarDecl(const clang::VarDecl *VD,
                            clang::DeclGroupRef DGR);

  void getTmpTransName(unsigned Postfix, std::string &Name);

  bool insertStringBeforeFunc(const clang::FunctionDecl *FD,
                              const std::string &Str);
private:

  static RewriteUtils *Instance;

  static const char *TmpVarNamePrefix;

  clang::Rewriter *TheRewriter;

  clang::SourceManager *SrcManager;

  RewriteUtils(void)
  : TheRewriter(NULL),
    SrcManager(NULL)
  { }

  ~RewriteUtils(void) { }

  int getOffsetUntil(const char *Buf, char Symbol);

  int getSkippingOffset(const char *Buf, char Symbol);

  clang::SourceLocation getEndLocationUntil(clang::SourceRange Range,
                                            char Symbol);

  clang::SourceLocation getLocationUntil(clang::SourceLocation Loc,
                                           char Symbol);

  clang::SourceLocation getEndLocationAfter(clang::SourceRange Range,
                                           char Symbol);

  clang::SourceLocation getLocationAfter(clang::SourceLocation StartLoc,
                                           char Symbol);

  unsigned getLocationOffsetAndFileID(clang::SourceLocation Loc,
                                             clang::FileID &FID,
                                             clang::SourceManager *SrcManager);

  clang::SourceLocation getVarDeclTypeLocEnd(const clang::VarDecl *VD);

  clang::SourceLocation 
    getParamSubstringLocation(clang::SourceLocation StartLoc, size_t Size, 
                         const std::string &Substr);

  void indentAfterNewLine(llvm::StringRef Str,
                                 std::string &NewStr,
                                 const std::string &IndentStr);

  unsigned getOffsetBetweenLocations(clang::SourceLocation StartLoc,
                                            clang::SourceLocation EndLoc,
                                            clang::SourceManager *SrcManager);

  // Unimplemented
  RewriteUtils(const RewriteUtils &);

  void operator=(const RewriteUtils &);
};

#endif
