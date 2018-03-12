//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2015, 2016, 2017 The University of Utah
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "SimplifyStruct.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Lex/Lexer.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"
#include "clang/AST/RecordLayout.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"This pass replaces a struct with its parent if it has only one \
field, and this field is a struct, e.g, \n\
  struct S1 { \n\
    int f1;\n\
    int f2;\n\
  }\n\
  struct S2 { \n\
    struct S1 f1;\n\
  } \n\
In the above code, struct S2 will be replaced with struct S1, including\n\
all its referenced. \n";

static RegisterTransformation<SimplifyStruct>
         Trans("simplify-struct", DescriptionMsg);

class SimplifyStructCollectionVisitor : public 
  RecursiveASTVisitor<SimplifyStructCollectionVisitor> {

public:
  explicit SimplifyStructCollectionVisitor(SimplifyStruct *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitRecordDecl(RecordDecl *RD);

private:

  SimplifyStruct *ConsumerInstance;
};

class SimplifyStructRewriteVisitor : public 
  RecursiveASTVisitor<SimplifyStructRewriteVisitor> {

public:
  explicit SimplifyStructRewriteVisitor(SimplifyStruct *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitVarDecl(VarDecl *VD);

  bool VisitRecordDecl(RecordDecl *RD);

  bool VisitRecordTypeLoc(RecordTypeLoc RTLoc);

  bool VisitMemberExpr(MemberExpr *ME);

private:

  SimplifyStruct *ConsumerInstance;
};

bool SimplifyStructCollectionVisitor::VisitRecordDecl(RecordDecl *RD)
{
  if (ConsumerInstance->isInIncludedFile(RD))
    return true;
  if (!RD->isThisDeclarationADefinition() || !RD->isStruct())
    return true;
  if (ConsumerInstance->isSpecialRecordDecl(RD))
    return true;
  if (RD->isInvalidDecl())
    return true;

  const ASTRecordLayout &Info = ConsumerInstance->Context->getASTRecordLayout(RD);
  unsigned Count = Info.getFieldCount();
  if (Count != 1)
    return true;

  const FieldDecl *FD = *(RD->field_begin());
  TransAssert(FD && "Invalid FieldDecl!");
  const Type *Ty = FD->getType().getTypePtr();
  const RecordType *RT = Ty->getAs<RecordType>();
  if (!RT)
    return true;

  const RecordDecl *NestedRD = RT->getDecl();
  if (NestedRD->getNameAsString() == "")
    return true;

  ConsumerInstance->ValidInstanceNum++;
  if (ConsumerInstance->TransformationCounter == 
      ConsumerInstance->ValidInstanceNum) {
    ConsumerInstance->TheRecordDecl = 
      dyn_cast<RecordDecl>(RD->getCanonicalDecl());
    ConsumerInstance->ReplacingRecordDecl = 
      dyn_cast<RecordDecl>(NestedRD->getCanonicalDecl());
    ConsumerInstance->setQualifierFlags(FD);
  }
  return true;
}

bool SimplifyStructRewriteVisitor::VisitVarDecl(VarDecl *VD)
{
  if (!ConsumerInstance->ConstField && !ConsumerInstance->VolatileField)
    return true;

  QualType QT = VD->getType();
  const Type *Ty = QT.getTypePtr();
  const RecordType *RT = Ty->getAs<RecordType>();
  if (!RT)
    return true;

  const RecordDecl *RD = RT->getDecl();
  if (RD != ConsumerInstance->TheRecordDecl)
    return true;

  SourceLocation LocStart = VD->getLocStart();
  void *LocPtr = LocStart.getPtrEncoding();
  if (ConsumerInstance->VisitedVarDeclLocs.count(LocPtr))
    return true;

  ConsumerInstance->VisitedVarDeclLocs.insert(LocPtr);

  std::string QualStr = "";
  if (ConsumerInstance->ConstField && !QT.isConstQualified())
    QualStr += "const ";
  if (ConsumerInstance->VolatileField && !QT.isVolatileQualified())
    QualStr += "volatile ";
  ConsumerInstance->TheRewriter.InsertText(LocStart, QualStr);
  return true;
}

bool SimplifyStructRewriteVisitor::VisitRecordDecl(RecordDecl *RD)
{
  RecordDecl *CanonicalRD = dyn_cast<RecordDecl>(RD->getCanonicalDecl());
  if (CanonicalRD != ConsumerInstance->TheRecordDecl)
    return true;

  SourceLocation LocStart = RD->getLocation();
  void *LocPtr = LocStart.getPtrEncoding();
  if (!ConsumerInstance->VisitedLocs.count(LocPtr)) {
    ConsumerInstance->VisitedLocs.insert(LocPtr);
    std::string RPName = 
      ConsumerInstance->ReplacingRecordDecl->getNameAsString();
    if (RD->getNameAsString() != "") {
      ConsumerInstance->RewriteHelper->replaceRecordDeclName(RD, RPName);
    }
    else {
      ConsumerInstance->TheRewriter.ReplaceText(LocStart, 
        /*struct*/6, "struct " + RPName);
    }
  }

  if (!RD->isThisDeclarationADefinition())
    return true;

  SourceLocation LBLoc = RD->getBraceRange().getBegin();
  SourceLocation RBLoc = RD->getBraceRange().getEnd();
  ConsumerInstance->TheRewriter.RemoveText(SourceRange(LBLoc, RBLoc));
  return true;
}

bool SimplifyStructRewriteVisitor::VisitRecordTypeLoc(RecordTypeLoc RTLoc)
{
  const Type *Ty = RTLoc.getTypePtr();
  if (Ty->isUnionType())
    return true;

  RecordDecl *RD = RTLoc.getDecl();
  RecordDecl *CanonicalRD = dyn_cast<RecordDecl>(RD->getCanonicalDecl());
  if (CanonicalRD != ConsumerInstance->TheRecordDecl)
    return true;

  SourceLocation LocStart = RTLoc.getLocStart();
  void *LocPtr = LocStart.getPtrEncoding();
  if (ConsumerInstance->VisitedLocs.count(LocPtr))
    return true;
  ConsumerInstance->VisitedLocs.insert(LocPtr);

  ConsumerInstance->RewriteHelper->replaceRecordType(RTLoc, 
    ConsumerInstance->ReplacingRecordDecl->getNameAsString());
  return true;
}

bool SimplifyStructRewriteVisitor::VisitMemberExpr(MemberExpr *ME)
{
  ValueDecl *OrigDecl = ME->getMemberDecl();
  FieldDecl *FD = dyn_cast<FieldDecl>(OrigDecl);

  if (!FD) {
    // in C++, getMemberDecl returns a CXXMethodDecl.
    if (TransformationManager::isCXXLangOpt())
      return true;
    TransAssert(0 && "Bad FD!\n");
  }

  RecordDecl *RD = FD->getParent();
  if (!RD || (dyn_cast<RecordDecl>(RD->getCanonicalDecl()) != 
              ConsumerInstance->TheRecordDecl))
    return true;

  const Type *T = FD->getType().getTypePtr();
  const RecordType *RT = T->getAs<RecordType>();
  TransAssert(RT && "Invalid record type!");
  const RecordDecl *ReplacingRD = 
    dyn_cast<RecordDecl>(RT->getDecl()->getCanonicalDecl());
  (void)ReplacingRD; 
  TransAssert((ReplacingRD == ConsumerInstance->ReplacingRecordDecl) && 
    "Unmatched Replacing RD!");

  SourceLocation LocEnd = ME->getLocEnd();
  if (LocEnd.isMacroID()) {
    LocEnd = ConsumerInstance->SrcManager->getSpellingLoc(LocEnd);
  }
  SourceLocation ArrowPos = 
      Lexer::findLocationAfterToken(LocEnd,
                                    tok::arrow,
                                    *(ConsumerInstance->SrcManager),
                                    ConsumerInstance->Context->getLangOpts(),
                                    /*SkipTrailingWhitespaceAndNewLine=*/true);
  SourceLocation PeriodPos = 
      Lexer::findLocationAfterToken(LocEnd,
                                    tok::period,
                                    *(ConsumerInstance->SrcManager),
                                    ConsumerInstance->Context->getLangOpts(),
                                    /*SkipTrailingWhitespaceAndNewLine=*/true);

  std::string ES;
  ConsumerInstance->RewriteHelper->getExprString(ME, ES);

  // no more MemberExpr upon this ME
  if (ArrowPos.isInvalid() && PeriodPos.isInvalid()) {
    SourceLocation StartLoc = ME->getLocStart();
    size_t Pos;

    if (ME->isArrow()) {
      Pos = ES.find("->");
    }
    else {
      Pos = ES.find(".");
    }
    TransAssert((Pos != std::string::npos) && "Cannot find arrow or dot!");
    StartLoc = StartLoc.getLocWithOffset(Pos);

    int Off = ES.length() - Pos;
    ConsumerInstance->TheRewriter.RemoveText(StartLoc, Off);
    return true;
  }

  SourceLocation StartLoc = ME->getMemberLoc();
  const char *StartBuf = 
    ConsumerInstance->SrcManager->getCharacterData(StartLoc);
  const char *EndBuf;
  if (ArrowPos.isValid()) {
    EndBuf = ConsumerInstance->SrcManager->getCharacterData(ArrowPos);
    EndBuf++;
  }
  else {
    TransAssert(PeriodPos.isValid() && "Bad dot position!");
    EndBuf = ConsumerInstance->SrcManager->getCharacterData(PeriodPos);
  }
  int Off = EndBuf - StartBuf;
  ConsumerInstance->TheRewriter.RemoveText(StartLoc, Off);
  return true;
}

void SimplifyStruct::Initialize(ASTContext &context) 
{
  Transformation::Initialize(context);
  CollectionVisitor = new SimplifyStructCollectionVisitor(this);
  RewriteVisitor = new SimplifyStructRewriteVisitor(this);
}

void SimplifyStruct::HandleTranslationUnit(ASTContext &Ctx)
{
  // ISSUE: not well tested on CXX code, so currently disable this pass for CXX
  if (TransformationManager::isCXXLangOpt()) {
    ValidInstanceNum = 0;
    TransError = TransMaxInstanceError;
    return;
  }

  TransAssert(CollectionVisitor && "NULL CollectionVisitor!");
  CollectionVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());
  if (QueryInstanceOnly) {
    return;
  }

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  TransAssert(RewriteVisitor && "NULL RewriteVisitor!");
  TransAssert(TheRecordDecl && "NULL TheRecordDecl!");
  TransAssert(ReplacingRecordDecl && "NULL ReplacingRecordDecl!");
  RewriteVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void SimplifyStruct::setQualifierFlags(const FieldDecl *FD)
{
  QualType QT = FD->getType();
  if (QT.isConstQualified())
    ConstField = true;
  if (QT.isVolatileQualified())
    VolatileField = true;
}

SimplifyStruct::~SimplifyStruct(void)
{
  delete CollectionVisitor;
  delete RewriteVisitor;
}

