## -*- mode: CMake -*-
##
## Copyright (c) 2016 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

cmake_minimum_required(VERSION 2.8.12)

###############################################################################

project(unifdef)

include_directories(${PROJECT_BINARY_DIR})
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR})

add_executable(unifdef
  unifdef.c
  unifdef.h
  version.h
  )

###############################################################################

install(TARGETS unifdef
  RUNTIME DESTINATION "libexec"
  )

###############################################################################

## End of file.
