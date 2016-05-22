## -*- mode: CMake -*-
##
## Copyright (c) 2012, 2013, 2014, 2015, 2016 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

cmake_minimum_required(VERSION 2.8.12)

find_package(FLEX)

###############################################################################

project(clex)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR})
add_definitions(-DGIT_HASH="${creduce_GIT_HASH}")
add_definitions(-DHAVE_CONFIG_H)

FLEX_TARGET(clex_scanner clex.l ${PROJECT_BINARY_DIR}/clex.c)

add_executable(clex
  defs.h
  driver.c
  ${FLEX_clex_scanner_OUTPUTS}
)

###############################################################################

project(strlex)
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR})
add_definitions(-DGIT_HASH="${creduce_GIT_HASH}")
add_definitions(-DHAVE_CONFIG_H)

FLEX_TARGET(strlex_scanner strlex.l ${PROJECT_BINARY_DIR}/strlex.c)

add_executable(strlex
  defs.h
  driver.c
  ${FLEX_strlex_scanner_OUTPUTS}
)

###############################################################################

install(TARGETS clex strlex
  RUNTIME DESTINATION "libexec"
  PERMISSIONS
    OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

###############################################################################

## End of file.
