## -*- mode: CMake -*-
##
## Copyright (c) 2012, 2013, 2014, 2015, 2016 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

cmake_minimum_required(VERSION 2.8.12)
project(clang_delta)

# Find LLVM
find_package(LLVM REQUIRED CONFIG NO_CMAKE_BUILDS_PATH)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Check LLVM version
if (${LLVM_PACKAGE_VERSION} VERSION_LESS "3.8")
  message(FATAL_ERROR "LLVM version must be >= 3.8")
endif()
set(LLVM_VERSION ${LLVM_VERSION_MAJOR}${LLVM_VERSION_MINOR})

# Add flags for LLVM
add_definitions(${LLVM_DEFINITIONS})
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS HAVE_CONFIG_H
  $<$<CONFIG:Release>:NDEBUG> $<$<CONFIG:Debug>:ENABLE_TRANS_ASSERT>)
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CMAKE_BINARY_DIR})
link_directories(${LLVM_LIBRARY_DIRS})

llvm_map_components_to_libnames(LLVM_LIBS
  irreader
  mcparser
  option
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
)

set(TRANSFORMATION_HEADERS
  AggregateToScalar.h
  BinOpSimplification.h
  CallExprToValue.h
  ClassTemplateToClass.h
  CombineGlobalVarDecl.h
  CombineLocalVarDecl.h
  CommonParameterRewriteVisitor.h
  CommonRenameClassRewriteVisitor.h
  CommonStatementVisitor.h
  CommonTemplateArgumentVisitor.h
  CopyPropagation.h
  EmptyStructToInt.h
  InstantiateTemplateParam.h
  InstantiateTemplateTypeParamToInt.h
  LiftAssignmentExpr.h
  LocalToGlobal.h
  MoveFunctionBody.h
  MoveGlobalVar.h
  ParamToGlobal.h
  ParamToLocal.h
  ReduceArrayDim.h
  ReduceArraySize.h
  ReduceClassTemplateParameter.h
  ReducePointerLevel.h
  ReducePointerPairs.h
  RemoveAddrTaken.h
  RemoveArray.h
  RemoveBaseClass.h
  RemoveCtorInitializer.h
  RemoveEnumMemberValue.h
  RemoveNamespace.h
  RemoveNestedFunction.h
  RemovePointer.h
  RemoveTrivialBaseTemplate.h
  RemoveUnresolvedBase.h
  RemoveUnusedEnumMember.h
  RemoveUnusedFunction.h
  RemoveUnusedOuterClass.h
  RemoveUnusedStructField.h
  RemoveUnusedVar.h
  RenameCXXMethod.h
  RenameClass.h
  RenameFun.h
  RenameParam.h
  RenameVar.h
  ReplaceArrayIndexVar.h
  ReplaceCallExpr.h
  ReplaceClassWithBaseTemplateSpec.h
  ReplaceDependentName.h
  ReplaceDependentTypedef.h
  ReplaceDerivedClass.h
  ReplaceFunctionDefWithDecl.h
  ReplaceOneLevelTypedefType.h
  ReplaceSimpleTypedef.h
  ReplaceUndefinedFunction.h
  ReturnVoid.h
  SimpleInliner.h
  SimplifyCallExpr.h
  SimplifyCommaExpr.h
  SimplifyDependentTypedef.h
  SimplifyIf.h
  SimplifyNestedClass.h
  SimplifyRecursiveTemplateInstantiation.h
  SimplifyStruct.h
  SimplifyStructUnionDecl.h
  TemplateArgToInt.h
  TemplateNonTypeArgToInt.h
  UnifyFunctionDecl.h
  UnionToStruct.h
)

set(TRANSFORMATION_SOURCES
  AggregateToScalar.cpp
  BinOpSimplification.cpp
  CallExprToValue.cpp
  ClassTemplateToClass.cpp
  CombineGlobalVarDecl.cpp
  CombineLocalVarDecl.cpp
  CopyPropagation.cpp
  EmptyStructToInt.cpp
  InstantiateTemplateParam.cpp
  InstantiateTemplateTypeParamToInt.cpp
  LiftAssignmentExpr.cpp
  LocalToGlobal.cpp
  MoveFunctionBody.cpp
  MoveGlobalVar.cpp
  ParamToGlobal.cpp
  ParamToLocal.cpp
  ReduceArrayDim.cpp
  ReduceArraySize.cpp
  ReduceClassTemplateParameter.cpp
  ReducePointerLevel.cpp
  ReducePointerPairs.cpp
  RemoveAddrTaken.cpp
  RemoveArray.cpp
  RemoveBaseClass.cpp
  RemoveCtorInitializer.cpp
  RemoveEnumMemberValue.cpp
  RemoveNamespace.cpp
  RemoveNestedFunction.cpp
  RemovePointer.cpp
  RemoveTrivialBaseTemplate.cpp
  RemoveUnresolvedBase.cpp
  RemoveUnusedEnumMember.cpp
  RemoveUnusedFunction.cpp
  RemoveUnusedOuterClass.cpp
  RemoveUnusedStructField.cpp
  RemoveUnusedVar.cpp
  RenameCXXMethod.cpp
  RenameClass.cpp
  RenameFun.cpp
  RenameParam.cpp
  RenameVar.cpp
  ReplaceArrayIndexVar.cpp
  ReplaceCallExpr.cpp
  ReplaceClassWithBaseTemplateSpec.cpp
  ReplaceDependentName.cpp
  ReplaceDependentTypedef.cpp
  ReplaceDerivedClass.cpp
  ReplaceFunctionDefWithDecl.cpp
  ReplaceOneLevelTypedefType.cpp
  ReplaceSimpleTypedef.cpp
  ReplaceUndefinedFunction.cpp
  ReturnVoid.cpp
  SimpleInliner.cpp
  SimplifyCallExpr.cpp
  SimplifyCommaExpr.cpp
  SimplifyDependentTypedef.cpp
  SimplifyIf.cpp
  SimplifyNestedClass.cpp
  SimplifyRecursiveTemplateInstantiation.cpp
  SimplifyStruct.cpp
  SimplifyStructUnionDecl.cpp
  TemplateArgToInt.cpp
  TemplateNonTypeArgToInt.cpp
  UnifyFunctionDecl.cpp
  UnionToStruct.cpp
)

add_executable(clang_delta
  ${CMAKE_BINARY_DIR}/config.h
  ${TRANSFORMATION_HEADERS}
  ${TRANSFORMATION_SOURCES}
  RewriteUtils.h
  Transformation.h
  TransformationManager.h
  ClangDelta.cpp
  RewriteUtils.cpp
  Transformation.cpp
  TransformationManager.cpp
)

target_link_libraries(clang_delta ${CLANG_LIBS} ${LLVM_LIBS})

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
