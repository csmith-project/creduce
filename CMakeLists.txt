## -*- mode: CMake -*-
##
## Copyright (c) 2012, 2013, 2014, 2015, 2016, 2017, 2018 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

cmake_minimum_required(VERSION 2.8.12)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(CheckIncludeFile)
include(CheckCXXCompilerFlag)
include(GetGitRevisionDescription)

project(creduce)

###############################################################################

# Locate LLVM and check its version.  Do this here because we need the LLVM
# package definitions in the "CMakeLists.txt" files for multiple subdirs.
#
find_package(LLVM REQUIRED CONFIG NO_CMAKE_BUILDS_PATH)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in ${LLVM_DIR}")
if (${LLVM_PACKAGE_VERSION} VERSION_LESS "6.0")
  message(FATAL_ERROR "C-Reduce requires LLVM 6.0 or later")
endif()
#
# Do this here, too, just to keep it with the LLVM check above.
#
find_package(Clang REQUIRED CONFIG NO_CMAKE_BUILDS_PATH)
message(STATUS "Using ClangConfig.cmake in ${Clang_DIR}")

# Locate Perl and check its version.
#
include(FindPerl)
if(${PERL_VERSION_STRING} VERSION_LESS "5.10.0")
  message(FATAL_ERROR "C-Reduce requires Perl 5.10.0 or later")
endif()

# Locate flex.  Do this here because we need the package definitions in the
# "CMakeLists.txt" files for multiple subdirs.
#
find_package(FLEX)

###############################################################################

# Determine the short git hash for the source tree.  The logic here follows the
# logic in the `git-hash.sh` script.
#
## METHOD 1: The source tree is the result of `git archive'.
# `git archive' inserts the abbreviated hash of the archive's commit into this
# file.  (See the `.gitattributes' file.)
#
set(GIT_HASH "$Format:%h$")
if(GIT_HASH MATCHES "^\\$")
  ## METHOD 2: The source tree is a git repository.
  get_git_head_revision(GIT_REFSPEC GIT_HASH)
  if(NOT GIT_HASH STREQUAL "GITDIR-NOTFOUND")
    # Trim to the short hash.
    string(SUBSTRING "${GIT_HASH}" 0 7 GIT_HASH)
  else()
    ## METHOD 3: Give up.
    set(GIT_HASH "unknown")
  endif()
endif()

###############################################################################

# Generate the "config.h" file.
#
set(ENABLE_TRANS_ASSERT ON CACHE BOOL
  "Use assert() in clang_delta transformations.")

check_include_file("dlfcn.h"		HAVE_DLFCN_H)
check_include_file("inttypes.h"		HAVE_INTTYPES_H)
check_include_file("memory.h"		HAVE_MEMORY_H)
check_include_file("stdint.h"		HAVE_STDINT_H)
check_include_file("stdlib.h"		HAVE_STDLIB_H)
check_include_file("strings.h"		HAVE_STRINGS_H)
check_include_file("string.h"		HAVE_STRING_H)
check_include_file("sys/stat.h"		HAVE_SYS_STAT_H)
check_include_file("sys/types.h"	HAVE_SYS_TYPES_H)
check_include_file("unistd.h"		HAVE_UNISTD_H)

set(creduce_PACKAGE			"creduce")
set(creduce_PACKAGE_BUGREPORT		"creduce-bugs@flux.utah.edu")
set(creduce_PACKAGE_NAME		"creduce")
set(creduce_PACKAGE_STRING		"creduce 2.8.0")
set(creduce_PACKAGE_TARNAME		"creduce")
set(creduce_PACKAGE_URL			"http://embed.cs.utah.edu/creduce/")
set(creduce_PACKAGE_VERSION		"2.8.0")
set(creduce_VERSION			"2.8.0")

# FIXME: Should be determined automatically.
set(YYTEXT_POINTER 1)

configure_file("cmake_config.h.in" "${PROJECT_BINARY_DIR}/config.h")
add_definitions("-DHAVE_CONFIG_H")

###############################################################################

# Use option `-fvisibility-inlines-hidden` if the C++ compiler supports it.
# See LLVM file `share/llvm/cmake/HandleLLVMOptions.cmake`.
#
check_cxx_compiler_flag(
  "-fvisibility-inlines-hidden"
  SUPPORTS_FVISIBILITY_INLINES_HIDDEN_FLAG
  )

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
    OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # XXX figure out how to get "-std=c++11 -fno-rtti" from LLVM.  That's how we
  # get those options in the Automake path...
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-strict-aliasing -Wall -Wextra -Wno-long-long -Wno-unused-parameter -Wno-missing-field-initializers")
  if(SUPPORTS_FVISIBILITY_INLINES_HIDDEN_FLAG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility-inlines-hidden")
  endif()
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()

###############################################################################

add_subdirectory(clang_delta)
add_subdirectory(clex)
add_subdirectory(creduce)
add_subdirectory(delta)
# `unifdef' requires a Unix-y platform.
if(UNIX)
  add_subdirectory(unifdef)
else()
  message(WARNING
    "Internal tool `unifdef${CMAKE_EXECUTABLE_SUFFIX}' cannot be compiled on this OS; skipping.")
endif()

###############################################################################

## End of file.
