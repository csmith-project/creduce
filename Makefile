# for my convenience
#llvm_installation_dir = /uusoc/exports/scratch/chenyang/clang_reducer/llvm-3.0

llvm_dir = ${LLVM_PATH}

ENABLE_TRANS_ASSERT = -DENABLE_TRANS_ASSERT
ifeq (${DISABLE_TRANS_ASSERT}, 1)
  ENABLE_TRANS_ASSERT = -DDISABLE_TRANS_ASSERT
endif

CXX = g++
CXXFLAGS = -Wall

LLVM_CXXFLAGS = `${llvm_dir}/bin/llvm-config --cxxflags`
LLVM_LDFLAGS = `${llvm_dir}/bin/llvm-config --ldflags`
LLVM_LIBS = `${llvm_dir}/bin/llvm-config --libs`

INCLUDES = ${LLVM_CXXFLAGS} -I${llvm_dir}/include/clang/
LDFLAGS = ${LLVM_LDFLAGS}
LIBS = -lclangFrontendTool -lclangFrontend -lclangDriver -lclangSerialization \
       -lclangCodeGen -lclangParse -lclangSema -lclangAnalysis \
       -lclangIndex -lclangRewrite -lclangAST -lclangLex -lclangBasic \
       ${LLVM_LIBS} 

TRANSFORM_OBJS = ParamToLocal.o ParamToGlobal.o LocalToGlobal.o ReturnVoid.o \
                 RemoveNestedFunction.o BinOpSimplification.o AggregateToScalar.o \
                 RenameVar.o RenameFun.o RenameParam.o CombineGlobalVarDecl.o \
                 CombineLocalVarDecl.o ReplaceCallExpr.o SimpleInliner.o \
                 RemoveUnusedFunction.o ReducePointerLevel.o LiftAssignmentExpr.o \
                 CopyPropagation.o RemoveUnusedVar.o

OBJS = ClangDelta.o \
       TransformationManager.o \
       RewriteUtils.o \
       Transformation.o \
       ${TRANSFORM_OBJS}

.SUFFIXES : .o .cpp
.cpp.o :
	${CXX} -g ${ENABLE_TRANS_ASSERT} ${CXXFLAGS} ${INCLUDES} -c $<

clang_delta: ${OBJS}
	${CXX} -g -o clang_delta ${OBJS} ${LDFLAGS} ${LIBS}

ClangDelta.o: ClangDelta.cpp

TransformationManager.o: TransformationManager.cpp TransformationManager.h ${TRANSFORM_OBJS}

RewriteUtils.o: RewriteUtils.cpp RewriteUtils.h 

Transformation.o: Transformation.cpp Transformation.h RewriteUtils.o

ParamToLocal.o: ParamToLocal.cpp ParamToLocal.h Transformation.o

ParamToGlobal.o: ParamToGlobal.cpp ParamToGlobal.h Transformation.o

RemoveNestedFunction.o: RemoveNestedFunction.cpp RemoveNestedFunction.h Transformation.o

BinOpSimplification.o: BinOpSimplification.cpp BinOpSimplification.h Transformation.o

LocalToGlobal.o: LocalToGlobal.cpp LocalToGlobal.h Transformation.o

ReturnVoid.o: ReturnVoid.cpp ReturnVoid.h Transformation.o

AggregateToScalar.o: AggregateToScalar.cpp AggregateToScalar.h Transformation.o

RenameVar.o: RenameVar.cpp RenameVar.h Transformation.o

RenameFun.o: RenameFun.cpp RenameFun.h Transformation.o

RenameParam.o: RenameParam.cpp RenameParam.h Transformation.o

CombineGlobalVarDecl.o: CombineGlobalVarDecl.cpp CombineGlobalVarDecl.h Transformation.o

CombineLocalVarDecl.o: CombineLocalVarDecl.cpp CombineLocalVarDecl.h Transformation.o

ReplaceCallExpr.o: ReplaceCallExpr.cpp ReplaceCallExpr.h Transformation.o

SimpleInliner.o: SimpleInliner.cpp SimpleInliner.h Transformation.o

RemoveUnusedFunction.o: RemoveUnusedFunction.cpp RemoveUnusedFunction.h Transformation.o

ReducePointerLevel.o: ReducePointerLevel.cpp ReducePointerLevel.h Transformation.o

LiftAssignmentExpr.o: LiftAssignmentExpr.cpp LiftAssignmentExpr.h Transformation.o

CopyPropagation.o: CopyPropagation.cpp CopyPropagation.h Transformation.o

RemoveUnusedVar.o: RemoveUnusedVar.cpp RemoveUnusedVar.h Transformation.o

clean:
	rm -rf *.o

cleanall:
	rm -rf *.o clang_delta

