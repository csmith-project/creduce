## -*- mode: Makefile-Automake -*-
##
## Copyright (c) 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020 The University of Utah
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
check-clang-delta: tests/lit.site.cfg tests/remove_lit_checks clang_delta
	@ echo "--- Running tests for clang_delta ---"
	@ $(LLVM_BINDIR)/llvm-lit -sv tests

tests/lit.site.cfg: tests/lit.site.cfg.in
	@ echo s=@abs_builddir[@]=$(abs_builddir)=g > lit.site.cfg.tmp
	@ echo s=@LLVM_BINDIR[@]=$(LLVM_BINDIR)=g >> lit.site.cfg.tmp
	@ echo s=@clang_delta_srcdir@=$(abs_srcdir)=g >> lit.site.cfg.tmp
	@ sed -f lit.site.cfg.tmp $< > $@
	@ rm -f lit.site.cfg.tmp

tests/remove_lit_checks: tests/remove_lit_checks.in
	@ echo s=@PERL_EXECUTABLE[@]=$(PERL)=g > remove_lit_checks.tmp
	@ sed -f remove_lit_checks.tmp $< > $@
	@ chmod +x $@
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
	-lclang-cpp \
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
	tests/aggregate-to-scalar/cast.c \
	tests/aggregate-to-scalar/test1.c \
	tests/aggregate-to-scalar/test1.cc \
	tests/aggregate-to-scalar/test2.c \
	tests/aggregate-to-scalar/test2.cc \
	tests/aggregate-to-scalar/test3.c \
	tests/aggregate-to-scalar/test4.c \
	tests/callexpr-to-value/macro1.c \
	tests/callexpr-to-value/macro2.c \
	tests/callexpr-to-value/test1.c \
	tests/callexpr-to-value/test2.c \
	tests/copy-propagation/copy1.cpp \
	tests/copy-propagation/copy2.cpp \
	tests/empty-struct-to-int/empty-struct.cpp \
	tests/empty-struct-to-int/empty-struct2.cpp \
	tests/empty-struct-to-int/empty-struct3.cpp \
	tests/empty-struct-to-int/empty-struct4.cpp \
	tests/empty-struct-to-int/empty-struct5.cpp \
	tests/empty-struct-to-int/empty-struct6.c \
	tests/empty-struct-to-int/empty-struct7.c \
	tests/empty-struct-to-int/struct_int.c \
	tests/empty-struct-to-int/test1.cc \
	tests/empty-struct-to-int/test2.cc \
	tests/empty-struct-to-int/test3.c \
	tests/lit.cfg \
	tests/lit.site.cfg.in \
	tests/local-to-global/macro.c \
	tests/local-to-global/unnamed_1.c \
	tests/local-to-global/unnamed_2.c \
	tests/local-to-global/unnamed_3.c \
	tests/param-to-global/macro.c \
	tests/reduce-array-dim/non-type-temp-arg.cpp \
	tests/reduce-pointer-level/scalar-init-expr.cpp \
	tests/remove-enum-member-value/builtin_macro.c \
	tests/remove-namespace/macro.cpp \
	tests/remove-namespace/namespace.cpp \
	tests/remove-namespace/namespace10.cpp \
	tests/remove-namespace/namespace11.cpp \
	tests/remove-namespace/namespace12.cpp \
	tests/remove-namespace/namespace13.cpp \
	tests/remove-namespace/namespace14.cpp \
	tests/remove-namespace/namespace15.cpp \
	tests/remove-namespace/namespace2.cpp \
	tests/remove-namespace/namespace3.cpp \
	tests/remove-namespace/namespace4.cpp \
	tests/remove-namespace/namespace5.cpp \
	tests/remove-namespace/namespace6.cpp \
	tests/remove-namespace/namespace7.cpp \
	tests/remove-namespace/namespace8.cpp \
	tests/remove-namespace/namespace9.cpp \
	tests/remove-nested-function/remove_nested_func1.cc \
	tests/remove-unused-field/designated1.c \
	tests/remove-unused-field/designated2.c \
	tests/remove-unused-field/designated3.c \
	tests/remove-unused-field/designated4.c \
	tests/remove-unused-field/designated5.c \
	tests/remove-unused-field/unused_field1.c \
	tests/remove-unused-field/unused_field2.c \
	tests/remove-unused-field/unused_field3.cpp \
	tests/remove-unused-function/class.cc \
	tests/remove-unused-function/const.cc \
	tests/remove-unused-function/default.cc \
	tests/remove-unused-function/delete.cc \
	tests/remove-unused-function/delete2.cc \
	tests/remove-unused-function/inline_ns.cc \
	tests/remove-unused-function/macro1.cc \
	tests/remove-unused-function/macro2.cc \
	tests/remove-unused-function/macro3.cc \
	tests/remove-unused-function/template1.cc \
	tests/remove-unused-function/template2.cc \
	tests/remove-unused-function/unused-funcs.cc \
	tests/remove-unused-var/struct1.c \
	tests/remove-unused-var/struct2.c \
	tests/remove-unused-var/unused_var.cpp \
	tests/remove_lit_checks.in \
	tests/rename-class/base_specifier.cpp \
	tests/rename-class/bool.cc \
	tests/rename-class/class_template.cc \
	tests/rename-class/class_template2.cc \
	tests/rename-class/dependent.cpp \
	tests/rename-class/dependent_name.cpp \
	tests/rename-class/derive.cc \
	tests/rename-class/dtor.cc \
	tests/rename-class/dtor1.cc \
	tests/rename-class/elaborated_type1.cpp \
	tests/rename-class/elaborated_type2.cpp \
	tests/rename-class/explicit_specialization.cpp \
	tests/rename-class/forward_decl.cc \
	tests/rename-class/injected_name.cpp \
	tests/rename-class/instantiation.cpp \
	tests/rename-class/parm.cpp \
	tests/rename-class/partial_specialization.cpp \
	tests/rename-class/rename-class1.cpp \
	tests/rename-class/rename-class2.cpp \
	tests/rename-class/specialization.cpp \
	tests/rename-class/static_member.cc \
	tests/rename-class/template_class_1.cpp \
	tests/rename-class/template_parm.cpp \
	tests/rename-class/template_template.cpp \
	tests/rename-class/template_template_parm.cpp \
	tests/rename-class/typedef.cpp \
	tests/rename-class/typedef2.cc \
	tests/rename-class/using.cpp \
	tests/rename-class/using1.cc \
	tests/rename-cxx-method/overloaded.cc \
	tests/rename-cxx-method/test1.cc \
	tests/rename-cxx-method/test2.cc \
	tests/rename-cxx-method/test3.cc \
	tests/rename-fun/func_templ.cc \
	tests/rename-fun/overloaded.cc \
	tests/rename-fun/test1.c \
	tests/rename-fun/test1.h \
	tests/rename-param/invalid.c \
	tests/rename-var/rename-var.c \
	tests/replace-derived-class/replace-derived1.cpp \
	tests/replace-derived-class/replace-derived2.cpp \
	tests/replace-derived-class/replace-derived3.cpp \
	tests/replace-derived-class/replace-derived4.cpp \
	tests/replace-function-def-with-decl/macro1.c \
	tests/replace-function-def-with-decl/macro2.c \
	tests/return-void/test1.c \
	tests/return-void/test1.cc \
	tests/return-void/test2.c \
	tests/return-void/test3.c \
	tests/return-void/test4.c \
	tests/return-void/test5.c \
	tests/return-void/test6.c \
	tests/simplify-callexpr/macro.c \
	tests/simplify-callexpr/test.c \
	tests/simplify-callexpr/test2.c \
	tests/simplify-if/macro.c \
	tests/union-to-struct/union1.c \
	tests/union-to-struct/union2.c \
	tests/union-to-struct/union3.c

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
