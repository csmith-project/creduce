#ifndef REWRITE_UTILS_H
#define REWRITE_UTILS_h

#include <string>
#include "clang/Basic/SourceLocation.h"

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
}

class RewriteUtils {
public:
  static clang::SourceLocation getEndLocationFromBegin
    (clang::SourceRange Range, clang::Rewriter *TheRewriter);

  static bool removeParamFromFuncDecl(const clang::ParmVarDecl *PV,
                                      unsigned int NumParams,
                                      int ParamPos,
                                      clang::Rewriter *TheRewriter,
                                      clang::SourceManager *SrcManager);

  static bool removeArgFromCallExpr(clang::CallExpr *CallE,
                                    int ParamPos,
                                    clang::Rewriter *TheRewriter,
                                    clang::SourceManager *SrcManager);
                                    
  static bool removeVarFromDeclStmt(clang::DeclStmt *DS,
                                    clang::VarDecl *VD,
                                    clang::Decl *PrevDecl,
                                    bool IsFirstDecl,
                                    clang::Rewriter *TheRewriter,
                                    clang::SourceManager *SrcManager);

  static bool getExprString(const clang::Expr *E, 
                            std::string &ES,
                            clang::Rewriter *TheRewriter,
                            clang::SourceManager *SrcManager);

  static bool getStmtString(const clang::Stmt *S, 
                            std::string &Str,
                            clang::Rewriter *TheRewriter,
                            clang::SourceManager *SrcManager);

  static bool replaceExpr(const clang::Expr *E, 
                          const std::string &ES,
                          clang::Rewriter *TheRewriter,
                          clang::SourceManager *SrcManager);

  static bool replaceExprNotInclude(const clang::Expr *E, 
                          const std::string &ES,
                          clang::Rewriter *TheRewriter,
                          clang::SourceManager *SrcManager);

  static bool addLocalVarToFunc(const std::string &VarStr,
                                clang::FunctionDecl *FD,
                                clang::Rewriter *TheRewriter,
                                clang::SourceManager *SrcManager);
                                 
  static std::string getStmtIndentString(clang::Stmt *S,
                                         clang::SourceManager *SrcManager);

  static bool addNewAssignStmtBefore(clang::Stmt *BeforeStmt,
                                     const std::string &VarName,
                                     clang::Expr *RHS,
                                     bool NeedParen,
                                     clang::Rewriter *TheRewriter,
                                     clang::SourceManager *SrcManager);

  static bool addStringBeforeStmt(clang::Stmt *BeforeStmt,
                                  const std::string &Str,
                                  bool NeedParen,
                                  clang::Rewriter *TheRewriter,
                                  clang::SourceManager *SrcManager);

  static bool addStringAfterStmt(clang::Stmt *AfterStmt, 
                                 const std::string &Str,
                                 clang::Rewriter *TheRewriter,
                                 clang::SourceManager *SrcManager);

  static bool addStringAfterVarDecl(clang::VarDecl *VD,
                                    const std::string &Str,
                                    clang::Rewriter *TheRewriter,
                                    clang::SourceManager *SrcManager);

  static bool replaceVarDeclName(clang::VarDecl *VD,
                              const std::string &NameStr,
                              clang::Rewriter *TheRewriter,
                              clang::SourceManager *SrcManager);

  static bool replaceFunctionDeclName(clang::FunctionDecl *FD,
                              const std::string &NameStr,
                              clang::Rewriter *TheRewriter,
                              clang::SourceManager *SrcManager);

  static const char *getTmpVarNamePrefix(void);

  static void getStringBetweenLocs(std::string &Str, 
                                   clang::SourceLocation LocStart,
                                   clang::SourceLocation LocEnd, 
                                   clang::Rewriter *TheRewriter,
                                   clang::SourceManager *SrcManager);

  static bool getDeclGroupStrAndRemove(clang::DeclGroupRef DGR, 
                                       std::string &Str,
                                       clang::Rewriter *TheRewriter,
                                       clang::SourceManager *SrcManager);

  static clang::SourceLocation getDeclGroupRefEndLoc(clang::DeclGroupRef DGR,
                                          clang::Rewriter* TheRewriter,
                                          clang::SourceManager *SrcManager);

  static bool getDeclStmtStrAndRemove(clang::DeclStmt *DS, 
                                      std::string &Str,
                                      clang::Rewriter *TheRewriter,
                                      clang::SourceManager *SrcManager);

  static clang::SourceLocation getDeclStmtEndLoc(clang::DeclStmt *DS,
                                          clang::Rewriter* TheRewriter,
                                          clang::SourceManager *SrcManager);

private:

  static const char *TmpVarNamePrefix;

  static int getOffsetUntil(const char *Buf, char Symbol);

  static int getSkippingOffset(const char *Buf, char Symbol);

  static clang::SourceLocation getEndLocationUntil(clang::SourceRange Range,
                                           char Symbol, 
                                           clang::Rewriter *TheRewriter,
                                           clang::SourceManager *SrcManager);

  static clang::SourceLocation getEndLocationAfter(clang::SourceRange Range,
                                           char Symbol, 
                                           clang::Rewriter *TheRewriter,
                                           clang::SourceManager *SrcManager);

  static clang::SourceLocation getLocationAfter(clang::SourceLocation StartLoc,
                                           char Symbol,
                                           clang::Rewriter *TheRewriter,
                                           clang::SourceManager *SrcManager);

  static unsigned getLocationOffsetAndFileID(clang::SourceLocation Loc,
                                             clang::FileID &FID,
                                             clang::SourceManager *SrcManager);

  static clang::SourceLocation getVarDeclTypeLocEnd(clang::VarDecl *VD,
                                 clang::Rewriter *TheRewriter);

  static clang::SourceLocation 
    getParamSubstringLocation(clang::SourceLocation StartLoc, size_t Size, 
                         const std::string &Substr, 
                         clang::Rewriter *TheRewriter,
                         clang::SourceManager *SrcManager);

  static void indentAfterNewLine(llvm::StringRef Str,
                                 std::string &NewStr,
                                 const std::string &IndentStr);

  static unsigned getOffsetBetweenLocations(clang::SourceLocation StartLoc,
                                            clang::SourceLocation EndLoc,
                                            clang::SourceManager *SrcManager);

  RewriteUtils(void);

  ~RewriteUtils(void);
};

#endif
