#ifndef REWRITE_UTILS_H
#define REWRITE_UTILS_h

namespace clang {
  class ParmVarDecl;
  class Rewriter;
  class SourceManager;
  class CallExpr;
}

class RewriteUtils {
public:
  static bool removeParamFromFuncDecl(const clang::ParmVarDecl *PV,
                                      unsigned int NumParams,
                                      int ParamPos,
                                      clang::Rewriter *TheRewriter,
                                      clang::SourceManager *SrcManager);

  static bool removeArgFromCallExpr(clang::CallExpr *CallE,
                                    int ParamPos,
                                    clang::Rewriter *TheRewriter,
                                    clang::SourceManager *SrcManager);
                                    
                                 
private:

  RewriteUtils(void);

  ~RewriteUtils(void);
};

#endif
