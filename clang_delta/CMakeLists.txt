## -*- mode: CMake -*-
##
## Copyright (c) 2012, 2013, 2014, 2015, 2016, 2017, 2018 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

cmake_minimum_required(VERSION 2.8.12)
project(clang_delta)

###############################################################################

# find_package(LLVM) is done by the topmost "CMakeLists.txt" file.
# find_package(Clang) is done by the topmost "CMakeLists.txt" file.

# Generate file "git_version.cpp".
#
configure_file("git_version.cpp.in" "git_version.cpp" @ONLY)

# set values for replacing variables in lit.site.cfg.in
#
set(LLVM_BINDIR ${LLVM_TOOLS_BINARY_DIR})
set(abs_builddir ${CMAKE_CURRENT_BINARY_DIR})
set(clang_delta_srcdir ${CMAKE_CURRENT_SOURCE_DIR})

# Generate file "lit.site.cfg".
#
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tests/lit.site.cfg.in
  ${CMAKE_CURRENT_BINARY_DIR}/tests/lit.site.cfg @ONLY)

# Generate file "remove_lit_checks.pl", which is necessary for removing
# "lit-functioning" lines such as "CHECK:" and "RUN:" from test output.
#
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tests/remove_lit_checks.in
  ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/remove_lit_checks @ONLY)
file(COPY ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/remove_lit_checks
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/tests
  FILE_PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE)


# Add flags for compiling against LLVM.
#
add_definitions(${LLVM_DEFINITIONS})
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
  $<$<CONFIG:Release>:NDEBUG>)
# include_directories(${PROJECT_BINARY_DIR}) --- if we generated .h files here
include_directories(${CMAKE_BINARY_DIR})
include_directories(${PROJECT_SOURCE_DIR}) # needed for gen'ed .cpp files
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CLANG_INCLUDE_DIRS}) # only works for LLVM post-4.0

link_directories(${LLVM_LIBRARY_DIRS})

llvm_map_components_to_libnames(LLVM_LIBS
  coverage
  irreader
  mcparser
  objcarcopts
  option
  passes
  profiledata
  support
)

set(CLANG_LIBS
  clangStaticAnalyzerFrontend
  clangStaticAnalyzerCheckers
  clangStaticAnalyzerCore
  clangFrontendTool
  clangFrontend
  clangDriver
  clangSerialization
  clangCodeGen
  clangParse
  clangSema
  clangAnalysis
  clangRewriteFrontend
  clangRewrite
  clangAST
  clangBasic
  clangEdit
  clangLex
  clangARCMigrate
)

