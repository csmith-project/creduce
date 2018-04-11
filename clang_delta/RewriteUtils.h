//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2015, 2016, 2017, 2018 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef REWRITE_UTILS_H
#define REWRITE_UTILS_H

#include <string>
#include "clang/Basic/SourceLocation.h"
#include "clang/AST/NestedNameSpecifier.h"

#ifndef ENABLE_TRANS_ASSERT
  #define TransAssert(x) {if (!(x)) exit(-1);}
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
  class TemplateDecl;
  class Stmt;
  class DeclGroupRef;
  class DeclRefExpr;
  class NamedDecl;
  class IfStmt;
  class Type;
  class FieldDecl;
  class CXXConstructExpr;
  class RecordDecl;
  class CXXMemberCallExpr;
  class RecordTypeLoc;
  class CXXDestructorDecl;
  class CXXCtorInitializer;
  class CXXRecordDecl;
  class ClassTemplateDecl;
  class CXXMethodDecl;
  class NestedNameSpecifierLoc;
  class ValueDecl;
}

class RewriteUtils {
public:
  static RewriteUtils *GetInstance(clang::Rewriter *RW);

  static void Finalize(void);

  clang::SourceLocation getEndLocationFromBegin(clang::SourceRange Range);

  bool removeParamFromFuncDecl(const clang::ParmVarDecl *PV,
                                      unsigned int NumParams,
                                      int ParamPos);

  bool removeArgFromCallExpr(const clang::CallExpr *CallE,
                             int ParamPos);
                                    
  bool removeArgFromCXXConstructExpr(const clang::CXXConstructExpr *CE,
                                     int ParamPos);
                                    
  bool removeVarFromDeclStmt(clang::DeclStmt *DS,
                                    const clang::VarDecl *VD,
                                    clang::Decl *PrevDecl,
                                    bool IsFirstDecl,
                                    bool *StmtRemoved);

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

  bool addStringBeforeStmtAndReplaceExpr(clang::Stmt *BeforeStmt,
                                  const std::string &StmtStr,
                                  const clang::Expr *E,
                                  const std::string &ExprStr,
                                  bool NeedParen);

  bool addStringAfterStmt(clang::Stmt *AfterStmt, 
                                 const std::string &Str);

  bool addStringAfterVarDecl(const clang::VarDecl *VD,
                             const std::string &Str);

  bool addStringAfterFuncDecl(const clang::FunctionDecl *FD,
                              const std::string &Str);

  bool replaceVarDeclName(clang::VarDecl *VD,
                          const std::string &NameStr);

  bool replaceFunctionDeclName(const clang::FunctionDecl *FD,
                          const std::string &NameStr);

  bool replaceRecordDeclName(const clang::RecordDecl *RD,
                             const std::string &NameStr);

  bool replaceVarTypeName(const clang::VarDecl *VD,
                          const std::string &NameStr);

  const char *getTmpVarNamePrefix(void);

  void getStringBetweenLocs(std::string &Str, 
                            clang::SourceLocation LocStart,
                            clang::SourceLocation LocEnd);

  void getStringBetweenLocsAfterStart(std::string &Str, 
                                      clang::SourceLocation LocStart,
                                      clang::SourceLocation LocEnd);

  bool getDeclGroupStrAndRemove(clang::DeclGroupRef DGR, 
                                       std::string &Str);

  bool getEntireDeclGroupStrAndRemove(clang::DeclGroupRef DGR,
                                      std::string &Str);

  clang::SourceLocation getDeclGroupRefEndLoc(clang::DeclGroupRef DGR);

  bool getDeclStmtStrAndRemove(clang::DeclStmt *DS, 
                                      std::string &Str);

  clang::SourceLocation getDeclStmtEndLoc(clang::DeclStmt *DS);

  bool removeAStarBefore(const clang::Decl *D);

  bool removeAStarAfter(const clang::Expr *E);

  bool removeAnAddrOfAfter(const clang::Expr *E);

  bool removeASymbolAfter(const clang::Expr *E, char Symbol);

  bool insertAnAddrOfBefore(const clang::Expr *E);

  bool insertAStarBefore(const clang::Expr *E);

  bool removeVarInitExpr(const clang::VarDecl *VD);

  bool removeVarDecl(const clang::VarDecl *VD,
                            clang::DeclGroupRef DGR);

  bool removeVarDecl(const clang::VarDecl *VD);

  void getTmpTransName(unsigned Postfix, std::string &Name);

  bool insertStringBeforeFunc(const clang::FunctionDecl *FD,
                              const std::string &Str);

  bool insertStringBeforeTemplateDecl(const clang::TemplateDecl *D,
                                      const std::string &Str);

  bool replaceUnionWithStruct(const clang::NamedDecl *ND);

  bool removeIfAndCond(const clang::IfStmt *IS);

  clang::SourceLocation getLocationUntil(clang::SourceLocation Loc,
                                         char Symbol);

  clang::SourceLocation getLocationAfter(clang::SourceLocation Loc,
                                         char Symbol);

  bool removeArraySubscriptExpr(const clang::Expr *E);

