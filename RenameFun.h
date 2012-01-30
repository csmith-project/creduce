#ifndef RENAME_FUN_H
#define RENAME_FUN_H

#include <string>
#include <vector>
#include "llvm/ADT/DenseMap.h"
#include "Transformation.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class FunctionDecl;
}

class RNFunCollectionVisitor;
class RenameFunVisitor;

class RenameFun : public Transformation {
friend class RNFunCollectionVisitor;
friend class RenameFunVisitor;

public:

  RenameFun(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      FunCollectionVisitor(NULL),
      RenameVisitor(NULL),
      FunNamePrefix("fn"),
      FunNamePostfix(0)
  { }

  ~RenameFun(void);

  virtual bool skipCounter(void) {
    return true;
  }

private:
  
  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void addFun(clang::FunctionDecl *FD);

  RNFunCollectionVisitor *FunCollectionVisitor;

  RenameFunVisitor *RenameVisitor;

  llvm::DenseMap<clang::FunctionDecl *, std::string> FunToNameMap;

  const std::string FunNamePrefix;

  unsigned int FunNamePostfix;

  // Unimplemented
  RenameFun(void);

  RenameFun(const RenameFun &);

  void operator=(const RenameFun &);
};
#endif
