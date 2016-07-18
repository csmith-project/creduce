
// Here is a quick command line for testing:
// ./clang_delta --transformation=vector-to-array ~/empty.cpp
// --counter=1./clang_delta --transformation=vector-to-array ~/empty.cpp
// --counter=1

// TODO:
// pointer/array of vector
// dependent type vector
// vector used before class definition (lesser priotity, not common)
// vector not inside std::
// all scopes, global, static, member, local, threadsafe, whatever
// vector class without params
// other templates not changed

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "VectorToArray.h"

#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

#include <cctype>

#define ARRAY_SUFFIX "[10]"

using namespace clang;

static const char *DescriptionMsg =
    "Try replacing vectors with arrays. It transforms the following code: \n\
  vector<int> v' \n\
to \n\
  int v[8]\n";

static RegisterTransformation<VectorToArray> Trans("vector-to-array",
                                                   DescriptionMsg);

class VectorToArrayCollectionVisitor
    : public RecursiveASTVisitor<VectorToArrayCollectionVisitor> {
public:
  explicit VectorToArrayCollectionVisitor(VectorToArray *instance)
      : Consumer(instance) {}

  bool VisitNamedDecl(NamedDecl *D) { return true; }

  bool VisitClassTemplateDecl(ClassTemplateDecl *D) {
    if (Consumer->isInIncludedFile(D))
      return true;
    auto *ns = dyn_cast<NamespaceDecl>(D->getDeclContext());
    if (!ns)
      return true;
    if (ns->getName() != "std")
      return true;
    if (!dyn_cast<TranslationUnitDecl>(ns->getDeclContext()))
      return true;
    if (!D->isThisDeclarationADefinition())
      return true;
    if (D->getName() != "vector")
      return true;

    Consumer->TheVectorDecl = D;
    return true;
  }

  bool VisitVarDecl(VarDecl *D) {
    if (Consumer->isInIncludedFile(D))
      return true;
    const Type *Ty = D->getType().getTypePtr();
    if (!Ty)
      return true;
    CXXRecordDecl *CXXRD = Ty->getAsCXXRecordDecl();
    if (!CXXRD)
      return true;
    CXXRecordDecl *CXXRDT = CXXRD->getTemplateInstantiationPattern();
    if (!CXXRDT)
      return true;
    ClassTemplateDecl *CTD = CXXRDT->getDescribedClassTemplate();
    while (CTD && !CTD->isThisDeclarationADefinition()) {
      CTD = CTD->getPreviousDecl();
    }
    if (!CTD || CTD != Consumer->TheVectorDecl)
      return true;
    ++Consumer->ValidInstanceNum;
    if (Consumer->ValidInstanceNum == Consumer->TransformationCounter)
      Consumer->TheVarDecl = D;
    return true;
  }

  // bool VisitVarDecl(VarDecl *VD) {
  //   if (Consumer->isInIncludedFile(VD))
  //     return true;
  //   if (!VD->isThisDeclarationADefinition())
  //     return true;
  //   if (VD->isInvalidDecl())
  //     return true;

  //   QualType QT = VD->getType();
  //   const Type *T = QT->getTypePtr();
  //   const CXXRecordDecl *RD = T->getAsCXXRecordDecl();
  //   if (RD == nullptr)
  //     return true;
  //   continue from here

  // }
private:
  VectorToArray *Consumer;
};

class VectorToArrayRewriteVisitor
    : public RecursiveASTVisitor<VectorToArrayRewriteVisitor> {
private:
  const std::string getVectorElemTypeName(VarDecl *VD) {
    CXXRecordDecl *CXXRD = VD->getType().getTypePtr()->getAsCXXRecordDecl();
    auto CTSD = dyn_cast<ClassTemplateSpecializationDecl>(CXXRD);
    const TemplateArgument &TmplArg = CTSD->getTemplateArgs()[0];
    return TmplArg.getAsType().getAsString();
  }

public:
  bool VisitVarDecl(VarDecl *VD) {
    if (VD != Consumer->TheVarDecl)
      return true;

    Rewriter &TheRewriter = Consumer->TheRewriter;
    const SourceManager &SM = TheRewriter.getSourceMgr();

    SourceLocation NameLoc = VD->getLocation();
    IdentifierInfo *IdInfo = VD->getIdentifier();
    assert(IdInfo && "Nameless variable");
    unsigned NameLength = IdInfo->getLength();
    assert(NameLength && "Variable name has no length");

    SourceLocation TypeLocStart = VD->getLocStart();
    const std::string ElemTypeName = getVectorElemTypeName(VD);

    SourceLocation NameLocM1 = NameLoc.getLocWithOffset(-1);
    bool isInvalid = false;
    const char *charBeforeName = SM.getCharacterData(NameLocM1, &isInvalid);
    assert(!isInvalid && "failed to get char before name");

    TheRewriter.ReplaceText(NameLoc.getLocWithOffset(NameLength), 0,
                            ARRAY_SUFFIX);
    if (!std::isspace(*charBeforeName))
      TheRewriter.InsertText(NameLoc, " ");
    TheRewriter.ReplaceText(SourceRange(TypeLocStart, NameLocM1), ElemTypeName);

    return true;
  }
  explicit VectorToArrayRewriteVisitor(VectorToArray *Cons) : Consumer(Cons) {}

private:
  VectorToArray *Consumer;
};

void VectorToArray::Initialize(ASTContext &ctx) {
  Transformation::Initialize(ctx);
  CollectionVisitor = new VectorToArrayCollectionVisitor(this);
  RewriteVisitor = new VectorToArrayRewriteVisitor(this);
}

void VectorToArray::HandleTranslationUnit(ASTContext &ctx) {

  CollectionVisitor->TraverseDecl(ctx.getTranslationUnitDecl());
  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  TransAssert(RewriteVisitor && "NULL RewriteVisitor!");
  TransAssert(TheVectorDecl && "NULL TheRecordDecl!");
  TransAssert(TheVarDecl && "NULL TheVarDecl!");
  // RewriteVisitor->TraverseDecl(ctx.getTranslationUnitDecl());
  // ORI NOTE: This is much more efficient, but will not work if we want
  //           xto do more than one change
  RewriteVisitor->VisitVarDecl(TheVarDecl);

  if (ctx.getDiagnostics().hasErrorOccurred() ||
      ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}
