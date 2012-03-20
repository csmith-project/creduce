## -*- mode: Makefile.am -*-
##
## Copyright (c) 2012 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

bin_PROGRAMS = clang_delta

GIT_HASH := $(shell "$(top_srcdir)/git-hash.sh" "$(top_srcdir)" || echo error)
GIT_FLAG = -DGIT_VERSION=\"$(GIT_HASH)\"

ENABLE_TRANS_ASSERT = -DENABLE_TRANS_ASSERT
# ifeq ($(DISABLE_TRANS_ASSERT), 1)
#   ENABLE_TRANS_ASSERT = -DDISABLE_TRANS_ASSERT
# endif

LLVMCXXFLAGS = `$(LLVM_PATH)/bin/llvm-config --cxxflags`
LLVMLDFLAGS = `$(LLVM_PATH)/bin/llvm-config --ldflags`
LLVMLIBS = `$(LLVM_PATH)/bin/llvm-config --libs | tr " " "\n" | grep -v gtest`

clang_delta_CPPFLAGS = \
	$(GIT_FLAG) \
	$(ENABLE_TRANS_ASSERT) \
	-I"$(LLVM_PATH)/include/clang/"

clang_delta_CXXFLAGS = \
	$(LLVMCXXFLAGS)

clang_delta_LDFLAGS = \
	$(LLVMLDFLAGS) \
	-lclangFrontendTool -lclangFrontend -lclangDriver -lclangSerialization \
	-lclangCodeGen -lclangParse -lclangSema -lclangAnalysis \
	-lclangIndex -lclangRewrite -lclangAST -lclangLex -lclangBasic \
	-lclangEdit \
	$(LLVMLIBS) \
	-ldl

clang_delta_SOURCES = \
	AggregateToScalar.cpp \
	AggregateToScalar.h \
	BinOpSimplification.cpp \
	BinOpSimplification.h \
	CallExprToValue.cpp \
	CallExprToValue.h \
	ClangDelta.cpp \
	CombineGlobalVarDecl.cpp \
	CombineGlobalVarDecl.h \
	CombineLocalVarDecl.cpp \
	CombineLocalVarDecl.h \
	CopyPropagation.cpp \
	CopyPropagation.h \
	LiftAssignmentExpr.cpp \
	LiftAssignmentExpr.h \
	LocalToGlobal.cpp \
	LocalToGlobal.h \
	MoveFunctionBody.cpp \
	MoveFunctionBody.h \
	MoveGlobalVar.cpp \
	MoveGlobalVar.h \
	ParamToGlobal.cpp \
	ParamToGlobal.h \
	ParamToLocal.cpp \
	ParamToLocal.h \
	ReduceArrayDim.cpp \
	ReduceArrayDim.h \
	ReduceArraySize.cpp \
	ReduceArraySize.h \
	ReducePointerLevel.cpp \
	ReducePointerLevel.h \
	RemoveNestedFunction.cpp \
	RemoveNestedFunction.h \
	RemoveUnusedFunction.cpp \
	RemoveUnusedFunction.h \
	RemoveUnusedStructField.cpp \
	RemoveUnusedStructField.h \
	RemoveUnusedVar.cpp \
	RemoveUnusedVar.h \
	RenameFun.cpp \
	RenameFun.h \
	RenameParam.cpp \
	RenameParam.h \
	RenameVar.cpp \
	RenameVar.h \
	ReplaceCallExpr.cpp \
	ReplaceCallExpr.h \
	ReturnVoid.cpp \
	ReturnVoid.h \
	RewriteUtils.cpp \
	RewriteUtils.h \
	SimpleInliner.cpp \
	SimpleInliner.h \
	SimplifyCallExpr.cpp \
	SimplifyCallExpr.h \
	SimplifyCommaExpr.cpp \
	SimplifyCommaExpr.h \
	SimplifyIf.cpp \
	SimplifyIf.h \
	SimplifyStructUnionDecl.cpp \
	SimplifyStructUnionDecl.h \
	Transformation.cpp \
	Transformation.h \
	TransformationManager.cpp \
	TransformationManager.h \
	UnifyFunctionDecl.cpp \
	UnifyFunctionDecl.h \
	UnionToStruct.cpp \
	UnionToStruct.h

###############################################################################

## End of file.