  bool getFunctionDefStrAndRemove(const clang::FunctionDecl *FD,
                                  std::string &Str);

  bool getFunctionDeclStrAndRemove(const clang::FunctionDecl *FD,
                                   std::string &Str);

  bool replaceFunctionDefWithStr(const clang::FunctionDecl *FD,
                                 const std::string &Str);

  clang::SourceLocation getEndLocationUntil(clang::SourceRange Range,
                                            char Symbol);

  bool removeFieldDecl(const clang::FieldDecl *FD);

  bool removeDecl(const clang::Decl *D);

  bool replaceNamedDeclName(const clang::NamedDecl *ND,
                            const std::string &NameStr);

  ///\brief Replaces a value decl with a given string.
  ///
  ///For example: \code
  /// enum E {...};
  /// template <E argName> struct S { } => template <int> struct S { }
  ///\endcode
  ///
  ///\param[in] VD - The decl to be replaced.
  ///\param[in] Str - The replacement
  ///\returns true on success.
  ///
  bool replaceValueDecl(const clang::ValueDecl *ValD, const std::string &Str);

  bool replaceCXXDtorCallExpr(const clang::CXXMemberCallExpr *CE,
                              std::string &Name);

  bool removeSpecifier(clang::NestedNameSpecifierLoc Loc);

  bool replaceSpecifier(clang::NestedNameSpecifierLoc Loc, 
                        const std::string &Name);

  void getQualifierAsString(clang::NestedNameSpecifierLoc Loc,
                            std::string &Str);

  void getSpecifierAsString(clang::NestedNameSpecifierLoc Loc,
                            std::string &Str);

  bool replaceRecordType(clang::RecordTypeLoc &RTLoc, const std::string &Name);

  bool isSingleDecl(const clang::VarDecl *VD);

  bool isTheFirstDecl(const clang::VarDecl *VD);

  bool removeTextFromLeftAt(clang::SourceRange Range, char C,
                            clang::SourceLocation EndLoc);

  clang::SourceLocation getLocationFromLeftUntil(clang::SourceLocation StartLoc,
                                                 char C);

  bool removeTextUntil(clang::SourceRange Range, char C);

  bool replaceCXXDestructorDeclName(const clang::CXXDestructorDecl *DtorDecl, 
                                    const std::string &Name);

  bool removeCXXCtorInitializer(const clang::CXXCtorInitializer *Init,
                                unsigned Index, unsigned NumInits);

  bool removeClassDecls(const clang::CXXRecordDecl *CXXRD);

  bool removeClassTemplateDecls(const clang::ClassTemplateDecl *TmplD);

  bool replaceCXXMethodNameAfterQualifier(
         const clang::NestedNameSpecifierLoc *QualLoc,
         const clang::CXXMethodDecl *MD,
         const std::string &NewName);

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

  clang::SourceLocation getEndLocationAfter(clang::SourceRange Range,
                                            char Symbol);

  clang::SourceLocation getLocationAfterSkiping(clang::SourceLocation StartLoc,
                                                char Symbol);

  unsigned getLocationOffsetAndFileID(clang::SourceLocation Loc,
                                             clang::FileID &FID,
                                             clang::SourceManager *SrcManager);

  clang::SourceLocation getVarDeclTypeLocEnd(const clang::VarDecl *VD);

  clang::SourceLocation getVarDeclTypeLocBegin(const clang::VarDecl *VD);

  clang::SourceLocation 
    getParamSubstringLocation(clang::SourceLocation StartLoc, size_t Size, 
                         const std::string &Substr);

  void indentAfterNewLine(llvm::StringRef Str,
                                 std::string &NewStr,
                                 const std::string &IndentStr);

  void addOpenParenBeforeStmt(clang::Stmt *S, const std::string &IndentStr);

  bool addStringBeforeStmtInternal(clang::Stmt *S, const std::string &Str,
                                   const std::string &IndentStr,
                                   bool NeedParen);

  unsigned getOffsetBetweenLocations(clang::SourceLocation StartLoc,
                                            clang::SourceLocation EndLoc,
                                            clang::SourceManager *SrcManager);

  clang::SourceLocation skipPossibleTypeRange(const clang::Type *Ty,
                                           clang::SourceLocation OrigEndLoc,
                                           clang::SourceLocation VarStartLoc);

  void skipRangeByType(const std::string &BufStr, 
                       const clang::Type *Ty, int &Offset);

  bool removeArgFromExpr(const clang::Expr *E, int ParamPos);

  const clang::Expr *getArgWrapper(const clang::Expr *E, int ParamPos);

  unsigned getNumArgsWrapper(const clang::Expr *E);

  clang::SourceLocation getExpansionEndLoc(clang::SourceLocation EndLoc);

  clang::SourceLocation getMacroExpansionLoc(clang::SourceLocation Loc);

  clang::SourceRange getFileLocSourceRange(clang::SourceRange LocRange);

  // Unimplemented
  RewriteUtils(const RewriteUtils &);

  void operator=(const RewriteUtils &);
};

#endif
