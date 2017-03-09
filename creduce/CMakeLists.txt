## -*- mode: CMake -*-
##
## Copyright (c) 2012, 2013, 2014, 2015, 2016 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

cmake_minimum_required(VERSION 2.8.12)

project(creduce_python)

###############################################################################

# find_package(LLVM) is done by the topmost "CMakeLists.txt" file.

###############################################################################

# Check for the run-time prerequisites of C-Reduce.  We only warn the user when
# these are not found at configure time.  Let the user install the dependencies
# later.
#
find_program(CLANG_FORMAT
  "clang-format${CMAKE_EXECUTABLE_SUFFIX}"
  PATHS "${LLVM_TOOLS_BINARY_DIR}"
)

if(NOT CLANG_FORMAT)
  message(STATUS "`clang-format${CMAKE_EXECUTABLE_SUFFIX}' was not found")
  message("You must install `clang-format' before running C-Reduce.")
  set(CLANG_FORMAT "clang-format")
endif()

###############################################################################

# Generate file "creduce.py".
#
configure_file("${creduce_python_SOURCE_DIR}/creduce.py"
  "${creduce_python_BINARY_DIR}/creduce.py"
)

###############################################################################

# Copy the Python modules into the build tree so that we can run C-Reduce there.
#
configure_file(
  "${creduce_python_SOURCE_DIR}/__init__.py"
  "${creduce_python_BINARY_DIR}/__init__.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/pass_groups/all.json"
  "${creduce_python_BINARY_DIR}/pass_groups/all.json"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/pass_groups/opencl-120.json"
  "${creduce_python_BINARY_DIR}/pass_groups/opencl-120.json"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/passes/__init__.py"
  "${creduce_python_BINARY_DIR}/passes/__init__.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/passes/abstract.py"
  "${creduce_python_BINARY_DIR}/passes/abstract.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/passes/balanced.py"
  "${creduce_python_BINARY_DIR}/passes/balanced.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/passes/blank.py"
  "${creduce_python_BINARY_DIR}/passes/blank.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/passes/clang.py"
  "${creduce_python_BINARY_DIR}/passes/clang.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/passes/clangbinarysearch.py"
  "${creduce_python_BINARY_DIR}/passes/clangbinarysearch.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/passes/clex.py"
  "${creduce_python_BINARY_DIR}/passes/clex.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/passes/comments.py"
  "${creduce_python_BINARY_DIR}/passes/comments.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/passes/includeincludes.py"
  "${creduce_python_BINARY_DIR}/passes/includeincludes.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/passes/includes.py"
  "${creduce_python_BINARY_DIR}/passes/includes.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/passes/indent.py"
  "${creduce_python_BINARY_DIR}/passes/indent.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/passes/ints.py"
  "${creduce_python_BINARY_DIR}/passes/ints.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/passes/lines.py"
  "${creduce_python_BINARY_DIR}/passes/lines.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/passes/peep.py"
  "${creduce_python_BINARY_DIR}/passes/peep.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/passes/special.py"
  "${creduce_python_BINARY_DIR}/passes/special.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/passes/ternary.py"
  "${creduce_python_BINARY_DIR}/passes/ternary.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/passes/unifdef.py"
  "${creduce_python_BINARY_DIR}/passes/unifdef.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/tests/__init__.py"
  "${creduce_python_BINARY_DIR}/tests/__init__.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/tests/balanced.py"
  "${creduce_python_BINARY_DIR}/tests/balanced.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/tests/comments.py"
  "${creduce_python_BINARY_DIR}/tests/comments.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/tests/ints.py"
  "${creduce_python_BINARY_DIR}/tests/ints.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/tests/nestedmatcher.py"
  "${creduce_python_BINARY_DIR}/tests/nestedmatcher.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/tests/peep.py"
  "${creduce_python_BINARY_DIR}/tests/peep.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/tests/special.py"
  "${creduce_python_BINARY_DIR}/tests/special.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/tests/ternary.py"
  "${creduce_python_BINARY_DIR}/tests/ternary.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/utils/__init__.py"
  "${creduce_python_BINARY_DIR}/utils/__init__.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/utils/compat.py"
  "${creduce_python_BINARY_DIR}/utils/compat.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/utils/error.py"
  "${creduce_python_BINARY_DIR}/utils/error.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/utils/info.py"
  "${creduce_python_BINARY_DIR}/utils/info.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/utils/nestedmatcher.py"
  "${creduce_python_BINARY_DIR}/utils/nestedmatcher.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/utils/readkey.py"
  "${creduce_python_BINARY_DIR}/utils/readkey.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/utils/statistics.py"
  "${creduce_python_BINARY_DIR}/utils/statistics.py"
  COPYONLY
)
configure_file(
  "${creduce_python_SOURCE_DIR}/utils/testing.py"
  "${creduce_python_BINARY_DIR}/utils/testing.py"
  COPYONLY
)

###############################################################################

install(DIRECTORY "${creduce_python_BINARY_DIR}/"
  DESTINATION "${CMAKE_INSTALL_PREFIX}/share/${creduce_PACKAGE}"
  FILES_MATCHING
  PATTERN "*.py"
  PATTERN "*.json"
  PATTERN "__pycache__" EXCLUDE
  PATTERN "CMakeFiles" EXCLUDE
)

###############################################################################

## End of file.
