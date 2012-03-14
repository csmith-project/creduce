##===----------------------------------------------------------------------===
##
## Copyright (c) 2012 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See LICENSE.TXT for details.
##
##===----------------------------------------------------------------------===

llvm_dir = ${LLVM_PATH}

ENABLE_TRANS_ASSERT = -DENABLE_TRANS_ASSERT
ifeq (${DISABLE_TRANS_ASSERT}, 1)
  ENABLE_TRANS_ASSERT = -DDISABLE_TRANS_ASSERT
endif

CXX = g++
CXXFLAGS = -Wall

LLVM_CXXFLAGS = `${llvm_dir}/bin/llvm-config --cxxflags`
LLVM_LDFLAGS = `${llvm_dir}/bin/llvm-config --ldflags`
LLVM_LIBS = `${llvm_dir}/bin/llvm-config --libs | tr " " "\n" | grep -v gtest`

INCLUDES = ${LLVM_CXXFLAGS} -I${llvm_dir}/include/clang/
LDFLAGS = ${LLVM_LDFLAGS}
LIBS = -lclangFrontendTool -lclangFrontend -lclangDriver -lclangSerialization \
       -lclangCodeGen -lclangParse -lclangSema -lclangAnalysis \
       -lclangIndex -lclangRewrite -lclangAST -lclangLex -lclangBasic -lclangEdit \
       ${LLVM_LIBS} -ldl

TRANSFORM_HEADERS = ParamToLocal.h ParamToGlobal.h LocalToGlobal.h ReturnVoid.h \
                    RemoveNestedFunction.h BinOpSimplification.h AggregateToScalar.h \
                    RenameVar.h RenameFun.h RenameParam.h CombineGlobalVarDecl.h \
                    CombineLocalVarDecl.h ReplaceCallExpr.h SimpleInliner.h \
                    RemoveUnusedFunction.h ReducePointerLevel.h LiftAssignmentExpr.h \
                    CopyPropagation.h RemoveUnusedVar.h SimplifyCallExpr.h \
                    CallExprToValue.h UnionToStruct.h SimplifyIf.h \
                    ReduceArrayDim.h ReduceArraySize.h MoveFunctionBody.h \
                    SimplifyStructUnionDecl.h SimplifyCommaExpr.h MoveGlobalVar.h \
                    UnifyFunctionDecl.h RemoveUnusedStructField.h

TRANSFORM_SOURCES = ParamToLocal.cpp ParamToGlobal.cpp LocalToGlobal.cpp ReturnVoid.cpp \
                    RemoveNestedFunction.cpp BinOpSimplification.cpp AggregateToScalar.cpp \
                    RenameVar.cpp RenameFun.cpp RenameParam.cpp CombineGlobalVarDecl.cpp \
                    CombineLocalVarDecl.cpp ReplaceCallExpr.cpp SimpleInliner.cpp \
                    RemoveUnusedFunction.cpp ReducePointerLevel.cpp LiftAssignmentExpr.cpp \
                    CopyPropagation.cpp RemoveUnusedVar.cpp SimplifyCallExpr.cpp \
                    CallExprToValue.cpp UnionToStruct.cpp SimplifyIf.cpp \
                    ReduceArrayDim.cpp ReduceArraySize.cpp MoveFunctionBody.cpp \
                    SimplifyStructUnionDecl.cpp SimplifyCommaExpr.cpp MoveGlobalVar.cpp \
                    UnifyFunctionDecl.cpp RemoveUnusedStructField.cpp

TRANSFORM_OBJS = ParamToLocal.o ParamToGlobal.o LocalToGlobal.o ReturnVoid.o \
                 RemoveNestedFunction.o BinOpSimplification.o AggregateToScalar.o \
                 RenameVar.o RenameFun.o RenameParam.o CombineGlobalVarDecl.o \
                 CombineLocalVarDecl.o ReplaceCallExpr.o SimpleInliner.o \
                 RemoveUnusedFunction.o ReducePointerLevel.o LiftAssignmentExpr.o \
                 CopyPropagation.o RemoveUnusedVar.o SimplifyCallExpr.o \
                 CallExprToValue.o UnionToStruct.o SimplifyIf.o \
                 ReduceArrayDim.o ReduceArraySize.o MoveFunctionBody.o \
                 SimplifyStructUnionDecl.o SimplifyCommaExpr.o MoveGlobalVar.o \
                 UnifyFunctionDecl.o RemoveUnusedStructField.o

OBJS = ClangDelta.o \
       TransformationManager.o \
       RewriteUtils.o \
       Transformation.o \
       ${TRANSFORM_OBJS}

SUB_TRANSFORMATION_DEPS = Transformation.cpp Transformation.h TransformationManager.h TransformationManager.cpp RewriteUtils.cpp RewriteUtils.h

.SUFFIXES : .o .cpp
.cpp.o :
	${CXX} -g ${ENABLE_TRANS_ASSERT} ${INCLUDES} ${CXXFLAGS} -c $<

