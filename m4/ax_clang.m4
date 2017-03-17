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
#   Test for a Clang build or installation tree, and set the output variables
#   `CLANG_CPPFLAGS' and `CLANG_LDFLAGS' as needed for compiling and linking
#   programs against that tree.
#
#   Clang is usually built and/or installed alongside LLVM.  When that is the
#   case, the information we get from macro AX_LLVM is enough for compiling and
#   linking programs against Clang.  This AX_CLANG macro is needed only when
#   the LLVM and Clang trees are separate, for whatever reason.
#
#   `CLANG_CPPFLAGS' is set to the command-line "-I" directives that are
#   needed to compile programs against the specified Clang tree.  This macro
#   digs the include directories out of the file "ClangConfig.cmake".
#
#   `CLANG_LDFLAGS' is set to the command-line "-L" directive that is needed.
#   This is easy to determine: the libraries are in the tree's "lib" directory.

AC_DEFUN([AX_CLANG],
[
  AC_ARG_WITH([clang],
    AS_HELP_STRING([--with-clang@<:@=DIR@:>@],
      [use separate Clang tree located in DIR
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
      [--with-clang requires an argument (a Clang build/install tree)])
  else

    clang_dir="$with_clang"
    clang_cmake_file="$clang_dir/lib/cmake/clang/ClangConfig.cmake"
    AC_CHECK_FILE([$clang_cmake_file],
      [],
      [
        AC_MSG_ERROR(
          [argument to --with-clang is not a Clang build/install tree])
      ])

    # Dig the list of include directories out of the $clang_cmake_file, which
    # is a CMake file.  The sed expressions below:
    #
    # + Find the line: set(CLANG_INCLUDE_DIRS "<dirlist>")
    #   and output:    <dirlist>
    # + From the list: <dir1>;<dir2>;...
    #   output:        -I<dir1>;-I<dir2>;...
    # + Erase semis:   -I<dir1> -I<dir2> ...
    #
changequote(<<, >>)dnl
    CLANG_CPPFLAGS=`dnl
      sed -n -E dnl
        -e 's/^set\(CLANG_INCLUDE_DIRS [^"]*"([^"]+)"\)$/\1/p' dnl
        "$clang_cmake_file" | dnl
      sed -E dnl
        -e 's/([^;]+)/-I\1/g' dnl
        -e 's/;/ /g'`
changequote([, ])dnl
    #
    # Find the list of library directories.  This is easy: everything is in
    # the "lib" subdirectory.
    #
    CLANG_LDFLAGS="-L$clang_dir/lib"
  fi
  AC_SUBST(CLANG_CPPFLAGS)
  AC_SUBST(CLANG_LDFLAGS)
])
