## -*- mode: CMake -*-
##
## Copyright (c) 2012, 2013, 2014, 2015, 2016, 2018, 2019 The University of Utah
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
  set(missing_required_runtime_prereq "yes")
endif()


if(NOT CLANG_FORMAT)
  message(STATUS "`clang-format${CMAKE_EXECUTABLE_SUFFIX}' was not found")
  message("You must install `clang-format' before running C-Reduce.")
  set(CLANG_FORMAT "clang-format")
endif()

###############################################################################

# Generate file "creduce.py".
#
file(READ "${creduce_python_SOURCE_DIR}/creduce.py"
  CREDUCE_CONTENT
)

string(REPLACE "%PACKAGE_BUGREPORT%" "${creduce_PACKAGE_BUGREPORT}"
  CREDUCE_CONTENT "${CREDUCE_CONTENT}"
)
string(REPLACE "%PACKAGE_NAME%" "${creduce_PACKAGE_NAME}"
  CREDUCE_CONTENT "${CREDUCE_CONTENT}"
)
string(REPLACE "%PACKAGE_STRING%" "${creduce_PACKAGE_STRING}"
  CREDUCE_CONTENT "${CREDUCE_CONTENT}"
)
string(REPLACE "%PACKAGE_URL%" "${creduce_PACKAGE_URL}"
  CREDUCE_CONTENT "${CREDUCE_CONTENT}"
)
string(REPLACE "%PACKAGE_VERSION%" "${creduce_PACKAGE_VERSION}"
  CREDUCE_CONTENT "${CREDUCE_CONTENT}"
)
string(REPLACE "%VERSION%" "${creduce_VERSION}"
  CREDUCE_CONTENT "${CREDUCE_CONTENT}"
)
string(REPLACE "%GIT_HASH%" "${GIT_HASH}"
  CREDUCE_CONTENT "${CREDUCE_CONTENT}"
)

file(WRITE "${creduce_python_BINARY_DIR}/creduce.py"
  "${CREDUCE_CONTENT}"
)

###############################################################################

# Copy the Python modules into the build tree so that we can run C-Reduce there.
#
file(COPY "${creduce_python_SOURCE_DIR}/"
  DESTINATION "${creduce_python_BINARY_DIR}"
  FILES_MATCHING
  PATTERN "*.py"
  PATTERN "*.json"
  PATTERN "__pycache__" EXCLUDE
  PATTERN "creduce.py" EXCLUDE
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
