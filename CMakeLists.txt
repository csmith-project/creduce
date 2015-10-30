# Copyright (c) 2012, 2013, 2014, 2015 The University of Utah
# All rights reserved.
#
# This file is distributed under the University of Illinois Open Source
# License.  See the file COPYING for details.

cmake_minimum_required(VERSION 2.8.12)
project(creduce)

include(CheckIncludeFile)
#include(CheckPrototypeExists)

# Generate config.h
set(ENABLE_TRANS_ASSERT ON CACHE BOOL "Use assert() in clang_delta transformations.")

check_include_file("dlfcn.h" HAVE_DLFCN_H)
check_include_file("inttypes.h" HAVE_INTTYPES_H)
check_include_file("memory.h" HAVE_MEMORY_H)
check_include_file("stdint.h" HAVE_STDINT_H)
check_include_file("stdlib.h" HAVE_STDLIB_H)
check_include_file("strings.h" HAVE_STRINGS_H)
check_include_file("string.h" HAVE_STRING_H)
check_include_file("sys/stat.h" HAVE_SYS_STAT_H)
check_include_file("sys/types.h" HAVE_SYS_TYPES_H)
check_include_file("unistd.h" HAVE_UNISTD_H)

set(creduce_PACKAGE "creduce")
set(creduce_PACKAGE_BUGREPORT "creduce-bugs@flux.utah.edu")
set(creduce_PACKAGE_NAME "creduce")
set(creduce_PACKAGE_STRING "creduce 2.5.0")
set(creduce_PACKAGE_TARNAME "creduce")
set(creduce_PACKAGE_URL "http://embed.cs.utah.edu/creduce/")
set(creduce_PACKAGE_VERSION "2.5.0")
set(creduce_VERSION "2.5.0")

execute_process(COMMAND "git" "show" "-s" "--format=%h" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" OUTPUT_VARIABLE creduce_GIT_HASH)
string(STRIP "${creduce_GIT_HASH}" creduce_GIT_HASH)

#check_prototype_exists(free stdlib.h freeExists)
#if(freeExists)
#    set(STDC_HEADERS 1)
#endif(freeExists)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "-std=c++11 -fno-rtti -Wall -Wextra -Wno-long-long -Wno-unused-parameter")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -fno-strict-aliasing")
endif()

#FIXME: Should be determined automatically
set(YYTEXT_POINTER 1)

configure_file("cmake_config.h.in" "${PROJECT_BINARY_DIR}/config.h")

add_subdirectory(creduce)
add_subdirectory(clang_delta)
add_subdirectory(clex)
