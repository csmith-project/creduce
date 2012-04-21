## -*- mode: Makefile.am -*-
##
## Copyright (c) 2012 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

libexec_PROGRAMS = clang_delta

GIT_HASH := $(shell "$(top_srcdir)/git-hash.sh" "$(top_srcdir)" || echo error)
GIT_FLAG = -DGIT_VERSION=\"$(GIT_HASH)\"

LLVMCONFIG	:= $(LLVM_BINDIR)/llvm-config
LLVMCXXFLAGS	:= $(shell "$(LLVMCONFIG)" --cxxflags)
LLVMLDFLAGS	:= $(shell "$(LLVMCONFIG)" --ldflags)
LLVMINCLUDEDIR	:= $(shell "$(LLVMCONFIG)" --includedir)
LLVMLIBS	:= $(shell "$(LLVMCONFIG)" --libs engine)

clang_delta_CPPFLAGS = \
	$(GIT_FLAG) \
	-I"$(LLVMINCLUDEDIR)/clang"

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
