## -*- mode: Autoconf -*-
##
## Copyright (c) 2012 The University of Utah
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

dnl AC_PROG_CC # Automake 1.11 want us to use AM_PROG_CC_C_O instead...
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_LIBTOOL

dnl AX_LLVM

# clang_delta: handle configure-time choice of assertion-checking method.
#
AC_ARG_ENABLE([trans-assert],
  AS_HELP_STRING([--enable-trans-assert],
    [use assert() in clang_delta transformations]))
AS_IF([test "x$enable_trans_assert" = "xyes"],
  AC_DEFINE([ENABLE_TRANS_ASSERT], [1],
            [Use assert() in clang_delta transformations.]))

# If we are using g++ or clang++, set the default `CXXFLAGS' to something more
# pedantic.  (Both g++ and clang++ cause `GXX' to be set.)
#
# `-Wno-unused-parameter' quiets many warnings about LLVM headers.
#
if test "$ac_test_CXXFLAGS" = set; then
  # The user specified `CXXFLAGS', so do nothing.
  CXXFLAGS="$CXXFLAGS"
elif test "$GXX" = yes; then 
  CXXFLAGS="-g -O3 -Wall -Wextra -Wno-long-long -Wno-unused-parameter"
fi

AC_OUTPUT(
  Makefile
  c_reduce/Makefile
  clang_delta/Makefile
  scripts/Makefile
  tests/Makefile
)

###############################################################################

## End of file.
