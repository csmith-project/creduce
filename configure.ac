## -*- mode: Autoconf -*-
##
## Copyright (c) 2012, 2013, 2014, 2015, 2016, 2017, 2018 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

m4_include([version.m4])
AC_INIT(creduce, CREDUCE_VERSION, CREDUCE_BUGREPORT, creduce, CREDUCE_URL)

AC_CONFIG_SRCDIR(clang_delta/ClangDelta.cpp)
AC_CONFIG_AUX_DIR(autoconf)
AC_CONFIG_MACRO_DIR(m4)
AC_CONFIG_HEADERS(config.h)

AC_CANONICAL_TARGET

# `-Wno-portability': avoid warnings about using `:=' in `Makefile.am' files.
AM_INIT_AUTOMAKE([-Wall -Wno-portability])

AM_MAINTAINER_MODE

# Avoid configure-time warnings about `--datarootdir' being ignored.
# This can perhaps be removed in a few years after Autoconf 2.60.
# See <http://www.gnu.org/software/libtool/manual/autoconf/Changed-Directory-Variables.html>
#
AC_DEFUN([AC_DATAROOTDIR_CHECKED])

###############################################################################
## clang_delta

dnl AC_PROG_CC # Automake 1.11 want us to use AM_PROG_CC_C_O instead...
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_LIBTOOL

AX_LLVM([6.0],[engine])
AX_CLANG

# Handle configure-time choice of assertion-checking method.
#
AC_ARG_ENABLE([trans-assert],
  AS_HELP_STRING([--enable-trans-assert],
    [use assert() in clang_delta transformations [default=yes]]),
  [],
  [enable_trans_assert=yes])
AS_IF([test "x$enable_trans_assert" = "xyes"],
  AC_DEFINE([ENABLE_TRANS_ASSERT], [1],
            [Use assert() in clang_delta transformations.]))

# If we are using g++ or clang++, set the default `CXXFLAGS' to something more
# pedantic.  (Both g++ and clang++ cause `GXX' to be set.)
#
# `-fno-strict-aliasing' quiets many warnings about LLVM headers and possibly
#   improves our chances at working code.
# `-Wno-unused-parameter' quiets many warnings about LLVM headers.
# `-Wno-missing-field-initializers' quiets many warnings about LLVM headers.
#
if test "$ac_test_CXXFLAGS" = set; then
  # The user specified `CXXFLAGS', so do nothing.
  CXXFLAGS="$CXXFLAGS"
elif test "$GXX" = yes; then
  CXXFLAGS="-g -O3 -fno-strict-aliasing -Wall -Wextra -Wno-long-long -Wno-unused-parameter -Wno-missing-field-initializers"
fi

###############################################################################
## clex

AM_PROG_LEX

###############################################################################
## creduce

AC_ARG_VAR([PERL], [Perl interpreter])
AC_PATH_PROG([PERL], [perl], [no])
AS_IF([test "x$PERL" = "xno"],
  AC_MSG_ERROR([a Perl interpreter was not found]))
AX_PROG_PERL_VERSION([5.10.0],
  [],
  AC_MSG_ERROR([C-Reduce requires Perl 5.10.0 or later]))

# Check for the run-time prerequisites of C-Reduce.  We only warn the user when
# these are not found at configure time.  Let the user install the dependencies
# later.
#
missing_required_runtime_prereq=no
missing_optional_runtime_prereq=no
#
# creduce/creduce.in
#
AX_PROG_PERL_MODULES(
  [
    Exporter::Lite
    File::Which
    Getopt::Tabular
    Regexp::Common
  ],
  [
    REQUIRED_PERL_MODULES=yes
  ],
  [
    REQUIRED_PERL_MODULES=no
    missing_required_runtime_prereq=yes
  ])
AX_PROG_PERL_MODULES(
  [
    Term::ReadKey
  ],
  [
    OPTIONAL_PERL_MODULES=yes
  ],
  [
    OPTIONAL_PERL_MODULES=no
    missing_optional_runtime_prereq=yes
  ])
#
# creduce/pass_indent.pm
#
AC_ARG_VAR([CLANG_FORMAT], [`clang-format' (LLVM/Clang) program])
AC_PATH_PROG([CLANG_FORMAT], [clang-format], [no], [$LLVM_BINDIR:$PATH])
AS_IF([test "x$CLANG_FORMAT" = "xno"], [missing_required_runtime_prereq=yes])

###############################################################################

AC_OUTPUT(
  Makefile
  clang_delta/Makefile
  clex/Makefile
  creduce/Makefile
  delta/Makefile
  scripts/Makefile
  tests/Makefile
  unifdef/Makefile
)

###############################################################################

# Warn about things that will be needed at run time, but which we did not find
# at configure time.
#
# First warn about required run-time dependencies...
#
AS_IF([test "$missing_required_runtime_prereq" = "yes"],
  AS_BOX([WARNING: You cannot run C-Reduce until you install missing dependencies!]))

AS_IF([test "$REQUIRED_PERL_MODULES" = "no"],
  [
    AC_MSG_WARN([You must install Perl modules required by C-Reduce.])
  ])
AS_IF([test "x$CLANG_FORMAT" = "xno"],
  [
    AC_MSG_WARN([You must install `clang-format' before running C-Reduce.])
    CLANG_FORMAT=clang-format
  ])

AS_IF([test "$missing_required_runtime_prereq" = "yes"],
  AC_MSG_WARN([Read the INSTALL file for info about C-Reduce dependencies.]))

# ...and then warn about optional run-time dependencies.
#
AS_IF([test "$missing_optional_runtime_prereq" = "yes"],
  AS_BOX([NOTE: The following optional C-Reduce dependencies were not found.      ]))

AS_IF([test "$OPTIONAL_PERL_MODULES" = "no"],
  [
    AC_MSG_NOTICE([One or more useful Perl modules were not found.])
  ])

AS_IF([test "$missing_optional_runtime_prereq" = "yes"],
  AC_MSG_NOTICE([C-Reduce will use these tools if you install them in the future.]))
AS_IF([test "$missing_optional_runtime_prereq" = "yes"],
  AC_MSG_NOTICE([Read the INSTALL file for info about C-Reduce dependencies.]))

###############################################################################

## End of file.