add_executable(clang_delta
  ${CMAKE_BINARY_DIR}/config.h
  AggregateToScalar.cpp
  AggregateToScalar.h
  BinOpSimplification.cpp
  BinOpSimplification.h
  CallExprToValue.cpp
  CallExprToValue.h
  ClangDelta.cpp
  ClassTemplateToClass.cpp
  ClassTemplateToClass.h
  CombineGlobalVarDecl.cpp
  CombineGlobalVarDecl.h
  CombineLocalVarDecl.cpp
  CombineLocalVarDecl.h
  CommonParameterRewriteVisitor.h
  CommonRenameClassRewriteVisitor.h
  CommonStatementVisitor.h
  CommonTemplateArgumentVisitor.h
  CopyPropagation.cpp
  CopyPropagation.h
  EmptyStructToInt.cpp
  EmptyStructToInt.h
  ExpressionDetector.cpp
  ExpressionDetector.h
  InstantiateTemplateParam.cpp
  InstantiateTemplateParam.h
  InstantiateTemplateTypeParamToInt.cpp
  InstantiateTemplateTypeParamToInt.h
  LiftAssignmentExpr.cpp
  LiftAssignmentExpr.h
  LocalToGlobal.cpp
  LocalToGlobal.h
  MoveFunctionBody.cpp
  MoveFunctionBody.h
  MoveGlobalVar.cpp
  MoveGlobalVar.h
  ParamToGlobal.cpp
  ParamToGlobal.h
  ParamToLocal.cpp
  ParamToLocal.h
  ReduceArrayDim.cpp
  ReduceArrayDim.h
  ReduceArraySize.cpp
  ReduceArraySize.h
  ReduceClassTemplateParameter.cpp
  ReduceClassTemplateParameter.h
  ReducePointerLevel.cpp
  ReducePointerLevel.h
  ReducePointerPairs.cpp
  ReducePointerPairs.h
  RemoveAddrTaken.cpp
  RemoveAddrTaken.h
  RemoveArray.cpp
  RemoveArray.h
  RemoveBaseClass.cpp
  RemoveBaseClass.h
  RemoveCtorInitializer.cpp
  RemoveCtorInitializer.h
  RemoveEnumMemberValue.cpp
  RemoveEnumMemberValue.h
  RemoveNamespace.cpp
  RemoveNamespace.h
  RemoveNestedFunction.cpp
  RemoveNestedFunction.h
  RemovePointer.cpp
  RemovePointer.h
  RemoveTrivialBaseTemplate.cpp
  RemoveTrivialBaseTemplate.h
  RemoveUnresolvedBase.cpp
  RemoveUnresolvedBase.h
  RemoveUnusedEnumMember.cpp
  RemoveUnusedEnumMember.h
  RemoveUnusedFunction.cpp
  RemoveUnusedFunction.h
  RemoveUnusedOuterClass.cpp
  RemoveUnusedOuterClass.h
  RemoveUnusedStructField.cpp
  RemoveUnusedStructField.h
  RemoveUnusedVar.cpp
  RemoveUnusedVar.h
  RenameCXXMethod.cpp
  RenameCXXMethod.h
  RenameClass.cpp
  RenameClass.h
  RenameFun.cpp
  RenameFun.h
  RenameParam.cpp
  RenameParam.h
  RenameVar.cpp
  RenameVar.h
  ReplaceArrayAccessWithIndex.cpp
  ReplaceArrayAccessWithIndex.h
  ReplaceArrayIndexVar.cpp
  ReplaceArrayIndexVar.h
  ReplaceCallExpr.cpp
  ReplaceCallExpr.h
  ReplaceClassWithBaseTemplateSpec.cpp
  ReplaceClassWithBaseTemplateSpec.h
  ReplaceDependentName.cpp
  ReplaceDependentName.h
  ReplaceDependentTypedef.cpp
  ReplaceDependentTypedef.h
  ReplaceDerivedClass.cpp
  ReplaceDerivedClass.h
  ReplaceFunctionDefWithDecl.cpp
  ReplaceFunctionDefWithDecl.h
  ReplaceOneLevelTypedefType.cpp
  ReplaceOneLevelTypedefType.h
  ReplaceSimpleTypedef.cpp
  ReplaceSimpleTypedef.h
  ReplaceUndefinedFunction.cpp
  ReplaceUndefinedFunction.h
  ReturnVoid.cpp
  ReturnVoid.h
  RewriteUtils.cpp
  RewriteUtils.h
  SimpleInliner.cpp
  SimpleInliner.h
  SimplifyCallExpr.cpp
  SimplifyCallExpr.h
  SimplifyCommaExpr.cpp
  SimplifyCommaExpr.h
  SimplifyDependentTypedef.cpp
  SimplifyDependentTypedef.h
  SimplifyIf.cpp
  SimplifyIf.h
  SimplifyNestedClass.cpp
  SimplifyNestedClass.h
  SimplifyRecursiveTemplateInstantiation.cpp
  SimplifyRecursiveTemplateInstantiation.h
  SimplifyStruct.cpp
  SimplifyStruct.h
  SimplifyStructUnionDecl.cpp
  SimplifyStructUnionDecl.h
  TemplateArgToInt.cpp
  TemplateArgToInt.h
  TemplateNonTypeArgToInt.cpp
  TemplateNonTypeArgToInt.h
  Transformation.cpp
  Transformation.h
  TransformationManager.cpp
  TransformationManager.h
  UnifyFunctionDecl.cpp
  UnifyFunctionDecl.h
  UnionToStruct.cpp
  UnionToStruct.h
  VectorToArray.cpp
  VectorToArray.h
  ${PROJECT_BINARY_DIR}/git_version.cpp
  git_version.h
)

# ENE, LLVM 6.0: LLVM_LIBS end up in the link command line anyway.  I'm not
# quite sure why!  But if you list LLVM_LIBS here, the `clang_delta' binary
# can end up with dynamic link errors when it runs (multiply defined symbols).
#
target_link_libraries(clang_delta ${CLANG_LIBS})

# Custom target for running clang_delta tests
#
add_custom_target(check-clang-delta
  COMMAND ${LLVM_TOOLS_BINARY_DIR}/llvm-lit
          -sv ${CMAKE_CURRENT_BINARY_DIR}/tests
  DEPENDS clang_delta)

# On Windows, we also need to link with "Version.dll" system library.
# See <https://github.com/csmith-project/creduce/pull/126>.
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  target_link_libraries(clang_delta Version)
endif()

install(TARGETS clang_delta
  RUNTIME DESTINATION "libexec"
)
# If binary is read-only, then installing may fail with an error:
# CMake Error at cmake_install.cmake:45 (FILE):
#  file RPATH_REMOVE could not remove RPATH from file:
#    /disk2/randtest/install/libexec/clang_delta
#  Error opening file for update.
#
# Do I want RPATH at all?  We can turn it off, by setting CMAKE_SKIP_RPATH
# https://cmake.org/cmake/help/v2.8.8/cmake.html#variable%3aCMAKE_SKIP_RPATH
# See also the stuff about RPATH for `set_target_properties', which I don't
# really grok:
# https://cmake.org/cmake/help/v2.8.8/cmake.html#command:set_target_properties
#
#  PERMISSIONS
#    OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE

###############################################################################

## End of file.
