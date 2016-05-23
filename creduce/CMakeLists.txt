## -*- mode: CMake -*-
##
## Copyright (c) 2012, 2013, 2014, 2015, 2016 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

cmake_minimum_required(VERSION 2.8.12)

project(creduce_perl)

set(BIN_DIR "${CMAKE_INSTALL_PREFIX}/bin")
set(LIB_EXEC_DIR "${CMAKE_INSTALL_PREFIX}/libexec")
set(PKG_DATA_DIR "${CMAKE_INSTALL_PREFIX}/share/${creduce_PACKAGE}")
set(PERL_LIB_DIR "${PKG_DATA_DIR}/perl")

###

include(FindPerl)
if (${PERL_VERSION_STRING} VERSION_LESS "5.10.0")
  message(FATAL_ERROR "C-Reduce requires Perl 5.10.0 or later")
endif()

find_package(LLVM REQUIRED CONFIG NO_CMAKE_BUILDS_PATH)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

###

find_program(ASTYLE
  "astyle${CMAKE_EXECUTABLE_SUFFIX}"
  )
if(NOT ASTYLE)
  message(WARNING "astyle${CMAKE_EXECUTABLE_SUFFIX} could not be found! Set ASTYLE manually.")
  set(ASTYLE "no") # Emulate `configure.ac' behavior.
endif(NOT ASTYLE)

find_program(CLANG_FORMAT
  "clang-format${CMAKE_EXECUTABLE_SUFFIX}"
  PATHS "${LLVM_TOOLS_BINARY_DIR}"
  )
if(NOT CLANG_FORMAT)
  message(WARNING "clang-format${CMAKE_EXECUTABLE_SUFFIX} could not be found! Set CLANG_FORMAT manually.")
  set(CLANG_FORMAT "no") # Emulate `configure.ac' behavior.
endif(NOT CLANG_FORMAT)

find_program(INDENT
  NAMES
    "gindent${CMAKE_EXECUTABLE_SUFFIX}"
    "indent${CMAKE_EXECUTABLE_SUFFIX}"
  )
if(NOT INDENT)
  message(WARNING "indent${CMAKE_EXECUTABLE_SUFFIX} could not be found! Set INDENT manually.")
  set(INDENT "no") # Emulate `configure.ac' behavior.
endif(NOT INDENT)

###############################################################################

# Generate file "creduce".
#
file(READ "creduce.in" CREDUCE_CONTENT)
string(REPLACE "@perl@" "${PERL_EXECUTABLE}"
  CREDUCE_CONTENT "${CREDUCE_CONTENT}")
string(REPLACE "@PERL@" "${PERL_EXECUTABLE}"
  CREDUCE_CONTENT "${CREDUCE_CONTENT}")
string(REPLACE "@perllibdir@" "${PERL_LIB_DIR}"
  CREDUCE_CONTENT "${CREDUCE_CONTENT}")
string(REPLACE "@prefix@" "${CMAKE_INSTALL_PREFIX}"
  CREDUCE_CONTENT "${CREDUCE_CONTENT}")
file(WRITE
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/creduce"
  "${CREDUCE_CONTENT}")
file(COPY
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/creduce"
  DESTINATION "${PROJECT_BINARY_DIR}"
  FILE_PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE
  )

# Generate file "creduce_config.pm".
#
file(READ "creduce_config.pm.in" CREDUCE_CONTENT)
string(REPLACE "@bindir@" "${BIN_DIR}"
  CREDUCE_CONTENT "${CREDUCE_CONTENT}")
string(REPLACE "@libexecdir@" "${LIB_EXEC_DIR}"
  CREDUCE_CONTENT "${CREDUCE_CONTENT}")
string(REPLACE "@PACKAGE_BUGREPORT@" "${creduce_PACKAGE_BUGREPORT}"
  CREDUCE_CONTENT "${CREDUCE_CONTENT}")
string(REPLACE "@PACKAGE_NAME@" "${creduce_PACKAGE_NAME}"
  CREDUCE_CONTENT "${CREDUCE_CONTENT}")
string(REPLACE "@PACKAGE_STRING@" "${creduce_PACKAGE_STRING}"
  CREDUCE_CONTENT "${CREDUCE_CONTENT}")
string(REPLACE "@PACKAGE_URL@" "${creduce_PACKAGE_URL}"
  CREDUCE_CONTENT "${CREDUCE_CONTENT}")
string(REPLACE "@PACKAGE_VERSION@" "${creduce_PACKAGE_VERSION}"
  CREDUCE_CONTENT "${CREDUCE_CONTENT}")
string(REPLACE "@VERSION@" "${creduce_VERSION}"
  CREDUCE_CONTENT "${CREDUCE_CONTENT}")
string(REPLACE "@GIT_HASH@" "${creduce_GIT_HASH}"
  CREDUCE_CONTENT "${CREDUCE_CONTENT}")
# ENE: I do not understand why the \'s seem to be required in the following
# match strings.
string(REPLACE "\@ASTYLE@" "${ASTYLE}"
  CREDUCE_CONTENT "${CREDUCE_CONTENT}")
string(REPLACE "\@CLANG_FORMAT@" "${CLANG_FORMAT}"
  CREDUCE_CONTENT "${CREDUCE_CONTENT}")
string(REPLACE "\@INDENT@" "${INDENT}"
  CREDUCE_CONTENT "${CREDUCE_CONTENT}")
file(WRITE
  "${PROJECT_BINARY_DIR}/creduce_config.pm"
  "${CREDUCE_CONTENT}"
  )

###############################################################################

install(PROGRAMS
  "${PROJECT_BINARY_DIR}/creduce"
  DESTINATION "${BIN_DIR}"
  )

install(DIRECTORY
  "${PROJECT_BINARY_DIR}/"
  "${PROJECT_SOURCE_DIR}/"
  DESTINATION "${PERL_LIB_DIR}"
  FILES_MATCHING
    PATTERN "CMakeFiles" EXCLUDE
    PATTERN "*.pm"
  )

###############################################################################

## End of file.