clang_delta: ${OBJS}
	${CXX} -g -o clang_delta ${OBJS} ${LDFLAGS} ${LIBS}

ClangDelta.o: ClangDelta.cpp

TransformationManager.o: TransformationManager.cpp TransformationManager.h ${TRANSFORMATION_HEADERS} ${TRANSFORM_SOURCES}

RewriteUtils.o: RewriteUtils.cpp RewriteUtils.h 

Transformation.o: Transformation.cpp Transformation.h RewriteUtils.cpp RewriteUtils.h

ParamToLocal.o: ParamToLocal.cpp ParamToLocal.h ${SUB_TRANSFORMATION_DEPS}

ParamToGlobal.o: ParamToGlobal.cpp ParamToGlobal.h ${SUB_TRANSFORMATION_DEPS}

RemoveNestedFunction.o: RemoveNestedFunction.cpp RemoveNestedFunction.h ${SUB_TRANSFORMATION_DEPS}

BinOpSimplification.o: BinOpSimplification.cpp BinOpSimplification.h ${SUB_TRANSFORMATION_DEPS}

LocalToGlobal.o: LocalToGlobal.cpp LocalToGlobal.h ${SUB_TRANSFORMATION_DEPS}

ReturnVoid.o: ReturnVoid.cpp ReturnVoid.h ${SUB_TRANSFORMATION_DEPS}

AggregateToScalar.o: AggregateToScalar.cpp AggregateToScalar.h ${SUB_TRANSFORMATION_DEPS}

RenameVar.o: RenameVar.cpp RenameVar.h ${SUB_TRANSFORMATION_DEPS}

RenameFun.o: RenameFun.cpp RenameFun.h ${SUB_TRANSFORMATION_DEPS}

RenameParam.o: RenameParam.cpp RenameParam.h ${SUB_TRANSFORMATION_DEPS}

CombineGlobalVarDecl.o: CombineGlobalVarDecl.cpp CombineGlobalVarDecl.h ${SUB_TRANSFORMATION_DEPS}

CombineLocalVarDecl.o: CombineLocalVarDecl.cpp CombineLocalVarDecl.h ${SUB_TRANSFORMATION_DEPS}

ReplaceCallExpr.o: ReplaceCallExpr.cpp ReplaceCallExpr.h ${SUB_TRANSFORMATION_DEPS}

SimpleInliner.o: SimpleInliner.cpp SimpleInliner.h ${SUB_TRANSFORMATION_DEPS}

RemoveUnusedFunction.o: RemoveUnusedFunction.cpp RemoveUnusedFunction.h ${SUB_TRANSFORMATION_DEPS}

ReducePointerLevel.o: ReducePointerLevel.cpp ReducePointerLevel.h ${SUB_TRANSFORMATION_DEPS}

LiftAssignmentExpr.o: LiftAssignmentExpr.cpp LiftAssignmentExpr.h ${SUB_TRANSFORMATION_DEPS}

CopyPropagation.o: CopyPropagation.cpp CopyPropagation.h ${SUB_TRANSFORMATION_DEPS}

RemoveUnusedVar.o: RemoveUnusedVar.cpp RemoveUnusedVar.h ${SUB_TRANSFORMATION_DEPS}

SimplifyCallExpr.o: SimplifyCallExpr.cpp SimplifyCallExpr.h ${SUB_TRANSFORMATION_DEPS}

CallExprToValue.o: CallExprToValue.cpp CallExprToValue.h ${SUB_TRANSFORMATION_DEPS}

UnionToStruct.o: UnionToStruct.cpp UnionToStruct.h ${SUB_TRANSFORMATION_DEPS}

SimplifyIf.o: SimplifyIf.cpp SimplifyIf.h ${SUB_TRANSFORMATION_DEPS}

ReduceArrayDim.o: ReduceArrayDim.cpp ReduceArrayDim.h ${SUB_TRANSFORMATION_DEPS}

ReduceArraySize.o: ReduceArraySize.cpp ReduceArraySize.h ${SUB_TRANSFORMATION_DEPS}

MoveFunctionBody.o: MoveFunctionBody.cpp MoveFunctionBody.h ${SUB_TRANSFORMATION_DEPS}

SimplifyStructUnionDecl.o: SimplifyStructUnionDecl.cpp SimplifyStructUnionDecl.h ${SUB_TRANSFORMATION_DEPS}

SimplifyCommaExpr.o: SimplifyCommaExpr.cpp SimplifyCommaExpr.h ${SUB_TRANSFORMATION_DEPS}

MoveGlobalVar.o: MoveGlobalVar.cpp MoveGlobalVar.h ${SUB_TRANSFORMATION_DEPS}

UnifyFunctionDecl.o: UnifyFunctionDecl.cpp UnifyFunctionDecl.h ${SUB_TRANSFORMATION_DEPS}

RemoveUnusedStructField.o: RemoveUnusedStructField.cpp RemoveUnusedStructField.h ${SUB_TRANSFORMATION_DEPS}

clean:
	rm -rf *.o

distclean:
	rm -rf *.o clang_delta

