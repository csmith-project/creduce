# CMakeLists.txt (clang_delta)
# Copyright (c) 2015, Moritz Pflanzer}
# Imperial College London. All rights reserved.
#
# This program is provided under a three-clause BSD license. For full
# license terms please see the LICENSE file distributed with this
# source code.

cmake_minimum_required(VERSION 2.8.12)
project(clang_delta)

# Find LLVM
find_package(LLVM REQUIRED CONFIG NO_CMAKE_BUILDS_PATH)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Check LLVM version
if (${LLVM_PACKAGE_VERSION} VERSION_LESS "3.7")
  message(FATAL_ERROR "LLVM version must be >= 3.7")
endif()
set(LLVM_VERSION ${LLVM_VERSION_MAJOR}${LLVM_VERSION_MINOR})

# Add flags for LLVM
add_definitions(${LLVM_DEFINITIONS})
add_definitions(-DHAVE_CONFIG_H -DNDEBUG)
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CMAKE_BINARY_DIR})
link_directories(${LLVM_LIBRARY_DIRS})

# Get LLVM libraries for linking
#llvm_map_components_to_libnames(LLVM_LIBS
#  bitreader bitwriter core instrumentation ipo irreader
#  linker mcparser objcarcopts option target)
set(LLVM_LIBS
clangFrontendTool.lib
clangFrontend.lib
clangDriver.lib
clangSerialization.lib
clangCodeGen.lib
clangParse.lib
clangSema.lib
clangAnalysis.lib
clangRewriteFrontend.lib
clangRewrite.lib
clangAST.lib
clangBasic.lib
clangEdit.lib
clangLex.lib
LLVMLTO.lib
LLVMObjCARCOpts.lib
LLVMLinker.lib
LLVMBitWriter.lib
LLVMIRReader.lib
LLVMMIRParser.lib
LLVMAsmParser.lib
LLVMDebugInfoPDB.lib
LLVMTableGen.lib
LLVMOrcJIT.lib
LLVMInstrumentation.lib
LLVMOption.lib
LLVMX86Disassembler.lib
LLVMX86AsmParser.lib
LLVMX86CodeGen.lib
LLVMSelectionDAG.lib
LLVMAsmPrinter.lib
LLVMX86Desc.lib
LLVMMCDisassembler.lib
LLVMX86Info.lib
LLVMX86AsmPrinter.lib
LLVMX86Utils.lib
LLVMMCJIT.lib
LLVMDebugInfoDWARF.lib
LLVMLineEditor.lib
LLVMPasses.lib
LLVMipo.lib
LLVMVectorize.lib
LLVMInterpreter.lib
LLVMExecutionEngine.lib
LLVMRuntimeDyld.lib
LLVMCodeGen.lib
LLVMTarget.lib
LLVMScalarOpts.lib
LLVMProfileData.lib
LLVMObject.lib
LLVMMCParser.lib
LLVMBitReader.lib
LLVMInstCombine.lib
LLVMTransformUtils.lib
LLVMipa.lib
LLVMMC.lib
LLVMAnalysis.lib
LLVMCore.lib
LLVMSupport.lib
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

target_link_libraries(clang_delta ${LLVM_LIBS})

install(TARGETS clang_delta
  RUNTIME DESTINATION "bin"
)