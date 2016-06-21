## -*- mode: CMake -*-
##
## Copyright (c) 2012, 2013, 2014, 2015, 2016 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

cmake_minimum_required(VERSION 2.8.12)

# find_package(FLEX) is done by the topmost "CMakeLists.txt" file.

###############################################################################

project(clex)

include_directories(${PROJECT_BINARY_DIR})
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR})

FLEX_TARGET(clex_scanner
  clex.l
  ${PROJECT_BINARY_DIR}/clex.c
  )

add_executable(clex
  ${FLEX_clex_scanner_OUTPUTS}
  defs.h
  driver.c
  )

###############################################################################

project(strlex)
include_directories(${PROJECT_BINARY_DIR})
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR})

FLEX_TARGET(strlex_scanner
  strlex.l
  ${PROJECT_BINARY_DIR}/strlex.c
  )

add_executable(strlex
  ${FLEX_strlex_scanner_OUTPUTS}
  defs.h
  driver.c
  )

###############################################################################

install(TARGETS clex strlex
  RUNTIME DESTINATION "libexec"
  )

###############################################################################

## End of file.
