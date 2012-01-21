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

  static bool replaceExpr(const clang::Expr *E, 
                          const std::string &ES,
                          clang::Rewriter *TheRewriter,
                          clang::SourceManager *SrcManager);

  static bool addLocalVarToFunc(const std::string &VarStr,
                                clang::FunctionDecl *FD,
                                clang::Rewriter *TheRewriter,
                                clang::SourceManager *SrcManager);
                                 
  static std::string getStmtIndentString(clang::Stmt *S,
                                         clang::SourceManager *SrcManager);

private:

  static clang::SourceLocation getEndLocationUtil(clang::SourceRange Range,
                                           char Symbol, 
                                           clang::Rewriter *TheRewriter,
                                           clang::SourceManager *SrcManager);

  static unsigned getLocationOffsetAndFileID(clang::SourceLocation Loc,
                                             clang::FileID &FID,
                                             clang::SourceManager *SrcManager);

  RewriteUtils(void);

  ~RewriteUtils(void);
};

#endif
