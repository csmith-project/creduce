#ifndef STRUCT_TO_SCALAR_H
#define STRUCT_TO_SCALAR_H

#include <string>
#include <set>
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class FunctionDecl;
  class FieldDecl;
  class MemberExpr;
  class VarDecl;
  class InitListExpr;
  class DeclStmt;
  class Expr;
}

class ATSCollectionVisitor;

class AggregateToScalar : public Transformation {
friend class ATSCollectionVisitor;

public:

  AggregateToScalar(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      AggregateAccessVisitor(NULL),
      TheFieldDecl(NULL)
  { }

  ~AggregateToScalar(void);

private:
  
  typedef std::set<clang::MemberExpr *> VarRefsSet;
  
  typedef llvm::DenseMap<clang::FieldDecl *, VarRefsSet *> ValidFieldsMap;

  typedef llvm::SmallVector<unsigned int, 5> FieldIdxVector;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void addVarRefExpr(clang::FieldDecl *FD, clang::MemberExpr *ME);

  bool handleOneMemberExpr(clang::MemberExpr *ME, clang::ASTContext &Ctx);

  clang::VarDecl *getRefVarDeclAndFieldIdxs(clang::MemberExpr *ME,
                    FieldIdxVector &FieldIdxs);

  bool addTmpVar(clang::VarDecl *VD, const std::string &VarName,
                 const std::string *InitStr, clang::ASTContext &Ctx);

  void createNewVarName(clang::VarDecl *VD,
                        const FieldIdxVector &FieldIdxs,
                        std::string &VarName);

  bool getInitString(const FieldIdxVector &FieldIdxs,
                     clang::InitListExpr *ILE,
                     std::string &InitStr);

  bool replaceMemberExpr(clang::MemberExpr *ME, const std::string &NewName);

  void handleTheFieldDecl(clang::ASTContext &Ctx);

  clang::Expr *ignoreSubscriptExprImpCasts(clang::Expr *E,
                    FieldIdxVector &FieldIdxs);

  llvm::DenseMap<clang::VarDecl *, std::string> ProcessedVarDecls;

  llvm::DenseMap<clang::VarDecl *, clang::DeclStmt *> VarDeclToDeclStmtMap;

  llvm::DenseMap<clang::VarDecl *, clang::DeclGroupRef> VarDeclToDeclGroupMap;

  ValidFieldsMap ValidFields;

  ATSCollectionVisitor *AggregateAccessVisitor;

  clang::FieldDecl *TheFieldDecl;

  // Unimplemented
  AggregateToScalar(void);

  AggregateToScalar(const AggregateToScalar &);

  void operator=(const AggregateToScalar &);
};
#endif
