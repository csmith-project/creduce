## -*- mode: CMake -*-
##
## Copyright (c) 2012, 2013, 2014, 2015, 2016, 2018 The University of Utah
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

###############################################################################

# find_package(LLVM) is done by the topmost "CMakeLists.txt" file.

# include(FindPerl) is done by the topmost "CMakeLists.txt" file.

###############################################################################

# Check for the run-time prerequisites of C-Reduce.  We only warn the user when
# these are not found at configure time.  Let the user install the dependencies
# later.
#
set(missing_required_runtime_prereq "no")
set(missing_optional_runtime_prereq "no")
set(required_perl_modules "yes")
set(optional_perl_modules "yes")

find_program(CLANG_FORMAT
  "clang-format${CMAKE_EXECUTABLE_SUFFIX}"
  PATHS "${LLVM_TOOLS_BINARY_DIR}"
  )
if(NOT CLANG_FORMAT)
  message(STATUS "`clang-format${CMAKE_EXECUTABLE_SUFFIX}' was not found")
  set(missing_required_runtime_prereq "yes")
endif()

###

execute_process(COMMAND ${PERL_EXECUTABLE} -MExporter::Lite -e ""
  ERROR_QUIET
  RESULT_VARIABLE status
  )
if(status)
  message(STATUS "Required Perl module `Exporter::Lite' was not found")
  set(missing_required_runtime_prereq "yes")
  set(required_perl_modules "no")
endif()
#
execute_process(COMMAND ${PERL_EXECUTABLE} -MFile::Which -e ""
  ERROR_QUIET
  RESULT_VARIABLE status
  )
if(status)
  message(STATUS "Required Perl module `File::Which' was not found")
  set(required_perl_modules "no")
  set(missing_required_runtime_prereq "yes")
endif()
#
execute_process(COMMAND ${PERL_EXECUTABLE} -MGetopt::Tabular -e ""
  ERROR_QUIET
  RESULT_VARIABLE status
  )
if(status)
  message(STATUS "Required Perl module `Getopt::Tabular' was not found")
  set(required_perl_modules "no")
  set(missing_required_runtime_prereq "yes")
endif()
#
execute_process(COMMAND ${PERL_EXECUTABLE} -MRegexp::Common -e ""
  ERROR_QUIET
  RESULT_VARIABLE status
  )
if(status)
  message(STATUS "Required Perl module `Regexp::Common' was not found")
  set(required_perl_modules "no")
  set(missing_required_runtime_prereq "yes")
endif()
#
execute_process(COMMAND ${PERL_EXECUTABLE} -MTerm::ReadKey -e ""
  ERROR_QUIET
  RESULT_VARIABLE status
  )
if(status)
  message(STATUS "Optional Perl module `Term::ReadKey' was not found")
  set(optional_perl_modules "no")
  set(missing_optional_runtime_prereq "yes")
endif()

###

# Warn about things that will be needed at run time, but which we did not find
# at configure time.
#
# First warn about required run-time dependencies...
#
if(missing_required_runtime_prereq STREQUAL "yes")
  message(WARNING
    "You cannot run C-Reduce until you install missing dependencies!")
endif()

if(required_perl_modules STREQUAL "no")
  message("You must install Perl modules required by C-Reduce.")
endif()
if(NOT CLANG_FORMAT)
  message("You must install `clang-format' before running C-Reduce.")
  set(CLANG_FORMAT "clang-format")
endif()

if(missing_required_runtime_prereq STREQUAL "yes")
  message("Read the INSTALL file for info about C-Reduce dependencies.")
endif()

# ...and then warn about optional run-time dependencies.
#
if(missing_optional_runtime_prereq STREQUAL "yes")
  message("The following optional C-Reduce dependencies were not found.")
endif()

if(optional_perl_modules STREQUAL "no")
  message("One or more useful Perl modules were not found.")
endif()

if(missing_optional_runtime_prereq STREQUAL "yes")
  message("C-Reduce will use these tools if you install them in the future.")
  message("Read the INSTALL file for info about C-Reduce dependencies.")
endif()

###############################################################################

# Generate file "creduce".
#
file(READ "creduce.in" CREDUCE_CONTENT)
string(REPLACE "@perl@" "${PERL_EXECUTABLE}"
  CREDUCE_CONTENT "${CREDUCE_CONTENT}")
# The following is not needed, but triggers a warning in CMake 3.1+.
# See `cmake --help-policy CMP0053'.  Avoid the warning by not doing it.
# string(REPLACE "@PERL@" "${PERL_EXECUTABLE}"
#   CREDUCE_CONTENT "${CREDUCE_CONTENT}")
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
# ENE: I do not understand why the \'s seem to be required in the following
# match strings.
string(REPLACE "\@GIT_HASH@" "${GIT_HASH}"
  CREDUCE_CONTENT "${CREDUCE_CONTENT}")
string(REPLACE "\@CLANG_FORMAT@" "${CLANG_FORMAT}"
  CREDUCE_CONTENT "${CREDUCE_CONTENT}")
file(WRITE
  "${PROJECT_BINARY_DIR}/creduce_config.pm"
  "${CREDUCE_CONTENT}"
  )

###############################################################################

# Copy the Perl modules into the build tree so that we can run C-Reduce there.
#
# Blech!  But listing all the source files is what we do in the Autoconf-based
# build system, too.
#
add_custom_target(Modules ALL
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${PROJECT_SOURCE_DIR}/creduce_regexes.pm
    ${PROJECT_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${PROJECT_SOURCE_DIR}/creduce_utils.pm
    ${PROJECT_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${PROJECT_SOURCE_DIR}/pass_balanced.pm
    ${PROJECT_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${PROJECT_SOURCE_DIR}/pass_blank.pm
    ${PROJECT_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${PROJECT_SOURCE_DIR}/pass_clang.pm
    ${PROJECT_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${PROJECT_SOURCE_DIR}/pass_clang_binsrch.pm
    ${PROJECT_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${PROJECT_SOURCE_DIR}/pass_clex.pm
    ${PROJECT_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${PROJECT_SOURCE_DIR}/pass_comments.pm
    ${PROJECT_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${PROJECT_SOURCE_DIR}/pass_include_includes.pm
    ${PROJECT_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${PROJECT_SOURCE_DIR}/pass_includes.pm
    ${PROJECT_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${PROJECT_SOURCE_DIR}/pass_indent.pm
    ${PROJECT_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${PROJECT_SOURCE_DIR}/pass_ints.pm
    ${PROJECT_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${PROJECT_SOURCE_DIR}/pass_lines.pm
    ${PROJECT_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${PROJECT_SOURCE_DIR}/pass_peep.pm
    ${PROJECT_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${PROJECT_SOURCE_DIR}/pass_special.pm
    ${PROJECT_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${PROJECT_SOURCE_DIR}/pass_ternary.pm
    ${PROJECT_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${PROJECT_SOURCE_DIR}/pass_unifdef.pm
    ${PROJECT_BINARY_DIR}
  COMMENT "Copying C-Reduce Perl modules to build tree"
  )

###############################################################################

install(PROGRAMS
  "${PROJECT_BINARY_DIR}/creduce"
  DESTINATION "${BIN_DIR}"
  )

install(FILES
  "${PROJECT_BINARY_DIR}/creduce_config.pm"
  DESTINATION "${PERL_LIB_DIR}"
  )
install(DIRECTORY
  "${PROJECT_SOURCE_DIR}/"
  DESTINATION "${PERL_LIB_DIR}"
  FILES_MATCHING
    PATTERN "CMakeFiles" EXCLUDE
    PATTERN "*.pm"
  )

###############################################################################

## End of file.
