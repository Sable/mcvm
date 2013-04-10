INCLUDE = -Ivendor/include -Ilib/include
CXXFLAGS =   $(INCLUDE) -Wall -g -std=c++11 -Wno-deprecated
CXXFLAGS += -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -DMCVM_USE_LAPACKE -Wfatal-errors 
LLVMLIBS = $(shell vendor/bin/llvm-config --libfiles)
LIBS = -pthread  -ldl -llapacke -lcblas
LIBS +=  vendor/lib/libgccpp.a  vendor/lib/libgc.a 
CXX = g++

all:	source/analysis_arraycopy.o source/analysis_boundscheck.o source/analysis_copyplacement.o source/analysis_livevars.o \
	source/analysismanager.o source/analysis_metrics.o source/analysis_reachdefs.o source/analysis_typeinfer.o source/arrayobj.o \
	source/assignstmt.o source/binaryopexpr.o source/cellarrayexpr.o source/cellarrayobj.o source/cellindexexpr.o \
	source/chararrayobj.o source/client.o source/clientsocket.o source/configmanager.o source/dimvector.o source/endexpr.o \
	source/environment.o source/expressions.o source/exprstmt.o source/filesystem.o source/fnhandleexpr.o source/functions.o source/ifelsestmt.o \
	source/interpreter.o source/jitcompiler.o source/lambdaexpr.o source/loopstmts.o source/main.o source/matrixexpr.o source/matrixobjs.o source/matrixops.o source/objects.o \
	source/paramexpr.o source/parser.o source/plotting.o source/process.o source/profiling.o source/rangeexpr.o source/rangeobj.o source/runtimebase.o source/stdlib.o source/stmtsequence.o \
	source/switchstmt.o source/symbolexpr.o source/transform_endexpr.o source/transform_logic.o source/transform_loops.o source/transform_split.o source/transform_switch.o \
	source/typeinfer.o source/unaryopexpr.o source/utility.o source/xml.o
	$(CXX) source/*.o  $(LLVMLIBS) $(LIBS) -o mcvm

clean:
	rm source/*.o mcvm
