## -*- mode: Makefile-Automake -*-
##
## Copyright (c) 2012, 2013, 2014, 2015, 2016, 2017, 2018 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

libexec_PROGRAMS = clang_delta

GIT_HASH := $(shell "$(top_srcdir)/git-hash.sh" "$(top_srcdir)" || echo error)

DUMMY1 := $(shell echo "$(GIT_HASH)" > git_version.txt.tmp)
DUMMY2 := $(shell cmp -s git_version.txt.tmp git_version.txt || \
		  cp git_version.txt.tmp git_version.txt)
DUMMY3 := $(shell rm -f git_version.txt.tmp)

git_version_edit = sed \
	-e 's|@GIT_HASH[@]|$(GIT_HASH)|g'

git_version.cpp: git_version.cpp.in git_version.txt
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(git_version_edit) $${srcdir}$@.in > $@.tmp
	mv $@.tmp $@

# llvm-lit is not installed into LLVM_BINDIR. To run `make check-clang-delta',
# we have to manually copy llvm-lit from LLVM's build dir. This is also the
# reason that I didn't create a check-local rule for running clang_delta tests.
# It would fail everywhere else llvm-lit does exist, e.g. travis?
check-clang-delta: tests/lit.site.cfg remove_lit_checks clang_delta
	@ echo "--- Running tests for clang_delta ---"
	@ $(LLVM_BINDIR)/llvm-lit -sv tests

tests/lit.site.cfg: tests/lit.site.cfg.in
	@ echo s=@abs_builddir[@]=$(abs_builddir)=g > lit.site.cfg.tmp
	@ echo s=@LLVM_BINDIR[@]=$(LLVM_BINDIR)=g >> lit.site.cfg.tmp
	@ echo s=@clang_delta_srcdir@=$(abs_srcdir)=g >> lit.site.cfg.tmp
	@ sed -f lit.site.cfg.tmp tests/lit.site.cfg.in > $@
	@ rm -f lit.site.cfg.tmp

remove_lit_checks: tests/remove_lit_checks.in
	@ echo s=@PERL_EXECUTABLE[@]=$(PERL)=g > remove_lit_checks.tmp
	@ sed -f remove_lit_checks.tmp tests/remove_lit_checks.in > tests/$@
	@ chmod +x tests/$@
	@ rm -f remove_lit_checks.tmp

# When setting `LLVMCXXFLAGS', we weed out command-line options that might be
# troublesome (e.g., -W/-f options that are not supported by `CXX', or -W
# options that turn warnings into errors).  We also weed out options that might
# override choices that *we* want to control (e.g., debug and optimization
# options).
#
# The subparts of the `grep' invocation below remove compiler command-line
# options of the following forms:
#   -W...                --- warning options
#   -w                   --- inhibits all warnings
#   -pedantic...         --- pedantic warning options
#   -f...diagnostics...  --- diagnostics reporting options (GCC, Clang)
#   -f...show...         --- diagnostics reporting options (Clang)
#   -g...                --- debugging options
#   -O...                --- optimization options
# [And for the llvm.org-provided binary of Clang 3.8.0 for OS X, weed out:]
#   -isysroot
#   /Applications/Xcode.app/...
#
# The `tr/sed | grep | xargs' pipeline is intended to be portable.  We use
# `grep' for matching because writing fancy, portable `sed' expressions is
# difficult.  For example, some implementations use "\b" to match word
# boundaries while others use "[[:<:]]" and "[[:>:]]".  The Autoconf
# documentation says that anchored matches in `sed' are not portable.  Give up;
# use `grep' instead.  Bonus: better readability!
#
# The funny quoting in `LLVMLIBS' avoids a warning from Automake:
#   ...: linker flags such as `--libs' belong in `clang_delta_LDFLAGS'
# The warning comes about because Automake doesn't understand $(shell).
# Avoid the warning by making `--ldflags' and `--libs' not look like
# linker options.
#
LLVMCXXFLAGS	:= $(shell \
		     "$(LLVM_CONFIG)" --cxxflags | \
		     tr '\t' ' ' | sed -e 's/  */ /g' | tr ' ' '\n' | \
		     grep -v -e '^-W' \
			     -e '^-w$$' \
			     -e '^-pedantic' \
			     -e '^-f[a-z-]*diagnostics' \
			     -e '^-f[a-z-]*show' \
			     -e '^-g' \
			     -e '^-O$$' \
			     -e '^-O[0-9s]' \
			     -e '^-isysroot$$' \
			     -e '^/Applications/Xcode\.app/.*\.sdk$$' | \
		     xargs) \
                   -fno-rtti -fno-exceptions
LLVMLDFLAGS	:= $(shell "$(LLVM_CONFIG)" \--ldflags)
LLVMINCLUDEDIR	:= $(shell "$(LLVM_CONFIG)" --includedir)
LLVMLIBS	:= $(shell "$(LLVM_CONFIG)" \--libs) \
		   $(shell "$(LLVM_CONFIG)" \--system-libs)

