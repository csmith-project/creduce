# -*- mode: m4 -*-
#
# Copyright (c) 2012, 2013 The University of Utah
# Copyright (c) 2008 Andy Kitchen <agimbleinthewabe@gmail.com>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty provided the copyright notice
# and this notice are preserved.  This file is offered as-is, without any
# warranty.

###############################################################################

# SYNOPSIS
#
#   AX_LLVM([version],[llvm-libs])
#
# DESCRIPTION
#
#   Test for the existence of LLVM, test that LLVM is at least the specified
#   version, and test that a simple test program can be linked with the
#   libraries described in the llvm-libs argument, i.e.:
#
#     llvm --libs <llvm-libs>
#
#   If the <version> argument is the empty string, then the version check is
#   bypassed.
#
# This file was derived from the LLVM Autoconf macro found in the Autoconf
# Macro Archive: <http://www.gnu.org/software/autoconf-archive/ax_llvm.html>.
# The definition of AX_LLVM in this file is almost completely rewritten from
# the version (serial #12) found in the Archive.
#
# The current file has been updated for modern LLVM (3.3).

AC_DEFUN([AX_LLVM],
[
  AC_ARG_WITH([llvm],
    AS_HELP_STRING([--with-llvm@<:@=DIR@:>@],
      [use LLVM located in DIR]),
    [with_llvm="$withval"],
    [with_llvm=yes])

  if test "x$with_llvm" = "xno"; then
    AC_MSG_ERROR(
      [--with-llvm=no was given but this package requires LLVM])
  elif test "x$with_llvm" = "xyes"; then
    with_llvm_path="$PATH"
  else
    with_llvm_path="$with_llvm/bin$PATH_SEPARATOR$with_llvm"
  fi

  AC_PATH_PROG([LLVM_CONFIG], [llvm-config], [], [$with_llvm_path])
  if test -z "$LLVM_CONFIG"; then
    AC_MSG_ERROR(
      [LLVM is required but program `llvm-config' cannot be found in $with_llvm_path])
  fi

  if test -n "$1"; then
    AC_MSG_CHECKING([for LLVM version])
    LLVM_VERSION=`$LLVM_CONFIG --version`
    AC_MSG_RESULT([$LLVM_VERSION])
    AX_COMPARE_VERSION([$LLVM_VERSION],[ge],[$1],
      [],
      [
        AC_MSG_ERROR(
          [LLVM version $1 or later is required])
      ])
  fi

  LLVM_BINDIR=`$LLVM_CONFIG --bindir`
  LLVM_CPPFLAGS=`$LLVM_CONFIG --cxxflags`
  LLVM_LDFLAGS=`$LLVM_CONFIG --ldflags`
  LLVM_LIBS=`$LLVM_CONFIG --libs $2`

  # The output of `llvm-config --ldflags' often contains library directives
  # that must come *after* all the LLVM libraries on the link line: e.g.,
  # "-lpthread -lffi -ldl -lm".  To ensure this, we insert LLVM_LDFLAGS into
  # LIBS, *not* into LDFLAGS.

  AC_REQUIRE([AC_PROG_CXX])
  CPPFLAGS_SAVED="$CPPFLAGS"
  CPPFLAGS="$CPPFLAGS $LLVM_CPPFLAGS"
  LDFLAGS_SAVED="$LDFLAGS"
  LDFLAGS="$LDFLAGS"
  # LDFLAGS="$LDFLAGS $LLVM_LDFLAGS" --- see comment above.
  LIBS_SAVED="$LIBS"
  LIBS="$LIBS $LLVM_LIBS $LLVM_LDFLAGS"
  # LIBS="$LIBS $LLVM_LIBS" --- see comment above.

  AC_CACHE_CHECK(can compile with and link with LLVM([$2]),
    ax_cv_llvm,
    [
      AC_LANG_PUSH([C++])
      AC_LINK_IFELSE([
        AC_LANG_PROGRAM(
          [[@%:@include <llvm/IR/LLVMContext.h>
@%:@include <llvm/IR/Module.h>]],
          [[llvm::LLVMContext context;
llvm::Module *M = new llvm::Module("test", context);]])],
        ax_cv_llvm=yes,
        ax_cv_llvm=no)
      AC_LANG_POP([C++])
    ])

  CPPFLAGS="$CPPFLAGS_SAVED"
  LDFLAGS="$LDFLAGS_SAVED"
  LIBS="$LIBS_SAVED"

  if test "$ax_cv_llvm" != "yes"; then
    AC_MSG_FAILURE(
      [cannot compile and link test program with selected LLVM])
  fi
  AC_SUBST(LLVM_BINDIR)
])
