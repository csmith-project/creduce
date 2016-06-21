## -*- mode: CMake -*-
##
## Copyright (c) 2016 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

cmake_minimum_required(VERSION 2.8.12)

# find_package(FLEX) is done by the topmost "CMakeLists.txt" file.

###############################################################################

project(topformflat)

include_directories(${PROJECT_BINARY_DIR})
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR})

FLEX_TARGET(topformflat_scanner
  topformflat.l
  ${PROJECT_BINARY_DIR}/topformflat.c
  )

add_executable(topformflat
  ${FLEX_topformflat_scanner_OUTPUTS}
  )

###############################################################################

install(TARGETS topformflat
  RUNTIME DESTINATION "libexec"
  )

###############################################################################

## End of file.