clang_delta_CPPFLAGS = \
	$(CLANG_CPPFLAGS) \
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
# Pre LLVM-3.5, the output of `llvm-config --ldflags' would often contain
# library directives that must come *after* all the LLVM/Clang libraries on the
# link line: e.g., "-lpthread -lffi -ldl -lm".  The easiest way to get these
# into the right place is to add `LLVMLDFLAGS' to `clang_delta_LDADD' --- *not*
# to `clang_delta_LDFLAGS'.  Automake puts LDFLAGS early in the link line.
#
# Newer LLVM's (post-3.4) support `llvm-config --system-libs', which will
# possibly help us to straighten this out in the future.
#
# In LLVM 3.5, `llvm-config --ldflags' does not contain the list of system
# libraries.  So probably, we could move `LLVMLDFLAGS' back to the definition
# of `clang_delta_LDFLAGS'.  I'll do that later.
clang_delta_LDADD = \
	-lclangStaticAnalyzerFrontend -lclangStaticAnalyzerCheckers \
	-lclangStaticAnalyzerCore \
	-lclangFrontendTool -lclangFrontend -lclangDriver -lclangSerialization \
	-lclangCodeGen -lclangParse -lclangSema -lclangAnalysis \
	-lclangRewriteFrontend -lclangRewrite -lclangAST -lclangBasic -lclangEdit -lclangLex \
	-lclangARCMigrate \
	$(LLVMLIBS) \
	$(CLANG_LDFLAGS) \
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
	CommonTemplateArgumentVisitor.h \
	CopyPropagation.cpp \
	CopyPropagation.h \
	EmptyStructToInt.cpp \
	EmptyStructToInt.h \
	ExpressionDetector.cpp \
	ExpressionDetector.h \
	InstantiateTemplateParam.cpp \
	InstantiateTemplateParam.h \
	InstantiateTemplateTypeParamToInt.cpp \
	InstantiateTemplateTypeParamToInt.h \
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
	RemoveEnumMemberValue.cpp \
	RemoveEnumMemberValue.h \
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
	RemoveUnusedEnumMember.cpp \
	RemoveUnusedEnumMember.h \
	RemoveUnusedFunction.cpp \
	RemoveUnusedFunction.h \
	RemoveUnusedOuterClass.cpp \
	RemoveUnusedOuterClass.h \
	RemoveUnusedStructField.cpp \
	RemoveUnusedStructField.h \
	RemoveUnusedVar.cpp \
	RemoveUnusedVar.h \
	RenameCXXMethod.cpp \
	RenameCXXMethod.h \
	RenameClass.cpp \
	RenameClass.h \
	RenameFun.cpp \
	RenameFun.h \
	RenameParam.cpp \
	RenameParam.h \
	RenameVar.cpp \
	RenameVar.h \
	ReplaceArrayAccessWithIndex.cpp \
	ReplaceArrayAccessWithIndex.h \
	ReplaceArrayIndexVar.cpp \
	ReplaceArrayIndexVar.h \
	ReplaceCallExpr.cpp \
	ReplaceCallExpr.h \
	ReplaceClassWithBaseTemplateSpec.cpp \
	ReplaceClassWithBaseTemplateSpec.h \
	ReplaceDependentName.cpp \
	ReplaceDependentName.h \
	ReplaceDependentTypedef.cpp \
	ReplaceDependentTypedef.h \
	ReplaceDerivedClass.cpp \
	ReplaceDerivedClass.h \
	ReplaceFunctionDefWithDecl.cpp \
	ReplaceFunctionDefWithDecl.h \
	ReplaceOneLevelTypedefType.cpp \
	ReplaceOneLevelTypedefType.h \
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
	SimplifyNestedClass.cpp \
	SimplifyNestedClass.h \
	SimplifyRecursiveTemplateInstantiation.cpp \
	SimplifyRecursiveTemplateInstantiation.h \
	SimplifyStruct.cpp \
	SimplifyStruct.h \
	SimplifyStructUnionDecl.cpp \
	SimplifyStructUnionDecl.h \
	TemplateArgToInt.cpp \
	TemplateArgToInt.h \
	TemplateNonTypeArgToInt.cpp \
	TemplateNonTypeArgToInt.h \
	Transformation.cpp \
	Transformation.h \
	TransformationManager.cpp \
	TransformationManager.h \
	UnifyFunctionDecl.cpp \
	UnifyFunctionDecl.h \
	UnionToStruct.cpp \
	UnionToStruct.h \
	VectorToArray.cpp \
	VectorToArray.h \
	git_version.h

nodist_clang_delta_SOURCES = \
	git_version.cpp

CLEANFILES = \
	git_version.txt \
	git_version.txt.tmp \
	git_version.cpp \
	git_version.cpp.tmp \
	tests/lit.site.cfg \
	tests/remove_lit_checks

EXTRA_DIST = \
	CMakeLists.txt \
	README.txt \
	git_version.cpp.in \
	test_transformation \
	tests/lit.cfg \
	tests/lit.site.cfg.in \
	tests/remove-unused-var/unused_var.cpp \
	tests/remove_lit_checks.in \
	tests/rename-var/rename-var.c

CLEAN_TESTDIRS = $(sort $(dir $(wildcard tests/*/Output/.)))

clean-local:
	list='$(CLEAN_TESTDIRS)'; \
	for dir in $$list; do \
	  if test -d "$$dir"; then \
	    rm -rf $$dir; \
	  fi; \
	done

###############################################################################

## End of file.
