# Copyright (c) 2012, 2013, 2014, 2015 The University of Utah
# All rights reserved.
#
# This file is distributed under the University of Illinois Open Source
# License.  See the file COPYING for details.

cmake_minimum_required(VERSION 2.8.12)
project(creduce_perl)

include(FindPerl)

find_program(ASTYLE "astyle${CMAKE_EXECUTABLE_SUFFIX}")
if(NOT ASTYLE)
message(WARNING "astyle${CMAKE_EXECUTABLE_SUFFIX} could not be found! Set ASTYLE manually.")
endif(NOT ASTYLE)

# Find LLVM
find_package(LLVM QUIET CONFIG NO_CMAKE_BUILDS_PATH)

# clang-format may be in the LLVM path.
find_program(CLANG_FORMAT NAMES "clang-format" "clang-format${CMAKE_EXECUTABLE_SUFFIX}"
  PATHS ${LLVM_TOOLS_BINARY_DIR} ${CMAKE_EXECUTABLE_SUFFIX}
  )
if(NOT CLANG_FORMAT)
message(WARNING "clang-format${CMAKE_EXECUTABLE_SUFFIX} could not be found! Set CLANG_FORMAT manually.")
endif(NOT CLANG_FORMAT)

find_program(INDENT "indent${CMAKE_EXECUTABLE_SUFFIX}")
if(NOT INDENT)
message(WARNING "indent${CMAKE_EXECUTABLE_SUFFIX} could not be found! Set INDENT manually.")
endif(NOT INDENT)

find_program(TOPFORMFLAT "topformflat${CMAKE_EXECUTABLE_SUFFIX}")
if(NOT TOPFORMFLAT)
message(WARNING "topformflat${CMAKE_EXECUTABLE_SUFFIX} could not be found! Set TOPFORMFLAT manually.")
endif(NOT TOPFORMFLAT)

# Prepare creduce.pl
set(perl "${PERL_EXECUTABLE}")
set(perllibdir "${CMAKE_INSTALL_PREFIX}/modules")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/creduce.in
  ${PROJECT_BINARY_DIR}/creduce
  @ONLY
  )

# FIXME: Make creduce file in the binary folder actually executable.
# Make the creduce file executable
#file(COPY ${PROJECT_BINARY_DIR}/creduce
#  DESTINATION ${PROJECT_BINARY_DIR}
#  FILE_PERMISSIONS
#  OWNER_READ OWNER_WRITE OWNER_EXECUTE
#  GROUP_READ GROUP_EXECUTE
#  WORLD_READ WORLD_EXECUTE
#  )

# Prepare creduce_config.pm
set(bindir "${CMAKE_INSTALL_PREFIX}")
set(libexecdir "${CMAKE_INSTALL_PREFIX}/bin")
set(PACKAGE_BUGREPORT "${creduce_PACKAGE_BUGREPORT}")
set(PACKAGE_NAME "${creduce_PACKAGE_NAME}")
set(PACKAGE_STRING "${creduce_PACKAGE_STRING}")
set(PACKAGE_URL "${creduce_PACKAGE_URL}")
set(PACKAGE_VERSION "${creduce_PACKAGE_VERSION}")
set(VERSION "${creduce_VERSION}")
set(GIT_HASH "${creduce_GIT_HASH}")
#set(ASTYLE "${ASTYLE}")
#set(CLANG_FORMAT "${CLANG_FORMAT}")
#set(INDENT "${INDENT}")
#set(TOPFORMFLAT "${TOPFORMFLAT}")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/creduce_config.pm.in
  ${PROJECT_BINARY_DIR}/modules/creduce_config.pm
  @ONLY
  )

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/creduce_regexes.pm ${PROJECT_BINARY_DIR}/modules/ COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/creduce_utils.pm ${PROJECT_BINARY_DIR}/modules/ COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pass_balanced.pm ${PROJECT_BINARY_DIR}/modules/ COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pass_blank.pm ${PROJECT_BINARY_DIR}/modules/ COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pass_clang.pm ${PROJECT_BINARY_DIR}/modules/ COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pass_clang_binsrch.pm ${PROJECT_BINARY_DIR}/modules/ COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pass_clex.pm ${PROJECT_BINARY_DIR}/modules/ COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pass_comments.pm ${PROJECT_BINARY_DIR}/modules/ COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pass_include_includes.pm ${PROJECT_BINARY_DIR}/modules/ COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pass_includes.pm ${PROJECT_BINARY_DIR}/modules/ COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pass_indent.pm ${PROJECT_BINARY_DIR}/modules/ COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pass_ints.pm ${PROJECT_BINARY_DIR}/modules/ COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pass_lines.pm ${PROJECT_BINARY_DIR}/modules/ COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pass_peep.pm ${PROJECT_BINARY_DIR}/modules/ COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pass_special.pm ${PROJECT_BINARY_DIR}/modules/ COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pass_ternary.pm ${PROJECT_BINARY_DIR}/modules/ COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pass_unifdef.pm ${PROJECT_BINARY_DIR}/modules/ COPYONLY)

install(PROGRAMS
  "${PROJECT_BINARY_DIR}/creduce"
  DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/"
)

install(DIRECTORY
  "${PROJECT_BINARY_DIR}/modules/"
  DESTINATION "${CMAKE_INSTALL_PREFIX}/modules"
)
