## -*- mode: Makefile-Automake -*-
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

# The funny quoting in `LLVMLIBS' avoids a warning from Automake:
#   ...: linker flags such as `--libs' belong in `clang_delta_LDFLAGS'
# The warning comes about because Automake doesn't understand $(shell).
# Avoid the warning by making `--ldflags' and `--libs' not look like
# linker options.
#
LLVMCXXFLAGS	:= $(shell "$(LLVM_CONFIG)" --cxxflags)
LLVMLDFLAGS	:= $(shell "$(LLVM_CONFIG)" \--ldflags)
LLVMINCLUDEDIR	:= $(shell "$(LLVM_CONFIG)" --includedir)
LLVMLIBS	:= $(shell "$(LLVM_CONFIG)" \--libs)

clang_delta_CPPFLAGS = \
	$(GIT_FLAG) \
	-I"$(LLVMINCLUDEDIR)/clang"

clang_delta_CXXFLAGS = \
	$(LLVMCXXFLAGS)

# See comment below about `clang_delta_LDADD' and `LLVMLDFLAGS'.
# clang_delta_LDFLAGS = \
#	$(LLVMLDFLAGS)

# Try to do the "right thing" by putting these in `clang_delta_LDADD' instead
# of `clang_delta_LDFLAGS'.  This leads to the funny escape in `LLVMLIBS',
# above, and the need to define `clang_delta_DEPENDENCIES', below.
#
# The output of `llvm-config --ldflags' often contains library directives that
# must come *after* all the LLVM/Clang libraries on the link line: e.g.,
# "-lpthread -lffi -ldl -lm".  The easiest way to get these into the right
# place is to add `LLVMLDFLAGS' to `clang_delta_LDADD' --- *not* to
# `clang_delta_LDFLAGS'.  Automake puts LDFLAGS early in the link line.
#
clang_delta_LDADD = \
	-lclangFrontendTool -lclangFrontend -lclangDriver -lclangSerialization \
	-lclangCodeGen -lclangParse -lclangSema -lclangAnalysis \
	-lclangRewrite -lclangAST -lclangLex -lclangBasic -lclangEdit \
	$(LLVMLIBS) \
	$(LLVMLDFLAGS)

# Automake doesn't grok our use of $(shell) in `clang_delta_LDADD', so it puts
# $(LLVMLIBS) in the dependencies by default.  To avoid this problem, we must
# define `clang_delta_DEPENDENCIES' explicitly.
#
clang_delta_DEPENDENCIES =

clang_delta_SOURCES = \
	AggregateToScalar.cpp \
	AggregateToScalar.h \
	BinOpSimplification.cpp \
	BinOpSimplification.h \
	CallExprToValue.cpp \
	CallExprToValue.h \
	ClangDelta.cpp \
	ClassTemplateToClass.cpp \
	ClassTemplateToClass.h \
	CombineGlobalVarDecl.cpp \
	CombineGlobalVarDecl.h \
	CombineLocalVarDecl.cpp \
	CombineLocalVarDecl.h \
	CommonParameterRewriteVisitor.h \
	CommonRenameClassRewriteVisitor.h \
	CommonStatementVisitor.h \
	CopyPropagation.cpp \
	CopyPropagation.h \
	EmptyStructToInt.cpp \
	EmptyStructToInt.h \
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
	ReduceClassTemplateParameter.cpp \
	ReduceClassTemplateParameter.h \
	ReducePointerLevel.cpp \
	ReducePointerLevel.h \
	ReducePointerPairs.cpp \
	ReducePointerPairs.h \
	RemoveAddrTaken.cpp \
	RemoveAddrTaken.h \
	RemoveArray.cpp \
	RemoveArray.h \
	RemoveBaseClass.cpp \
	RemoveBaseClass.h \
	RemoveCtorInitializer.cpp \
	RemoveCtorInitializer.h \
	RemoveNamespace.cpp \
	RemoveNamespace.h \
	RemoveNestedFunction.cpp \
	RemoveNestedFunction.h \
	RemovePointer.cpp \
	RemovePointer.h \
	RemoveTrivialBaseTemplate.cpp \
	RemoveTrivialBaseTemplate.h \
	RemoveUnresolvedBase.cpp \
	RemoveUnresolvedBase.h \
	RemoveUnusedFunction.cpp \
	RemoveUnusedFunction.h \
	RemoveUnusedStructField.cpp \
	RemoveUnusedStructField.h \
	RemoveUnusedVar.cpp \
	RemoveUnusedVar.h \
	RenameClass.cpp \
	RenameClass.h \
	RenameFun.cpp \
	RenameFun.h \
	RenameParam.cpp \
	RenameParam.h \
	RenameVar.cpp \
	RenameVar.h \
	ReplaceCallExpr.cpp \
	ReplaceCallExpr.h \
	ReplaceDerivedClass.cpp \
	ReplaceDerivedClass.h \
	ReplaceSimpleTypedef.cpp \
	ReplaceSimpleTypedef.h \
	ReplaceUndefinedFunction.cpp \
	ReplaceUndefinedFunction.h \
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
	SimplifyDependentTypedef.cpp \
	SimplifyDependentTypedef.h \
	SimplifyIf.cpp \
	SimplifyIf.h \
	SimplifyStruct.cpp \
	SimplifyStruct.h \
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

EXTRA_DIST = \
	README.txt \
	test_transformation

###############################################################################

## End of file.
