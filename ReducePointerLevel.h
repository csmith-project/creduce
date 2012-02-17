#ifndef REDUCE_POINTER_LEVEL_H
#define REDUCE_POINTER_LEVEL_H

#include <string>
#include "Transformation.h"
#include "llvm/ADT/SmallSet.h"

namespace clang {
  class DeclGroupRef;
  class ASTContext;
  class VarDecl;
}

class PointerLevelCollectionVisitor;
class PointerLevelAnalysisVisitor;

class ReducePointerLevel : public Transformation {
friend class PointerLevelCollectionVisitor;
friend class PointerLevelAnalysisVisitor;

public:

  ReducePointerLevel(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc),
      CollectionVisitor(NULL),
      AnalysisVisitor(NULL),
      MaxIndirectLevel(0),
      TheVarDecl(NULL)
  { }

  ~ReducePointerLevel(void);

private:
  
  typedef llvm::SmallPtrSet<const clang::VarDecl *, 20> VarDeclsSet;

  typedef llvm::DenseMap<unsigned int, VarDeclsSet *> LevelToVarMap;

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTopLevelDecl(clang::DeclGroupRef D);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  VarDeclsSet VisitedVarDecls;

  VarDeclsSet ValidVarDecls;

  LevelToVarMap AllPtrVarDecls;

  PointerLevelCollectionVisitor *CollectionVisitor;

  PointerLevelAnalysisVisitor *AnalysisVisitor;

  unsigned int MaxIndirectLevel;

  clang::VarDecl *TheVarDecl;

  // Unimplemented
  ReducePointerLevel(void);

  ReducePointerLevel(const ReducePointerLevel &);

  void operator=(const ReducePointerLevel &);
};

#endif
