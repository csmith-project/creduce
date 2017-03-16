# -*- mode: m4 -*-
#
# Copyright (c) 2017 The University of Utah
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty provided the copyright notice
# and this notice are preserved.  This file is offered as-is, without any
# warranty.

###############################################################################

# SYNOPSIS
#
#   AX_CLANG()
#
# DESCRIPTION
#
#   XXX

AC_DEFUN([AX_CLANG],
[
  AC_ARG_WITH([clang],
    AS_HELP_STRING([--with-clang@<:@=DIR@:>@],
      [use Clang development tree located in DIR
       [advanced option;
       default is to find Clang libraries and headers in LLVM tree]]),
    [with_clang="$withval"],
    [with_clang=no])

  CLANG_CPPFLAGS=''
  CLANG_LDFLAGS=''

  if test "x$with_clang" = "xno"; then
    true # Do nothing.
  elif test "x$with_clang" = "xyes"; then
    AC_MSG_ERROR(
      [--with-clang requires an argument (a Clang development tree)])
  else

    clang_dir="$with_clang"
    clang_cmake_file="$clang_dir/lib/cmake/clang/ClangConfig.cmake"
    AC_CHECK_FILE([$clang_cmake_file],
      [],
      [
        AC_MSG_ERROR(
          [argument to --with-clang is not a Clang development tree])
      ])

changequote(<<, >>)dnl
    CLANG_CPPFLAGS=`dnl
      sed -n -E dnl
        -e 's/^set\(CLANG_INCLUDE_DIRS [^"]*"([^"]+)"\)$/\1/p' dnl
        "$clang_cmake_file" | dnl
      sed -E dnl
        -e 's/([^;]+)/-I\1/g' dnl
        -e 's/;/ /g'`
changequote([, ])dnl
    CLANG_LDFLAGS="-L$clang_dir/lib"
  fi
  AC_SUBST(CLANG_CPPFLAGS)
  AC_SUBST(CLANG_LDFLAGS)
])
