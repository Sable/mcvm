// =========================================================================== //
//                                                                             //
// Copyright 2008-2011 Maxime Chevalier-Boisvert, Nurudeen Abiodun Lameed,     //
//   Rahul Garg and McGill University.                                         //
//                                                                             //
//   Licensed under the Apache License, Version 2.0 (the "License");           //
//   you may not use this file except in compliance with the License.          //
//   You may obtain a copy of the License at                                   //
//                                                                             //
//       http://www.apache.org/licenses/LICENSE-2.0                            //
//                                                                             //
//   Unless required by applicable law or agreed to in writing, software       //
//   distributed under the License is distributed on an "AS IS" BASIS,         //
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  //
//   See the License for the specific language governing permissions and       //
//   limitations under the License.                                            //
//                                                                             //
// =========================================================================== //

// Header files
#include <cassert>
#include <iostream>
#include <llvm/Module.h>
#include <llvm/PassManager.h>
#include <llvm/CallingConv.h>
#include <llvm/Constants.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Instructions.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/DataLayout.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Assembly/PrintModulePass.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/ManagedStatic.h>


#include "jitcompiler.h"
#include "runtimebase.h"
#include "interpreter.h"
#include "functions.h"
#include "environment.h"
#include "matrixobjs.h"
#include "matrixops.h"
#include "transform_logic.h"
#include "transform_split.h"

// Config variable to enable/disable the JIT compiler
ConfigVar JITCompiler::s_jitEnableVar("jit_enable", ConfigVar::BOOL, "false");

// Config variable to enable/disable array deep copy
ConfigVar JITCompiler::s_jitCopyEnableVar("jit_copy_enable", ConfigVar::BOOL, "false");

// Config variable to enable/disable on-stack replacement capability
ConfigVar JITCompiler::s_jitOsrEnableVar("jit_osr_enable", ConfigVar::BOOL, "false");
ConfigVar JITCompiler::s_jitOsrStrategyVar("jit_osr_strategy", ConfigVar::STRING, "any");

// Config variables to enable/disable specific JIT optimizations
ConfigVar JITCompiler::s_jitUseArrayOpts("jit_use_array_opts", ConfigVar::BOOL, "true");
ConfigVar JITCompiler::s_jitUseBinOpOpts("jit_use_binop_opts", ConfigVar::BOOL, "true");
ConfigVar JITCompiler::s_jitUseLibOpts("jit_use_libfunc_opts", ConfigVar::BOOL, "true");
ConfigVar JITCompiler::s_jitUseDirectCalls("jit_use_direct_calls", ConfigVar::BOOL, "true");

// Config variables to disable matrix read/write bounds checking
ConfigVar JITCompiler::s_jitNoReadBoundChecks("jit_no_read_bound_checks", ConfigVar::BOOL, "false");
ConfigVar JITCompiler::s_jitNoWriteBoundChecks("jit_no_write_bound_checks", ConfigVar::BOOL, "false");

llvm::LLVMContext* JITCompiler::s_Context;

// Void pointer type constant
llvm::Type* JITCompiler::VOID_PTR_TYPE;

// LLVM module to store functions
llvm::Module* JITCompiler::s_pModule = NULL;

// LLVM execution engine
llvm::ExecutionEngine* JITCompiler::s_pExecEngine = NULL;

llvm::DataLayout* JITCompiler::s_data_layout = nullptr ;

// LLVM function pass manager (for optimization passes)
llvm::FunctionPassManager* JITCompiler::s_pFunctionPasses = NULL;

// LLVM function pass manager (for the printing pass)
llvm::FunctionPassManager* JITCompiler::s_pPrintPass = NULL;

// LLVM function pass manager (for the osr info pass)
llvm::FunctionPassManager* JITCompiler::s_pOsrInfoPass = NULL;

// Map of function pointers to native function objects
JITCompiler::NativeMap JITCompiler::s_nativeMap;

// Map of signatures to optimized library functions
JITCompiler::LibFuncMap JITCompiler::s_libFuncMap;

// Map of program functions to function objects
JITCompiler::FunctionMap JITCompiler::s_functionMap;

llvm::Value* JITCompiler::createAddInstr(llvm::IRBuilder<>& builder, llvm::Value* pLVal, llvm::Value* pRVal) { 
	llvm::Type* ltype = pLVal->getType();
	if(ltype->isDoubleTy() || ltype->isFloatTy()) return builder.CreateFAdd(pLVal, pRVal); 
	return builder.CreateAdd(pLVal,pRVal);
}
llvm::Value* JITCompiler::createSubInstr(llvm::IRBuilder<>& builder, llvm::Value* pLVal, llvm::Value* pRVal) { 
	llvm::Type* ltype = pLVal->getType();
	if(ltype->isDoubleTy() || ltype->isFloatTy()) return builder.CreateFSub(pLVal, pRVal); 
	return builder.CreateSub(pLVal,pRVal);
}
llvm::Value* JITCompiler::createMulInstr(llvm::IRBuilder<>& builder, llvm::Value* pLVal, llvm::Value* pRVal) { 
	llvm::Type* ltype = pLVal->getType();
	if(ltype->isDoubleTy() || ltype->isFloatTy()) return builder.CreateFMul(pLVal, pRVal); 
	return builder.CreateMul(pLVal,pRVal);
}
/***************************************************************
* Function: CompError::toString()
* Purpose : Get a string representation of a compilation error
* Initial : Maxime Chevalier-Boisvert on March 9, 2009
****************************************************************
Revisions and bug fixes:
*/
std::string CompError::toString() const
{
	// Create a string to store the output
	std::string output;
	
	// Add the error text to the output
	output += m_errorText;
	
	// If an erroneous node was specified
	if (m_pErrorNode)
	{
		// Add a string representation of the node to the output
		output += ":\n" + m_pErrorNode->toString();
	}	
	
	// Return the output string
	return output;
}

/***************************************************************
* Function: JITCompiler::initialize()
* Purpose : Initialize the JIT compiler
* Initial : Maxime Chevalier-Boisvert on March 9, 2009
****************************************************************
Revisions and bug fixes:
*/
void JITCompiler::initialize()
{
    llvm::InitializeNativeTarget();

    s_Context = new llvm::LLVMContext() ;

    VOID_PTR_TYPE = llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(*s_Context));
    
    // Create the LLVM module object
    s_pModule = new llvm::Module("mcvm", *s_Context);
		
    s_data_layout = new llvm::DataLayout(s_pModule) ;

	s_pExecEngine = 
      llvm::ExecutionEngine::createJIT(s_pModule, 0, 0, llvm::CodeGenOpt::None);
	
	// Create a function pass manager for the module
	s_pFunctionPasses = new llvm::FunctionPassManager(s_pModule);
	
	// Add a verification pass to the function passes
	s_pFunctionPasses->add(llvm::createVerifierPass(llvm::PrintMessageAction));
	
	// Add optimization passes to the function passes
	s_pFunctionPasses->add(llvm::createCFGSimplificationPass());
	s_pFunctionPasses->add(llvm::createPromoteMemoryToRegisterPass());
	s_pFunctionPasses->add(llvm::createReassociatePass());
	s_pFunctionPasses->add(llvm::createConstantPropagationPass());
    //	s_pFunctionPasses->add(llvm::createCondPropagationPass());
	s_pFunctionPasses->add(llvm::createDeadCodeEliminationPass());
	s_pFunctionPasses->add(llvm::createGVNPass());
	s_pFunctionPasses->add(llvm::createInstructionCombiningPass());
	s_pFunctionPasses->add(llvm::createBlockPlacementPass());
	s_pFunctionPasses->add(llvm::createCFGSimplificationPass());
	
	// Create a function pass manager for the module
	s_pPrintPass = new llvm::FunctionPassManager(s_pModule);	
	
	// Add a pass to print generated functions
	s_pPrintPass->add(llvm::createPrintFunctionPass("", &llvm::outs()));
	
	// Create a type vector to represent arguments of eval type functions
	LLVMTypeVector evalArgs(2, VOID_PTR_TYPE);

	// Create a type vector to represent arguments of the object assignment function
	LLVMTypeVector assignArgs;
	assignArgs.push_back(VOID_PTR_TYPE);
	assignArgs.push_back(VOID_PTR_TYPE);
	assignArgs.push_back(VOID_PTR_TYPE);
	assignArgs.push_back(llvm::Type::getInt8Ty(*s_Context));
	
	// Create a type vector to represent arguments of parameterized expression evaluation functions
	LLVMTypeVector evalParamArgs;
	evalParamArgs.push_back(VOID_PTR_TYPE);
	evalParamArgs.push_back(VOID_PTR_TYPE);
	evalParamArgs.push_back(getIntType(sizeof(size_t)));		
	
	// Create a type vector to represent the arguments of the type conversion method
	LLVMTypeVector convertArgs;
	convertArgs.push_back(VOID_PTR_TYPE);
	convertArgs.push_back(getIntType(sizeof(DataObject::Type)));
	
	// Create a type vector to represent the arguments of the array object accessing method
	LLVMTypeVector getObjArgs;
	getObjArgs.push_back(VOID_PTR_TYPE);
	getObjArgs.push_back(getIntType(sizeof(size_t)));
	
	// Create a type vector to represent the arguments of the function calling method
	LLVMTypeVector callFnArgs;
	callFnArgs.push_back(VOID_PTR_TYPE);
	callFnArgs.push_back(VOID_PTR_TYPE);
	callFnArgs.push_back(getIntType(sizeof(size_t)));		
	
	// Create a type vector to represent the arguments of scalar matrix operations
	LLVMTypeVector f64ScalarOpArgs;
	f64ScalarOpArgs.push_back(VOID_PTR_TYPE);
	f64ScalarOpArgs.push_back(llvm::Type::getDoubleTy(*s_Context));

	// Create a type vector for int64 binary operators
	LLVMTypeVector i64BinOpArgs;
	i64BinOpArgs.push_back(llvm::Type::getInt64Ty(*s_Context));
	i64BinOpArgs.push_back(llvm::Type::getInt64Ty(*s_Context));
	
	// Create a type vector for float64 binary operators
	LLVMTypeVector f64BinOpArgs;
	f64BinOpArgs.push_back(llvm::Type::getDoubleTy(*s_Context));
	f64BinOpArgs.push_back(llvm::Type::getDoubleTy(*s_Context));
	
	// Create a type vector for the matrix expansion function
	LLVMTypeVector expandArgs;
	expandArgs.push_back(VOID_PTR_TYPE);
	expandArgs.push_back(llvm::PointerType::getUnqual(getIntType(sizeof(size_t))));
	expandArgs.push_back(getIntType(sizeof(size_t)));
		
	// Create a type vector for the matrix 1D read function
	LLVMTypeVector read1DArgs;
	read1DArgs.push_back(VOID_PTR_TYPE);
	read1DArgs.push_back(llvm::Type::getInt64Ty(*s_Context));

	// Create a type vector for the matrix 2D read function
	LLVMTypeVector read2DArgs;
	read2DArgs.push_back(VOID_PTR_TYPE);
	read2DArgs.push_back(llvm::Type::getInt64Ty(*s_Context));
	read2DArgs.push_back(llvm::Type::getInt64Ty(*s_Context));

	// Create type vectors for the matrix 1D write functions
	LLVMTypeVector f64Write1DArgs;
	f64Write1DArgs.push_back(VOID_PTR_TYPE);
	f64Write1DArgs.push_back(llvm::Type::getInt64Ty(*s_Context));
	f64Write1DArgs.push_back(llvm::Type::getDoubleTy(*s_Context));
	LLVMTypeVector i8Write1DArgs;
	i8Write1DArgs.push_back(VOID_PTR_TYPE);
	i8Write1DArgs.push_back(llvm::Type::getInt64Ty(*s_Context));
	i8Write1DArgs.push_back(llvm::Type::getInt8Ty(*s_Context));
	
	// Create type vectors for the matrix 2D write functions
	LLVMTypeVector f64Write2DArgs;
	f64Write2DArgs.push_back(VOID_PTR_TYPE);
	f64Write2DArgs.push_back(llvm::Type::getInt64Ty(*s_Context));
	f64Write2DArgs.push_back(llvm::Type::getInt64Ty(*s_Context));
	f64Write2DArgs.push_back(llvm::Type::getDoubleTy(*s_Context));
	LLVMTypeVector i8Write2DArgs;
	i8Write2DArgs.push_back(VOID_PTR_TYPE);
	i8Write2DArgs.push_back(llvm::Type::getInt64Ty(*s_Context));
	i8Write2DArgs.push_back(llvm::Type::getInt64Ty(*s_Context));
	i8Write2DArgs.push_back(llvm::Type::getInt8Ty(*s_Context));
	
	// Register JIT compiler support functions
	regNativeFunc("JITCompiler::callExceptHandler", (void*)JITCompiler::callExceptHandler, llvm::Type::getVoidTy(*s_Context), LLVMTypeVector(4, VOID_PTR_TYPE));
	
	// Register basic interpreter functions
	regNativeFunc("getBoolValue", (void*)getBoolValue, llvm::Type::getInt8Ty(*s_Context), LLVMTypeVector(1, VOID_PTR_TYPE), true, false, true);
	regNativeFunc("getInt64Value", (void*)getInt64Value, llvm::Type::getInt64Ty(*s_Context), LLVMTypeVector(1, VOID_PTR_TYPE), true, false, true);
	regNativeFunc("getFloat64Value", (void*)getFloat64Value, llvm::Type::getDoubleTy(*s_Context), LLVMTypeVector(1, VOID_PTR_TYPE), true, false, true);
	regNativeFunc("DataObject::convertType", (void*)DataObject::convertType, VOID_PTR_TYPE, convertArgs);
	regNativeFunc("ArrayObj::create", (void*)ArrayObj::create, VOID_PTR_TYPE, LLVMTypeVector(1, getIntType(sizeof(size_t))));
	regNativeFunc("ArrayObj::addObject", (void*)ArrayObj::addObject, llvm::Type::getVoidTy(*s_Context), evalArgs);
	regNativeFunc("ArrayObj::append", (void*)ArrayObj::append, llvm::Type::getVoidTy(*s_Context), evalArgs);
	regNativeFunc("ArrayObj::getArrayObj", (void*)ArrayObj::getArrayObj, VOID_PTR_TYPE, getObjArgs);
	regNativeFunc("ArrayObj::getArraySize", (void*)ArrayObj::getArraySize, getIntType(sizeof(size_t)), LLVMTypeVector(1, VOID_PTR_TYPE));
	regNativeFunc("Environment::bind", (void*)Environment::bind, VOID_PTR_TYPE, LLVMTypeVector(3, VOID_PTR_TYPE));
	regNativeFunc("Environment::lookup", (void*)Environment::lookup, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("Environment::extend", (void*)Environment::extend, VOID_PTR_TYPE, LLVMTypeVector(1, VOID_PTR_TYPE));
	regNativeFunc("ProgFunction::setLocalEnv", (void*)ProgFunction::setLocalEnv, llvm::Type::getVoidTy(*s_Context), evalArgs);
	regNativeFunc("ProgFunction::getLocalEnv", (void*)ProgFunction::getLocalEnv, VOID_PTR_TYPE, LLVMTypeVector(1, VOID_PTR_TYPE));
	regNativeFunc("RunError::throwError", (void*)RunError::throwError, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("Interpreter::callFunction", (void*)Interpreter::callFunction, VOID_PTR_TYPE, callFnArgs);
	regNativeFunc("Interpreter::execStatement", (void*)Interpreter::execStatement, llvm::Type::getVoidTy(*s_Context), evalArgs);
	regNativeFunc("Interpreter::evalAssignStmt", (void*)Interpreter::evalAssignStmt, llvm::Type::getVoidTy(*s_Context), evalArgs);
	regNativeFunc("Interpreter::assignObject", (void*)Interpreter::assignObject, llvm::Type::getVoidTy(*s_Context), assignArgs);
	regNativeFunc("Interpreter::evalExprStmt", (void*)Interpreter::evalExprStmt, llvm::Type::getVoidTy(*s_Context), evalArgs);
	regNativeFunc("Interpreter::evalExpression", (void*)Interpreter::evalExpression, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("Interpreter::evalSymbolExpr", (void*)Interpreter::evalSymbolExpr, VOID_PTR_TYPE, evalParamArgs);
	regNativeFunc("Interpreter::evalSymbol", (void*)Interpreter::evalSymbol, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("Interpreter::evalUnaryExpr", (void*)Interpreter::evalUnaryExpr, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("Interpreter::evalBinaryExpr", (void*)Interpreter::evalBinaryExpr, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("Interpreter::evalCellArrayExpr", (void*)Interpreter::evalCellArrayExpr, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("Interpreter::evalParamExpr", (void*)Interpreter::evalParamExpr, VOID_PTR_TYPE, evalParamArgs);
	regNativeFunc("Interpreter::evalCellIndexExpr", (void*)Interpreter::evalCellIndexExpr, VOID_PTR_TYPE, evalArgs);
	
	// Register matrix utility functions
	regNativeFunc("DataObject::copyObject", (void*)DataObject::copyObject, VOID_PTR_TYPE, LLVMTypeVector(1, VOID_PTR_TYPE), true, false, true);
	regNativeFunc("BaseMatrixObj::getDimCount", (void*)BaseMatrixObj::getDimCount, getIntType(sizeof(size_t)), LLVMTypeVector(1, VOID_PTR_TYPE), true, true, true);
	regNativeFunc("BaseMatrixObj::getSizeArray", (void*)BaseMatrixObj::getSizeArray, llvm::PointerType::getUnqual(getIntType(sizeof(size_t))), LLVMTypeVector(1, VOID_PTR_TYPE), true, true, true);
	regNativeFunc("BaseMatrixObj::expandMatrix", (void*)BaseMatrixObj::expandMatrix, llvm::Type::getVoidTy(*s_Context), expandArgs);
	regNativeFunc("MatrixF64Obj::makeScalar", (void*)MatrixF64Obj::makeScalar, VOID_PTR_TYPE, LLVMTypeVector(1, llvm::Type::getDoubleTy(*s_Context)));
	regNativeFunc("CharArrayObj::makeScalar", (void*)CharArrayObj::makeScalar, VOID_PTR_TYPE, LLVMTypeVector(1, llvm::Type::getInt8Ty(*s_Context)));
	regNativeFunc("LogicalArrayObj::makeScalar", (void*)LogicalArrayObj::makeScalar, VOID_PTR_TYPE, LLVMTypeVector(1, llvm::Type::getInt8Ty(*s_Context)));
	regNativeFunc("MatrixF64Obj::getScalarVal", (void*)MatrixF64Obj::getScalarVal, llvm::Type::getDoubleTy(*s_Context), LLVMTypeVector(1, VOID_PTR_TYPE), true, false, true);
	regNativeFunc("CharArrayObj::getScalarVal", (void*)CharArrayObj::getScalarVal, llvm::Type::getInt8Ty(*s_Context), LLVMTypeVector(1, VOID_PTR_TYPE), true, false, true);
	regNativeFunc("LogicalArrayObj::getScalarVal", (void*)LogicalArrayObj::getScalarVal, llvm::Type::getInt8Ty(*s_Context), LLVMTypeVector(1, VOID_PTR_TYPE), true, false, true);
	regNativeFunc("MatrixF64Obj::readElem1D", (void*)(MatrixF64Obj::MATRIX_1D_READ_FUNC)MatrixF64Obj::readElem1D, llvm::Type::getDoubleTy(*s_Context), read1DArgs);
	regNativeFunc("MatrixF64Obj::readElem2D", (void*)(MatrixF64Obj::MATRIX_2D_READ_FUNC)MatrixF64Obj::readElem2D, llvm::Type::getDoubleTy(*s_Context), read2DArgs);
	regNativeFunc("MatrixF64Obj::writeElem1D", (void*)(MatrixF64Obj::MATRIX_1D_WRITE_FUNC)MatrixF64Obj::writeElem1D, llvm::Type::getVoidTy(*s_Context), f64Write1DArgs);
	regNativeFunc("MatrixF64Obj::writeElem2D", (void*)(MatrixF64Obj::MATRIX_2D_WRITE_FUNC)MatrixF64Obj::writeElem2D, llvm::Type::getVoidTy(*s_Context), f64Write2DArgs);
	regNativeFunc("CharArrayObj::readElem1D", (void*)(CharArrayObj::MATRIX_1D_READ_FUNC)CharArrayObj::readElem1D, llvm::Type::getInt8Ty(*s_Context), read1DArgs);
	regNativeFunc("CharArrayObj::readElem2D", (void*)(CharArrayObj::MATRIX_2D_READ_FUNC)CharArrayObj::readElem2D, llvm::Type::getInt8Ty(*s_Context), read2DArgs);
	regNativeFunc("CharArrayObj::writeElem1D", (void*)(CharArrayObj::MATRIX_1D_WRITE_FUNC)CharArrayObj::writeElem1D, llvm::Type::getVoidTy(*s_Context), i8Write1DArgs);
	regNativeFunc("CharArrayObj::writeElem2D", (void*)(CharArrayObj::MATRIX_2D_WRITE_FUNC)CharArrayObj::writeElem2D, llvm::Type::getVoidTy(*s_Context), i8Write2DArgs);
	regNativeFunc("LogicalArrayObj::readElem1D", (void*)(LogicalArrayObj::MATRIX_1D_READ_FUNC)LogicalArrayObj::readElem1D, llvm::Type::getInt8Ty(*s_Context), read1DArgs);
	regNativeFunc("LogicalArrayObj::readElem2D", (void*)(LogicalArrayObj::MATRIX_2D_READ_FUNC)LogicalArrayObj::readElem2D, llvm::Type::getInt8Ty(*s_Context), read2DArgs);
	regNativeFunc("LogicalArrayObj::writeElem1D", (void*)(LogicalArrayObj::MATRIX_1D_WRITE_FUNC)LogicalArrayObj::writeElem1D, llvm::Type::getVoidTy(*s_Context), i8Write1DArgs);
	regNativeFunc("LogicalArrayObj::writeElem2D", (void*)(LogicalArrayObj::MATRIX_2D_WRITE_FUNC)LogicalArrayObj::writeElem2D, llvm::Type::getVoidTy(*s_Context), i8Write2DArgs);
	
	// Register addition operation (+) functions
	regNativeFunc("MatrixF64Obj::binArrayOp<AddOp>", (void*)(MatrixF64Obj::MATRIX_BINOP_FUNC)MatrixF64Obj::binArrayOp<AddOp<float64>, float64>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("MatrixF64Obj::scalarArrayOp<AddOp>", (void*)(MatrixF64Obj::F64_SCALAR_BINOP_FUNC)MatrixF64Obj::lhsScalarArrayOp<AddOp<float64>, float64, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("MatrixC128Obj::binArrayOp<AddOp>", (void*)(MatrixC128Obj::MATRIX_BINOP_FUNC)MatrixC128Obj::binArrayOp<AddOp<Complex128>, Complex128>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("MatrixC128Obj::scalarArrayOp<AddOp>", (void*)(MatrixC128Obj::F64_SCALAR_BINOP_FUNC)MatrixC128Obj::lhsScalarArrayOp<AddOp<Complex128>, Complex128, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("arrayArithOp<AddOp>", (void*)(MATRIX_BINOP_FUNC)arrayArithOp<AddOp>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("scalarArithOp<AddOp>", (void*)(SCALAR_BINOP_FUNC)lhsScalarArithOp<AddOp, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);

	// Register subtraction (-) operation functions
	regNativeFunc("MatrixF64Obj::binArrayOp<SubOp>", (void*)(MatrixF64Obj::MATRIX_BINOP_FUNC)MatrixF64Obj::binArrayOp<SubOp<float64>, float64>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("MatrixF64Obj::lhsScalarArrayOp<SubOp>", (void*)(MatrixF64Obj::F64_SCALAR_BINOP_FUNC)MatrixF64Obj::lhsScalarArrayOp<SubOp<float64>, float64, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("MatrixF64Obj::rhsScalarArrayOp<SubOp>", (void*)(MatrixF64Obj::F64_SCALAR_BINOP_FUNC)MatrixF64Obj::rhsScalarArrayOp<SubOp<float64>, float64, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("MatrixC128Obj::binArrayOp<SubOp>", (void*)(MatrixC128Obj::MATRIX_BINOP_FUNC)MatrixC128Obj::binArrayOp<SubOp<Complex128>, Complex128>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("MatrixC128Obj::lhsScalarArrayOp<SubOp>", (void*)(MatrixC128Obj::F64_SCALAR_BINOP_FUNC)MatrixC128Obj::lhsScalarArrayOp<SubOp<Complex128>, Complex128, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("MatrixC128Obj::rhsScalarArrayOp<SubOp>", (void*)(MatrixC128Obj::F64_SCALAR_BINOP_FUNC)MatrixC128Obj::rhsScalarArrayOp<SubOp<Complex128>, Complex128, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("arrayArithOp<SubOp>", (void*)(MATRIX_BINOP_FUNC)arrayArithOp<SubOp>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("lhsScalarArithOp<SubOp>", (void*)(SCALAR_BINOP_FUNC)lhsScalarArithOp<SubOp, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("rhsScalarArithOp<SubOp>", (void*)(SCALAR_BINOP_FUNC)rhsScalarArithOp<SubOp, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	
	// Register multiplication (*) and array multiplication (.*) operation functions
	regNativeFunc("MatrixF64Obj::binArrayOp<MultOp>", (void*)(MatrixF64Obj::MATRIX_BINOP_FUNC)MatrixF64Obj::binArrayOp<MultOp<float64>, float64>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("MatrixF64Obj::scalarMult", (void*)(MatrixF64Obj::F64_SCALAR_BINOP_FUNC)MatrixF64Obj::scalarMult<float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("MatrixF64Obj::matrixMult", (void*)(MatrixF64Obj::MATRIX_BINOP_FUNC)MatrixF64Obj::matrixMult, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("MatrixC128Obj::binArrayOp<MultOp>", (void*)(MatrixC128Obj::MATRIX_BINOP_FUNC)MatrixC128Obj::binArrayOp<MultOp<Complex128>, Complex128>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("MatrixC128Obj::scalarMult", (void*)(MatrixC128Obj::F64_SCALAR_BINOP_FUNC)MatrixC128Obj::scalarMult<float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("MatrixC128Obj::matrixMult", (void*)(MatrixC128Obj::MATRIX_BINOP_FUNC)MatrixC128Obj::matrixMult, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("arrayArithOp<MultOp>", (void*)(MATRIX_BINOP_FUNC)arrayArithOp<MultOp>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("scalarMultOp", (void*)(SCALAR_BINOP_FUNC)scalarMultOp, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("matrixMultOp", (void*)(MATRIX_BINOP_FUNC)matrixMultOp, VOID_PTR_TYPE, evalArgs);
		
	// Register division (/) and array-division (./) operation functions
	regNativeFunc("DivOp<float64>::op", (void*)DivOp<float64>::op, llvm::Type::getDoubleTy(*s_Context), f64BinOpArgs);
	regNativeFunc("MatrixF64Obj::binArrayOp<DivOp>", (void*)(MatrixF64Obj::MATRIX_BINOP_FUNC)MatrixF64Obj::binArrayOp<DivOp<float64>, float64>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("MatrixF64Obj::lhsScalarArrayOp<DivOp>", (void*)(MatrixF64Obj::F64_SCALAR_BINOP_FUNC)MatrixF64Obj::lhsScalarArrayOp<DivOp<float64>, float64, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("MatrixF64Obj::rhsScalarArrayOp<DivOp>", (void*)(MatrixF64Obj::F64_SCALAR_BINOP_FUNC)MatrixF64Obj::rhsScalarArrayOp<DivOp<float64>, float64, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("MatrixF64Obj::matrixRightDiv", (void*)(MatrixF64Obj::MATRIX_BINOP_FUNC)MatrixF64Obj::matrixRightDiv, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("MatrixC128Obj::binArrayOp<DivOp>", (void*)(MatrixC128Obj::MATRIX_BINOP_FUNC)MatrixC128Obj::binArrayOp<DivOp<Complex128>, Complex128>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("MatrixC128Obj::lhsScalarArrayOp<DivOp>", (void*)(MatrixC128Obj::F64_SCALAR_BINOP_FUNC)MatrixC128Obj::lhsScalarArrayOp<DivOp<Complex128>, Complex128, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("MatrixC128Obj::rhsScalarArrayOp<DivOp>", (void*)(MatrixC128Obj::F64_SCALAR_BINOP_FUNC)MatrixC128Obj::rhsScalarArrayOp<DivOp<Complex128>, Complex128, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("MatrixC128Obj::matrixRightDiv", (void*)(MatrixC128Obj::MATRIX_BINOP_FUNC)MatrixC128Obj::matrixRightDiv, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("arrayArithOp<DivOp>", (void*)(MATRIX_BINOP_FUNC)arrayArithOp<DivOp>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("lhsScalarArithOp<DivOp>", (void*)(SCALAR_BINOP_FUNC)lhsScalarArithOp<DivOp, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("rhsScalarArithOp<DivOp>", (void*)(SCALAR_BINOP_FUNC)rhsScalarArithOp<DivOp, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("matrixRightDivOp", (void*)(MATRIX_BINOP_FUNC)matrixRightDivOp, VOID_PTR_TYPE, evalArgs);
	
	// Register the array-OR (|) operation functions
	regNativeFunc("MatrixF64Obj::binArrayOp<OrOp>", (void*)(MatrixF64Obj::MATRIX_LOGIC_OP_FUNC)MatrixF64Obj::binArrayOp<OrOp<float64>, bool>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("MatrixF64Obj::scalarArrayOp<OrOp>", (void*)(MatrixF64Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixF64Obj::lhsScalarArrayOp<OrOp<float64>, bool, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("MatrixF64Obj::MatrixC128Obj::binArrayOp<OrOp>", (void*)(MatrixC128Obj::MATRIX_LOGIC_OP_FUNC)MatrixC128Obj::binArrayOp<OrOp<Complex128>, bool>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("MatrixC128Obj::scalarArrayOp<OrOp>", (void*)(MatrixC128Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixC128Obj::lhsScalarArrayOp<OrOp<Complex128>, bool, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("LogicalArrayObj::binArrayOp<OrOp>", (void*)(LogicalArrayObj::MATRIX_LOGIC_OP_FUNC)LogicalArrayObj::binArrayOp<OrOp<bool>, bool>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("LogicalArrayObj::scalarArrayOp<OrOp>", (void*)(LogicalArrayObj::F64_SCALAR_LOGIC_OP_FUNC)LogicalArrayObj::lhsScalarArrayOp<OrOp<bool>, bool, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("CharArrayObj::binArrayOp<OrOp>", (void*)(CharArrayObj::MATRIX_LOGIC_OP_FUNC)CharArrayObj::binArrayOp<OrOp<char>, bool>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("CharArrayObj::scalarArrayOp<OrOp>", (void*)(CharArrayObj::F64_SCALAR_LOGIC_OP_FUNC)CharArrayObj::lhsScalarArrayOp<OrOp<char>, bool, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);	
	regNativeFunc("matrixLogicOp<OrOp>", (void*)(MATRIX_BINOP_FUNC)matrixLogicOp<OrOp>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("scalarLogicOp<OrOp>", (void*)(SCALAR_BINOP_FUNC)lhsScalarLogicOp<OrOp, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	
	// Register the array-AND (&) operation functions
	regNativeFunc("MatrixF64Obj::binArrayOp<AndOp>", (void*)(MatrixF64Obj::MATRIX_LOGIC_OP_FUNC)MatrixF64Obj::binArrayOp<AndOp<float64>, bool>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("MatrixF64Obj::scalarArrayOp<AndOp>", (void*)(MatrixF64Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixF64Obj::lhsScalarArrayOp<AndOp<float64>, bool, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("MatrixF64Obj::MatrixC128Obj::binArrayOp<AndOp>", (void*)(MatrixC128Obj::MATRIX_LOGIC_OP_FUNC)MatrixC128Obj::binArrayOp<AndOp<Complex128>, bool>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("MatrixC128Obj::scalarArrayOp<AndOp>", (void*)(MatrixC128Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixC128Obj::lhsScalarArrayOp<AndOp<Complex128>, bool, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("LogicalArrayObj::binArrayOp<AndOp>", (void*)(LogicalArrayObj::MATRIX_LOGIC_OP_FUNC)LogicalArrayObj::binArrayOp<AndOp<bool>, bool>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("LogicalArrayObj::scalarArrayOp<AndOp>", (void*)(LogicalArrayObj::F64_SCALAR_LOGIC_OP_FUNC)LogicalArrayObj::lhsScalarArrayOp<AndOp<bool>, bool, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("CharArrayObj::binArrayOp<AndOp>", (void*)(CharArrayObj::MATRIX_LOGIC_OP_FUNC)CharArrayObj::binArrayOp<AndOp<char>, bool>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("CharArrayObj::scalarArrayOp<AndOp>", (void*)(CharArrayObj::F64_SCALAR_LOGIC_OP_FUNC)CharArrayObj::lhsScalarArrayOp<AndOp<char>, bool, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);	
	regNativeFunc("matrixLogicOp<AndOp>", (void*)(MATRIX_BINOP_FUNC)matrixLogicOp<AndOp>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("scalarLogicOp<AndOp>", (void*)(SCALAR_BINOP_FUNC)lhsScalarLogicOp<AndOp, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	
	// Register equality (==) comparison operation functions
	regNativeFunc("MatrixF64Obj::binArrayOp<EqualOp>", (void*)(MatrixF64Obj::MATRIX_LOGIC_OP_FUNC)MatrixF64Obj::binArrayOp<EqualOp<float64>, bool>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("MatrixF64Obj::scalarArrayOp<EqualOp>", (void*)(MatrixF64Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixF64Obj::lhsScalarArrayOp<EqualOp<float64>, bool, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("MatrixF64Obj::MatrixC128Obj::binArrayOp<EqualOp>", (void*)(MatrixC128Obj::MATRIX_LOGIC_OP_FUNC)MatrixC128Obj::binArrayOp<EqualOp<Complex128>, bool>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("MatrixC128Obj::scalarArrayOp<EqualOp>", (void*)(MatrixC128Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixC128Obj::lhsScalarArrayOp<EqualOp<Complex128>, bool, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("LogicalArrayObj::binArrayOp<EqualOp>", (void*)(LogicalArrayObj::MATRIX_LOGIC_OP_FUNC)LogicalArrayObj::binArrayOp<EqualOp<bool>, bool>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("LogicalArrayObj::scalarArrayOp<EqualOp>", (void*)(LogicalArrayObj::F64_SCALAR_LOGIC_OP_FUNC)LogicalArrayObj::lhsScalarArrayOp<EqualOp<bool>, bool, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("CharArrayObj::binArrayOp<EqualOp>", (void*)(CharArrayObj::MATRIX_LOGIC_OP_FUNC)CharArrayObj::binArrayOp<EqualOp<char>, bool>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("CharArrayObj::scalarArrayOp<EqualOp>", (void*)(CharArrayObj::F64_SCALAR_LOGIC_OP_FUNC)CharArrayObj::lhsScalarArrayOp<EqualOp<char>, bool, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);	
	regNativeFunc("matrixLogicOp<EqualOp>", (void*)(MATRIX_BINOP_FUNC)matrixLogicOp<EqualOp>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("scalarLogicOp<EqualOp>", (void*)(SCALAR_BINOP_FUNC)lhsScalarLogicOp<EqualOp, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);

	// Register greater-than (>) comparison operation functions
	regNativeFunc("MatrixF64Obj::binArrayOp<GreaterThanOp>", (void*)(MatrixF64Obj::MATRIX_LOGIC_OP_FUNC)MatrixF64Obj::binArrayOp<GreaterThanOp<float64>, bool>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("MatrixF64Obj::lhsScalarArrayOp<GreaterThanOp>", (void*)(MatrixF64Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixF64Obj::lhsScalarArrayOp<GreaterThanOp<float64>, bool, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("MatrixC128Obj::binArrayOp<GreaterThanOp>", (void*)(MatrixC128Obj::MATRIX_LOGIC_OP_FUNC)MatrixC128Obj::binArrayOp<GreaterThanOp<Complex128>, bool>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("MatrixC128Obj::lhsScalarArrayOp<GreaterThanOp>", (void*)(MatrixC128Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixC128Obj::lhsScalarArrayOp<GreaterThanOp<Complex128>, bool, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("CharArrayObj::binArrayOp<GreaterThanOp>", (void*)(CharArrayObj::MATRIX_LOGIC_OP_FUNC)CharArrayObj::binArrayOp<GreaterThanOp<char>, bool>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("CharArrayObj::lhsScalarArrayOp<GreaterThanOp>", (void*)(CharArrayObj::F64_SCALAR_LOGIC_OP_FUNC)CharArrayObj::lhsScalarArrayOp<GreaterThanOp<char>, bool, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);	
	regNativeFunc("matrixLogicOp<GreaterThanOp>", (void*)(MATRIX_BINOP_FUNC)matrixLogicOp<GreaterThanOp>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("lhsScalarLogicOp<GreaterThanOp>", (void*)(SCALAR_BINOP_FUNC)lhsScalarLogicOp<GreaterThanOp, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);

	// Register less-than (<) comparison operation functions
	regNativeFunc("MatrixF64Obj::binArrayOp<LessThanOp>", (void*)(MatrixF64Obj::MATRIX_LOGIC_OP_FUNC)MatrixF64Obj::binArrayOp<LessThanOp<float64>, bool>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("MatrixF64Obj::lhsScalarArrayOp<LessThanOp>", (void*)(MatrixF64Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixF64Obj::lhsScalarArrayOp<LessThanOp<float64>, bool, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("MatrixC128Obj::binArrayOp<LessThanOp>", (void*)(MatrixC128Obj::MATRIX_LOGIC_OP_FUNC)MatrixC128Obj::binArrayOp<LessThanOp<Complex128>, bool>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("MatrixC128Obj::lhsScalarArrayOp<LessThanOp>", (void*)(MatrixC128Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixC128Obj::lhsScalarArrayOp<LessThanOp<Complex128>, bool, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("CharArrayObj::binArrayOp<LessThanOp>", (void*)(CharArrayObj::MATRIX_LOGIC_OP_FUNC)CharArrayObj::binArrayOp<LessThanOp<char>, bool>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("CharArrayObj::lhsScalarArrayOp<LessThanOp>", (void*)(CharArrayObj::F64_SCALAR_LOGIC_OP_FUNC)CharArrayObj::lhsScalarArrayOp<LessThanOp<char>, bool, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);	
	regNativeFunc("matrixLogicOp<LessThanOp>", (void*)(MATRIX_BINOP_FUNC)matrixLogicOp<LessThanOp>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("lhsScalarLogicOp<LessThanOp>", (void*)(SCALAR_BINOP_FUNC)lhsScalarLogicOp<LessThanOp, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	
	// Register greater-than-or-equal (>=) comparison operation functions
	regNativeFunc("MatrixF64Obj::binArrayOp<GreaterThanEqOp>", (void*)(MatrixF64Obj::MATRIX_LOGIC_OP_FUNC)MatrixF64Obj::binArrayOp<GreaterThanEqOp<float64>, bool>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("MatrixF64Obj::lhsScalarArrayOp<GreaterThanEqOp>", (void*)(MatrixF64Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixF64Obj::lhsScalarArrayOp<GreaterThanEqOp<float64>, bool, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("MatrixC128Obj::binArrayOp<GreaterThanEqOp>", (void*)(MatrixC128Obj::MATRIX_LOGIC_OP_FUNC)MatrixC128Obj::binArrayOp<GreaterThanEqOp<Complex128>, bool>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("MatrixC128Obj::lhsScalarArrayOp<GreaterThanEqOp>", (void*)(MatrixC128Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixC128Obj::lhsScalarArrayOp<GreaterThanEqOp<Complex128>, bool, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("CharArrayObj::binArrayOp<GreaterThanEqOp>", (void*)(CharArrayObj::MATRIX_LOGIC_OP_FUNC)CharArrayObj::binArrayOp<GreaterThanEqOp<char>, bool>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("CharArrayObj::lhsScalarArrayOp<GreaterThanEqOp>", (void*)(CharArrayObj::F64_SCALAR_LOGIC_OP_FUNC)CharArrayObj::lhsScalarArrayOp<GreaterThanEqOp<char>, bool, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);	
	regNativeFunc("matrixLogicOp<GreaterThanEqOp>", (void*)(MATRIX_BINOP_FUNC)matrixLogicOp<GreaterThanEqOp>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("lhsScalarLogicOp<GreaterThanEqOp>", (void*)(SCALAR_BINOP_FUNC)lhsScalarLogicOp<GreaterThanEqOp, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	
	// Register less-than-or-equal (<=) comparison operation functions
	regNativeFunc("MatrixF64Obj::binArrayOp<LessThanEqOp>", (void*)(MatrixF64Obj::MATRIX_LOGIC_OP_FUNC)MatrixF64Obj::binArrayOp<LessThanEqOp<float64>, bool>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("MatrixF64Obj::lhsScalarArrayOp<LessThanEqOp>", (void*)(MatrixF64Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixF64Obj::lhsScalarArrayOp<LessThanEqOp<float64>, bool, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("MatrixC128Obj::binArrayOp<LessThanEqOp>", (void*)(MatrixC128Obj::MATRIX_LOGIC_OP_FUNC)MatrixC128Obj::binArrayOp<LessThanEqOp<Complex128>, bool>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("MatrixC128Obj::lhsScalarArrayOp<LessThanEqOp>", (void*)(MatrixC128Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixC128Obj::lhsScalarArrayOp<LessThanEqOp<Complex128>, bool, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	regNativeFunc("CharArrayObj::binArrayOp<LessThanEqOp>", (void*)(CharArrayObj::MATRIX_LOGIC_OP_FUNC)CharArrayObj::binArrayOp<LessThanEqOp<char>, bool>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("CharArrayObj::lhsScalarArrayOp<LessThanEqOp>", (void*)(CharArrayObj::F64_SCALAR_LOGIC_OP_FUNC)CharArrayObj::lhsScalarArrayOp<LessThanEqOp<char>, bool, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);	
	regNativeFunc("matrixLogicOp<LessThanEqOp>", (void*)(MATRIX_BINOP_FUNC)matrixLogicOp<LessThanEqOp>, VOID_PTR_TYPE, evalArgs);
	regNativeFunc("lhsScalarLogicOp<LessThanEqOp>", (void*)(SCALAR_BINOP_FUNC)lhsScalarLogicOp<LessThanEqOp, float64>, VOID_PTR_TYPE, f64ScalarOpArgs);
	
	// Register the local config variables
	ConfigManager::registerVar(&s_jitEnableVar);
	ConfigManager::registerVar(&s_jitUseArrayOpts);
	ConfigManager::registerVar(&s_jitUseBinOpOpts);
	ConfigManager::registerVar(&s_jitUseLibOpts);
	ConfigManager::registerVar(&s_jitUseDirectCalls);
	ConfigManager::registerVar(&s_jitNoReadBoundChecks);
	ConfigManager::registerVar(&s_jitNoWriteBoundChecks);
	ConfigManager::registerVar(&s_jitCopyEnableVar);
    ConfigManager::registerVar(&s_jitOsrEnableVar);
    ConfigManager::registerVar(&s_jitOsrStrategyVar);
}

/***************************************************************
 * Function: JITCompiler::initializeOSR()
 * Purpose : Initialize OSR data structures ... 
 * Initial : Nurudeen Abiodun Lameed on February 20, 2012
 ****************************************************************
Revisions and bug fixes:                              
*/
void JITCompiler::initializeOSR() {
  // initialize osr logic data structures
  if (!s_jitOsrEnableVar)
    return;
}


/***************************************************************
* Function: JITCompiler::shutdown()
* Purpose : Shut down the JIT compiler
* Initial : Maxime Chevalier-Boisvert on March 9, 2009
****************************************************************
Revisions and bug fixes:Revised for LLVM2.8/LLVM3.0 compatibility;
N.A. Lameed, 2011, 2012.
*/
void JITCompiler::shutdown()
{
	delete s_pFunctionPasses;
	delete s_pPrintPass;

	if (s_pOsrInfoPass) delete s_pOsrInfoPass;

    // free machine code 
    for(llvm::Module::iterator I = s_pModule->begin(),
          E = s_pModule->end(); I != E; ++I) {	
      s_pExecEngine->freeMachineCodeForFunction(I);
    }

    delete s_pExecEngine;
    llvm::llvm_shutdown();
}

/***************************************************************
* Function: JITCompiler::regNativeFunc()
* Purpose : Register a native function
* Initial : Maxime Chevalier-Boisvert on March 9, 2009
****************************************************************
Revisions and bug fixes:
*/
void JITCompiler::regNativeFunc(
	const std::string& name,
	void* pFuncPtr,
	llvm::Type* returnType,
	const LLVMTypeVector& argTypes,
	bool noMemWrites,
	bool noMemAccess,
	bool noThrows
)
{
	// Ensure that the JIT compiler was initialized
	assert (s_pModule != NULL && s_pExecEngine != NULL);
	
	// Ensure that the function is not already registered
	assert (s_nativeMap.find(pFuncPtr) == s_nativeMap.end());
	
    // Create a function type object for the function
    llvm::FunctionType* pFuncType = llvm::FunctionType::get(returnType, argTypes, false);
    
    // Create a function object with external linkage and the specified function type
    llvm::Function* pFuncObj = llvm::Function::Create(
    	pFuncType,
    	llvm::Function::ExternalLinkage,
    	name,
    	s_pModule
    ); 
    
    // Add a global mapping in the execution engine for the function pointer
    s_pExecEngine->addGlobalMapping(pFuncObj, pFuncPtr); 

    // Set the function attribute flags
    if (noMemWrites)
        pFuncObj->setOnlyReadsMemory();

    if (noMemAccess)
        pFuncObj->setDoesNotAccessMemory();
    
    if (noThrows)
        pFuncObj->setDoesNotThrow();

	// Create a native function object and store information about the function
	NativeFunc nativeFunc;
	nativeFunc.name = name;
	nativeFunc.returnType = returnType;
	nativeFunc.argTypes = argTypes;
	nativeFunc.pLLVMFunc = pFuncObj;

	// Add an entry in the native function map for this function
	s_nativeMap[pFuncPtr] = nativeFunc;
}

/***************************************************************
* Function: JITCompiler::regLibraryFunc()
* Purpose : Register an optimized library function
* Initial : Maxime Chevalier-Boisvert on July 29, 2009
****************************************************************
Revisions and bug fixes:
*/
void JITCompiler::regLibraryFunc(
	const LibFunction* pLibFunc,
	void* pFuncPtr,
	const TypeSetString& inputTypes,
	const TypeSet& returnType,
	bool noMemWrites,
	bool noMemAccess,
	bool noThrows	
)
{
	// Create a key object for this function
	LibFuncKey key(pLibFunc, inputTypes, returnType);
	
	// Ensure that the function is not already registered
	assert (s_libFuncMap.find(key) == s_libFuncMap.end());
		
	// Declare a vector for the input storage modes
	LLVMTypeVector inTypes;
	
	// For each input type set
	for (size_t i = 0; i < inputTypes.size(); ++i)
	{
		// Get the storage mode for the argument type
		DataObject::Type argObjType;
		llvm::Type* argType = getStorageMode(
			inputTypes[i],
			argObjType
		);
		
		// Add the argument type to the list
		inTypes.push_back(argType);
	}
	
	// Get the storage mode for the return type
	DataObject::Type retObjType;
	llvm::Type* retType = getStorageMode(
		returnType,
		retObjType
	);
	
	// Register the native function
	regNativeFunc(
		pLibFunc->getFuncName(),
		pFuncPtr,
		retType,
		inTypes,
		noMemWrites,
		noMemAccess,
		noThrows
	);
	
	// Store a pointer to the native function in the optimized library function map
	s_libFuncMap[key] = pFuncPtr;
}

/***************************************************************
* Function: JITCompiler::regLibraryFunc()
* Purpose : Set up parameters used by compileFunction
* Initial : Maxime Chevalier-Boisvert on July 29, 2009
****************************************************************
Revisions and bug fixes: 2010.
*/
void JITCompiler::setUpParameters(ProgFunction* pFunction, CompFunction& compFunction,  CompVersion& compVersion,
		const TypeSetString& argTypeStr)
{
// For each input argument type set
	for (size_t i = 0; i < argTypeStr.size(); ++i)
	{
		// Get the storage mode for this argument
		DataObject::Type objectType;
		llvm::Type* storageMode = getStorageMode( argTypeStr[i], objectType);
		
		// Store the argument storage mode and object type
		compVersion.inArgStoreModes.push_back(storageMode);
		compVersion.inArgObjTypes.push_back(objectType);
	}
	
	// Add a last argument for the "nargout" value
	compVersion.inArgStoreModes.push_back(llvm::Type::getInt64Ty(*s_Context));
	compVersion.inArgObjTypes.push_back(DataObject::MATRIX_F64);
	
	// Create a struct type for the input parameters
	compVersion.pInStructType = llvm::StructType::get(*s_Context, compVersion.inArgStoreModes, false);
	
	const ProgFunction::ParamVector& inParams = pFunction->getInParams();
	const ProgFunction::ParamVector& outParams = pFunction->getOutParams();
	
	// Find the variable type map for after the function body
	TypeInfoMap::const_iterator typeInfoItr = compVersion.pTypeInferInfo->postTypeMap.find(compFunction.pFuncBody);
	assert (typeInfoItr != compVersion.pTypeInferInfo->postTypeMap.end());
	const VarTypeMap& varTypes = typeInfoItr->second;
	
	// For each output parameter
	for (size_t i = 0; i < outParams.size(); ++i)
	{
		// Get the symbol for the return parameter
		SymbolExpr* pSymbol = outParams[i];
		
		// Find the type set for this variable
		VarTypeMap::const_iterator varTypeItr = varTypes.find(pSymbol);
		TypeSet typeSet = (varTypeItr != varTypes.end())? varTypeItr->second:TypeSet();	
		
		// Get the storage mode for the return parameter
		DataObject::Type objType;
		llvm::Type* storageMode = getStorageMode(typeSet, objType);
		
		// Store the storage mode and object type
		compVersion.outArgStoreModes.push_back(storageMode);
		compVersion.outArgObjTypes.push_back(objType);
	}	
		
	// Add an integer value to the output struct for the number of output parameters set
	compVersion.outArgStoreModes.push_back(llvm::Type::getInt64Ty(*s_Context));
	compVersion.outArgObjTypes.push_back(DataObject::MATRIX_F64);
	
	// Create a struct type for the output parameters
	compVersion.pOutStructType = llvm::StructType::get(*s_Context, compVersion.outArgStoreModes, false);
}

void JITCompiler::compFuncReturn(llvm::IRBuilder<>& exitBuilder, CompFunction& compFunction, 
		CompVersion& compVersion, VariableMap& exitVarMap, const ProgFunction::ParamVector& outParams)
{
	llvm::Function* pFuncObj = compVersion.pLLVMFunc;
	// Get a pointer to the return data structure
	llvm::Value* pRetStruct = ++(pFuncObj->arg_begin());
	
	// Create an initial basic block
	llvm::BasicBlock* pCurrentBlock = llvm::BasicBlock::Create(*s_Context, "", pFuncObj);
	llvm::IRBuilder<> currentBuilder(pCurrentBlock);		
	
	// Jump from the exit block to the current basic block
	exitBuilder.CreateBr(pCurrentBlock);
			
    // Create a basic block for the function return
	llvm::BasicBlock* pRetBlock = llvm::BasicBlock::Create(*s_Context, "ret", pFuncObj);
	llvm::IRBuilder<> retBuilder(pRetBlock);
	
	// Create a return void instruction
	retBuilder.CreateRetVoid();
	
	// For each output parameter
	for (size_t i = 0; i < outParams.size(); ++i)
	{
		// Get the symbol for the return parameter
		SymbolExpr* pSymbol = outParams[i];
		
		// Lookup the symbol in the exit variable map
		VariableMap::iterator varItr = exitVarMap.find(pSymbol);
	
		// Declare a variable for the variable value
		llvm::Value* pValue = NULL;
		
	    // Create a basic block to write the current value to the structure
		llvm::BasicBlock* pWriteBlock = llvm::BasicBlock::Create(*s_Context, "", pFuncObj);
		llvm::IRBuilder<> writeBuilder(pWriteBlock);
				
		// If the value is locally stored
		if (varItr != exitVarMap.end() && varItr->second.pValue != NULL)
		{	
			// Convert the value to the appropriate storage mode
			pValue = changeStorageMode(
				currentBuilder,
				varItr->second.pValue,
				varItr->second.objType,
				compVersion.outArgStoreModes[i]
			);
			
			// Jump to the struct writing block
			currentBuilder.CreateBr(pWriteBlock);
		}
		else
		{	
			// Lookup the variable in the environment
			LLVMValueVector lookupArgs;
			lookupArgs.push_back(getCallEnv(compFunction, compVersion));
			lookupArgs.push_back(createPtrConst(pSymbol));
			llvm::Value* pReadValue = createNativeCall(
				currentBuilder,
				(void*)Environment::lookup,
				lookupArgs
			);
			
			// Compare the pointer value to NULL
			llvm::Value* pPtrVal = currentBuilder.CreatePtrToInt(pReadValue, getIntType(PLATFORM_POINTER_SIZE));
			llvm::Value* pNULLVal = llvm::ConstantInt::get(getIntType(PLATFORM_POINTER_SIZE), NULL);
			llvm::Value* pCompVal = currentBuilder.CreateICmpEQ(pPtrVal, pNULLVal);	
			
		    // Create a basic block to set the number of values written
			llvm::BasicBlock* pSetNumBlock = llvm::BasicBlock::Create(*s_Context, "", pFuncObj);
			llvm::IRBuilder<> setBuilder(pSetNumBlock);
			
			// Get the address of the number struct field
			llvm::Value* gepValuesNum[2];
			gepValuesNum[0] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*s_Context), 0);
			gepValuesNum[1] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*s_Context), outParams.size());
            llvm::ArrayRef<llvm::Value*> gepValuesRef(gepValuesNum, gepValuesNum+2);
			llvm::Value* pNumFieldAddr = setBuilder.CreateGEP(pRetStruct, gepValuesRef);
			
			// Store the number of values written into the output structure
			llvm::Value* pNumWritten = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*s_Context), i);
			setBuilder.CreateStore(pNumWritten, pNumFieldAddr);

			// Jump to the return block
			setBuilder.CreateBr(pRetBlock);				
			
		    // Create a basic block to convert the value type
			llvm::BasicBlock* pConvBlock = llvm::BasicBlock::Create(*s_Context, "", pFuncObj);
			llvm::IRBuilder<> convBuilder(pConvBlock);	
			
			// Convert the value to the appropriate storage mode
			pValue = changeStorageMode(
				convBuilder,
				pReadValue,
				compVersion.outArgObjTypes[i],
				compVersion.outArgStoreModes[i]
			);
			
			// Jump to the struct writing block
			convBuilder.CreateBr(pWriteBlock);
			
			// If the pointer is null, stop the process
			currentBuilder.CreateCondBr(pCompVal, pSetNumBlock, pConvBlock);
		}
		
		// Get the address of the struct field
		llvm::Value* gepValuesVal[2];
		gepValuesVal[0] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*s_Context), 0);
		gepValuesVal[1] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*s_Context), i);
        llvm::ArrayRef<llvm::Value*> gepValRef(gepValuesVal, gepValuesVal+2);
		llvm::Value* pValFieldAddr = writeBuilder.CreateGEP(pRetStruct, gepValRef);
						
		// Store the value in the output structure
		writeBuilder.CreateStore(pValue, pValFieldAddr);	
		
		// If this is the last output parameter
		if (i == outParams.size() - 1)
		{
			// Get the address of the number struct field
			llvm::Value* gepValuesNum[2];
			gepValuesNum[0] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*s_Context), 0);
			gepValuesNum[1] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*s_Context), outParams.size());
            llvm::ArrayRef<llvm::Value*> gepValuesRef(gepValuesNum, gepValuesNum+2);
			llvm::Value* pNumFieldAddr = writeBuilder.CreateGEP(pRetStruct, gepValuesRef);
			
			// Store the number of values written into the output structure
			llvm::Value* pNumWritten = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*s_Context), outParams.size());
			writeBuilder.CreateStore(pNumWritten, pNumFieldAddr);

			// Jump to the return block
			writeBuilder.CreateBr(pRetBlock);
		}
		else
		{
			// Create a new current basic block
			pCurrentBlock = llvm::BasicBlock::Create(*s_Context, "", pFuncObj);
			currentBuilder.SetInsertPoint(pCurrentBlock);
			
			// Jump from the write block to the new current basic block
			writeBuilder.CreateBr(pCurrentBlock);
		}
	}
}

/***************************************************************
* Function: JITCompiler::compileFunction()
* Purpose : Compile a program function given argument types
* Initial : Maxime Chevalier-Boisvert on April 28, 2009
****************************************************************
Revisions and bug fixes:
*/
void JITCompiler::compileFunction(ProgFunction* pFunction, const TypeSetString& argTypeStr)
{
	// Ensure that the JIT compiler was initialized
	assert (s_pModule != NULL && s_pExecEngine != NULL);
	
	PROF_START_TIMER(Profiler::COMP_TIME_TOTAL);
	
	// Log that we are compiling this function
	std::cout << "Compiling function: \"" << pFunction->getFuncName() << "\"" << std::endl;

	// Attempt to find the function in the function map
	FunctionMap::iterator funcItr = s_functionMap.find(pFunction);
	
	// If there is no entry for this function
	if (funcItr == s_functionMap.end())
	{
		// Create a compiled function object
		CompFunction compFunction;
		
		// Get the current function body
		StmtSequence* pFuncBody = pFunction->getCurrentBody();
	
		// Transform the function body to split form
		pFuncBody = transformLogic(pFuncBody, pFunction);
		pFuncBody = splitSequence(pFuncBody, pFunction);
	
		// Store a pointer to the function object
		compFunction.pProgFunc = pFunction;
		
		// Store the transformed function body
		compFunction.pFuncBody = pFuncBody;
		
		// Update the function's current body
		// NOTE: this will avoid analyses running twice for two different function bodies
		pFunction->setCurrentBody(pFuncBody);
		s_functionMap.insert(std::pair<ProgFunction*, CompFunction>(pFunction, compFunction));
		funcItr = s_functionMap.find(pFunction);
		
		PROF_INCR_COUNTER(Profiler::FUNC_COMP_COUNT);
	}
	
	PROF_INCR_COUNTER(Profiler::FUNC_VERS_COUNT);
	
	// Get a reference to the compiled function object
	CompFunction& compFunction = funcItr->second;

	// If there is already an entry for this function version
	if (compFunction.versions.find(argTypeStr) != compFunction.versions.end())
	{
		// Throw a compilation error exception
		throw CompError("Function version is already compiled");
	}
	
	// Create an entry for this function version
	CompVersion& compVersion = compFunction.versions[argTypeStr];
	
	// Store the input argument types
	compVersion.inArgTypes = argTypeStr;
	
	PROF_START_TIMER(Profiler::ANA_TIME_TOTAL);
	
	// Perform analyses  the function body
	compVersion.pReachDefInfo = (const ReachDefInfo*)AnalysisManager::requestInfo(
		&computeReachDefs, pFunction, compFunction.pFuncBody, compVersion.inArgTypes);
	
	compVersion.pLiveVarInfo = (const LiveVarInfo*)AnalysisManager::requestInfo(&computeLiveVars,
		pFunction, compFunction.pFuncBody, compVersion.inArgTypes);
	
	compVersion.pTypeInferInfo = (const TypeInferInfo*)AnalysisManager::requestInfo(&computeTypeInfo,
		pFunction, compFunction.pFuncBody, compVersion.inArgTypes);
	
	compVersion.pMetricsInfo = (const MetricsInfo*)AnalysisManager::requestInfo(&computeMetrics,
		pFunction, compFunction.pFuncBody, compVersion.inArgTypes);

	compVersion.pBoundsCheckInfo = (const BoundsCheckInfo*)AnalysisManager::requestInfo(&computeBoundsCheck,
		pFunction, compFunction.pFuncBody, compVersion.inArgTypes);
	
	if (s_jitCopyEnableVar)
	{
		compVersion.pArrayCopyInfo = (const ArrayCopyAnalysisInfo*)AnalysisManager::requestInfo(
		&ArrayCopyElim::computeArrayCopyElim, pFunction, compFunction.pFuncBody, compVersion.inArgTypes);
	}

    //    std::cout << "IIR: \n" << pFunction->toString() << "\n";
	
	PROF_STOP_TIMER(Profiler::ANA_TIME_TOTAL);
	
	// If we are in verbose mode, log that the analyses are complete
	if (ConfigManager::s_verboseVar)
		std::cout << "Analysis process complete" << std::endl;

	// Create a name string for the function
	std::string funcName = pFunction->getFuncName() + "_" + ::toString((void*)compVersion.pTypeInferInfo);
	
	// Get the input and output parameters for the function
	const ProgFunction::ParamVector& inParams = pFunction->getInParams();
	const ProgFunction::ParamVector& outParams = pFunction->getOutParams();
	Expression::SymbolSet outParamSet(outParams.begin(), outParams.end());	
	
	setUpParameters(pFunction, compFunction, compVersion, argTypeStr);
	
	// Create a type vector for the function input argument types
	LLVMTypeVector inArgTypes;
	
	// Add a pointer to the input data structure to the input types
	inArgTypes.push_back(llvm::PointerType::getUnqual(compVersion.pInStructType));
	
	// Add a pointer to the output data structure to the input types
	inArgTypes.push_back(llvm::PointerType::getUnqual(compVersion.pOutStructType));
	
	// Get a function type object with the appropriate signature
	llvm::FunctionType* pFuncType = llvm::FunctionType::get(llvm::Type::getVoidTy(*s_Context), inArgTypes, false);
	
	// Create a function object with a void pointer input type and a void return type
	llvm::Constant* pFuncConst = s_pModule->getOrInsertFunction(funcName, pFuncType);	
	llvm::Function* pFuncObj = llvm::cast<llvm::Function>(pFuncConst);
	
	// Set the calling convention of the function to the C calling convention
    pFuncObj->setCallingConv(llvm::CallingConv::C);
	
    // Create an entry basic block for the function
	compVersion.pEntryBlock = llvm::BasicBlock::Create(*s_Context, "entry", pFuncObj);
	llvm::IRBuilder<> entryBuilder(compVersion.pEntryBlock);
    
    // Store a pointer to the LLVM function object
	compVersion.pLLVMFunc = pFuncObj;
	
	// Create the initial variable map
	VariableMap variableMap;
	
	// Ensure that there are no more arguments than the number of formal parameters
	assert (argTypeStr.size() <= inParams.size());

	// Get a pointer to the input struct value
	llvm::Value* pInStructVal = pFuncObj->arg_begin();
	
	// For each input argument
	for (size_t i = 0; i < argTypeStr.size(); ++i)
	{
		// Get the symbol for this argument
		SymbolExpr* pSymbol = inParams[i];
		
		// Read the argument from the input struct
		llvm::Value* gepValsArg[2];
		gepValsArg[0] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*s_Context), 0);
		gepValsArg[1] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*s_Context), i);
        llvm::ArrayRef<llvm::Value*> gepValsArgRef(gepValsArg, gepValsArg+2);
		llvm::Value* pArgAddrVal = entryBuilder.CreateGEP(pInStructVal, gepValsArgRef);
		llvm::Value* pArgVal = entryBuilder.CreateLoad(pArgAddrVal);		
		
		// Create a value object for this argument
		Value argValue(pArgVal, compVersion.inArgObjTypes[i]);
		
		// Add the argument value to the variable map 
		variableMap[pSymbol] = argValue;
	}
	
	// Get pointers to the "nargin" and "nargout" symbols
	SymbolExpr* pNarginSym = Interpreter::getNarginSym();
	SymbolExpr* pNargoutSym = Interpreter::getNargoutSym();
		
	// Read the "nargout" argument from the input struct
	llvm::Value* gepValsNargout[2];
	gepValsNargout[0] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*s_Context), 0);
	gepValsNargout[1] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*s_Context), argTypeStr.size());
    llvm::ArrayRef<llvm::Value*> gepValsNargoutRef(gepValsNargout, gepValsNargout+2);
	llvm::Value* pNargoutAddrVal = entryBuilder.CreateGEP(pInStructVal, gepValsNargoutRef);
	llvm::Value* pNargoutVal = entryBuilder.CreateLoad(pNargoutAddrVal);	
	
	// Add the "nargout" argument to the variable map
	variableMap[pNargoutSym] = Value(pNargoutVal, DataObject::MATRIX_F64);
	
	// Add the "nargin" constant to the variable map
	variableMap[pNarginSym] = Value(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*s_Context), argTypeStr.size()), DataObject::MATRIX_F64);
	
	if (s_jitCopyEnableVar)	// generate copy code(if necessary) before compiling the body
	{
		TypeInfoMap::const_iterator typeItr = compVersion.pTypeInferInfo->preTypeMap.find(compFunction.pFuncBody);
		assert (typeItr != compVersion.pTypeInferInfo->preTypeMap.end());
		const VarTypeMap& typeMap = typeItr->second;
		assert(compVersion.pArrayCopyInfo);
		const Expression::SymbolSet& copies = compVersion.pArrayCopyInfo->paramsToCopy;
		
		// generate the required copy code, if any 
		Expression::SymbolSet::const_iterator iter = copies.begin();
		for(; iter != copies.end(); ++iter)
		{
			CopyInfo cpEntry(*iter, Expression::SymbolSet());	
			genCopyCode(entryBuilder, compFunction, compVersion, typeMap, variableMap, cpEntry);
		}
	}
	
	// Declare variables to keep track of the possible exit points of the function body
	BranchPoint exitPoint;
	BranchList contPoints;
	BranchList breakPoints;
	BranchList returnPoints;
	
	// Compile the function's body
	llvm::BasicBlock* pSeqBlock = compStmtSeq(compFunction.pFuncBody,compFunction,compVersion,
		variableMap, exitPoint, contPoints, breakPoints, returnPoints);

	// Create an exit basic block for the function
	llvm::BasicBlock* pExitBlock = llvm::BasicBlock::Create(*s_Context, "exit", pFuncObj);

	// Ensure that there are no unmatched continue or break points
	if (contPoints.empty() == false || breakPoints.empty() == false)
		throw CompError("misplaced continue or break statement");
	
	// Add the exit point to the return point list, if it is specified
	if (exitPoint.first != NULL)
		returnPoints.push_back(exitPoint);
	
	// Ensure that there is at least one return point
	assert (returnPoints.empty() == false);
	
	// Find the type info after the function body
	TypeInfoMap::const_iterator bodyTypeItr = compVersion.pTypeInferInfo->postTypeMap.find(compFunction.pFuncBody);
	assert (bodyTypeItr != compVersion.pTypeInferInfo->postTypeMap.end());
	
	// Match the variable mappings for the return points
	VariableMap exitVarMap = matchBranchPoints(compFunction, compVersion, outParamSet, 
			bodyTypeItr->second, returnPoints, pExitBlock);
	
	//std::cout << "Done matching fn return branch points" << std::endl;
		
	// Create an IR builder for the exit block
	llvm::IRBuilder<> exitBuilder(pExitBlock);
	
	// If the function has no return parameters
	if (outParams.size() == 0)
	{
		// Return immediately
		exitBuilder.CreateRetVoid();
	}
	else
	{
		compFuncReturn(exitBuilder, compFunction, compVersion, exitVarMap, outParams);
	}
      
	// Add a branch to the compiled function body from the function's entry block
	// NOTE: we do this at the very end because instructions may need to be added
	//       to the entry block during compilation, see getCallEnv for details.
	entryBuilder.CreateBr(pSeqBlock);
	
	// Run the optimization passes on the function
	s_pFunctionPasses->run(*pFuncObj);
	
	// Get a function pointer to the compiled function
	COMP_FUNC_PTR pFuncPtr = (COMP_FUNC_PTR)s_pExecEngine->getPointerToFunction(pFuncObj);
	
	// Store a pointer to the compiled function code
	compVersion.pFuncPtr = pFuncPtr;
	
	// If we are in verbose mode
	if (ConfigManager::s_verboseVar)
	{
		// Log that compilation is complete
		std::cout << "Compilation complete" << std::endl;
		
		// Run the pretty-printer pass on the function
		s_pPrintPass->run(*pFuncObj);
	}
	
	PROF_STOP_TIMER(Profiler::COMP_TIME_TOTAL);
}

/***************************************************************
* Function: JITCompiler::callFunction()
* Purpose : Call a JIT-compiled version of a function
* Initial : Maxime Chevalier-Boisvert on April 29, 2009
****************************************************************
Revisions and bug fixes:
*/
ArrayObj* JITCompiler::callFunction(ProgFunction* pFunction, ArrayObj* pArguments, size_t outArgCount)
{
	// Get a reference to the input parameter vector
	const ProgFunction::ParamVector& inParams = pFunction->getInParams();

	// Get a reference to the output parameter vector
	const ProgFunction::ParamVector& outParams = pFunction->getOutParams();
	
	// If there are too many input arguments, throw an exception
	if (pArguments->getSize() > inParams.size())
		throw RunError("too many input arguments");	
	
	// If there are too many output arguments, throw an exception
	if (outArgCount > outParams.size())
		throw RunError("too many output arguments");

	// Build a type set string from the arguments
	TypeSetString argTypeStr = typeSetStrMake(pArguments);
		
	// Attempt to find the function in the function map
	FunctionMap::iterator funcItr = s_functionMap.find(pFunction);
	
	// If the version of this function is not yet compiled
	if (funcItr == s_functionMap.end() || funcItr->second.versions.find(argTypeStr) == funcItr->second.versions.end())
	{
		// Compile the requested function version
		compileFunction(pFunction, argTypeStr);
	}
	
	// Find the compiled function object
	funcItr = s_functionMap.find(pFunction);
	assert (funcItr != s_functionMap.end());
	
	// Get a reference to the compiled function object
	CompFunction& compFunction = funcItr->second;
	
	// Find a version of this function matching the argument types
	VersionMap::iterator versionItr = compFunction.versions.find(argTypeStr);
	assert (versionItr != compFunction.versions.end());
	
	// Get a reference to the compiled function version
	CompVersion& compVersion = versionItr->second;
	
	// If there is no call wrapper function available
	if (compVersion.pWrapperPtr == NULL)
	{
		// Compile a wrapper function for the function version
		compWrapperFunc(compFunction, compVersion);
	}
	
	// Call the wrapper function with the input arguments
	ArrayObj* pOutput = compVersion.pWrapperPtr(pArguments, outArgCount);
	
	// Return the output array
	return pOutput;
}

/***************************************************************
* Function: JITCompiler::getCallEnv()
* Purpose : Handle exceptions during function calls
* Initial : Maxime Chevalier-Boisvert on June 9, 2009
****************************************************************
Revisions and bug fixes:
*/
void JITCompiler::callExceptHandler(
	ProgFunction* pFunction,
	COMP_FUNC_PTR pFuncPtr,
	byte* pInStruct,
	byte* pOutStruct
)
{
	// Setup a try block to catch any errors
	try
	{
		// Perform the function call
		pFuncPtr(pInStruct, pOutStruct);		
	}
	
	// If runtime errors are caught
	catch (RunError error)
	{
		// Add additional error text
		error.addInfo("error during call to \"" + pFunction->getFuncName() + "\"");

		// Retrow the error
		throw error;
	}	
}

/***************************************************************
* Function: JITCompiler::getCallEnv()
* Purpose : Create/get the calling environment for a function
* Initial : Maxime Chevalier-Boisvert on June 9, 2009
****************************************************************
Revisions and bug fixes:
*/
llvm::Value* JITCompiler::getCallEnv(
	CompFunction& function,
	CompVersion& version
)
{
	// If there is no environment object at this point
	if (version.pEnvObject == NULL)
	{
		// Create an IR builder for the function's entry block
		llvm::IRBuilder<> irBuilder(version.pEntryBlock);
		
		// Create the call to get the function's local environment
		llvm::Value* pLocalEnvPtr = createNativeCall(
			irBuilder,
			(void*)ProgFunction::getLocalEnv,
			LLVMValueVector(1, createPtrConst(function.pProgFunc))
		);		

		// If the function is a script
		if (function.pProgFunc->isScript())
		{
			// Use the local environment directly
			version.pEnvObject = pLocalEnvPtr;
		}
		else
		{
			// Create the call to extend the function's local environment
			version.pEnvObject = createNativeCall(
				irBuilder,
				(void*)Environment::extend,
				LLVMValueVector(1, pLocalEnvPtr)
			);
		}	
	}
	
	// Return the environment object pointer
	return version.pEnvObject;	
}

/***************************************************************
* Function: JITCompiler::getCallStructs()
* Purpose : Create/get the call data struct for a function
* Initial : Maxime Chevalier-Boisvert on June 18, 2009
****************************************************************
Revisions and bug fixes:
*/
JITCompiler::LLVMValuePair JITCompiler::getCallStructs(
	CompFunction& callerFunction,
	CompVersion& callerVersion,
	CompFunction& calleeFunction,
	CompVersion& calleeVersion
)
{
	// Get the size of the in/out structures for the callee
    size_t inStructSize = s_data_layout->getTypeStoreSize(calleeVersion.pInStructType);
    size_t outStructSize = s_data_layout->getTypeStoreSize(calleeVersion.pOutStructType);

	// Update the caller's in/out struct sizes
	callerVersion.inStructSize = std::max(callerVersion.inStructSize, inStructSize);
	callerVersion.outStructSize = std::max(callerVersion.outStructSize, outStructSize);
	
	// Create an IR builder for the function's entry block
	llvm::IRBuilder<> irBuilder(callerVersion.pEntryBlock);
	
	// Allocate memory on the stack for the input call data structure
	llvm::Value* pNewInStruct = irBuilder.CreateAlloca(
		llvm::Type::getInt8Ty(*s_Context), 
		llvm::ConstantInt::get(
			llvm::Type::getInt32Ty(*s_Context),
			callerVersion.inStructSize
		)
	);
	
	// Allocate memory on the stack for the output call data structure
	llvm::Value* pNewOutStruct = irBuilder.CreateAlloca(
		llvm::Type::getInt8Ty(*s_Context),
		llvm::ConstantInt::get(
			llvm::Type::getInt32Ty(*s_Context),
			callerVersion.outStructSize
		)
	);

	// If are already call data structs at this point
	if (callerVersion.pInStruct != NULL)
	{	
		// Note: here, we redo the struct allocations in case the max
		// in/out struct sizes have changed as a result of a callee
		// function being compiled.
		callerVersion.pInStruct->replaceAllUsesWith(pNewInStruct);
		callerVersion.pOutStruct->replaceAllUsesWith(pNewOutStruct);
	}
	
	// Store the in/out struct pointers
	callerVersion.pInStruct = pNewInStruct;
	callerVersion.pOutStruct = pNewOutStruct;
	
	// Return call data structure pointers
	return LLVMValuePair(callerVersion.pInStruct, callerVersion.pOutStruct);	
}

/***************************************************************
* Function: JITCompiler::matchBranchPoints()
* Purpose : Match branching point variable mappings
* Initial : Maxime Chevalier-Boisvert on March 17, 2009
****************************************************************
Revisions and bug fixes:
*/
JITCompiler::VariableMap JITCompiler::matchBranchPoints(
	CompFunction& function,
	CompVersion& version,
	const Expression::SymbolSet& liveVars,
	const VarTypeMap& varTypes,
	const BranchList& branchPoints,
	llvm::BasicBlock* pDestBlock
)
{
	// If we are in verbose mode
	if (ConfigManager::s_verboseVar)
	{
		// Log that we are matching branch points
		std::cout << "Matching branch points" << std::endl;
		std::cout << "Number of branch points: " << branchPoints.size() << std::endl;
	}
	
	// Declare a variable for the desired variable mapping
	VariableMap varMapping;
	
	// Declare a set for all the variables stored in registers
	Expression::SymbolSet variables;
	
	// Create an IR builder for the destination block
	llvm::IRBuilder<> destBuilder(pDestBlock);
	
	// For each branch point
	for (BranchList::const_iterator pointItr = branchPoints.begin(); pointItr != branchPoints.end(); ++pointItr)
	{
		// Get a reference to this branch point
		const BranchPoint& branchPoint = *pointItr;
		
		// For each variable
		for (VariableMap::const_iterator varItr = branchPoint.second.begin(); varItr != branchPoint.second.end(); ++varItr)
		{
			// If this variable is not live at this point, skip it
			if (liveVars.find(varItr->first) == liveVars.end())
				continue;
			
			// Add the variable to the set
			variables.insert(varItr->first);
		}
	}
		
	// For each symbol in the set
	for (Expression::SymbolSet::iterator varItr = variables.begin(); varItr != variables.end(); ++varItr)
	{
		// Get a pointer to this symbol
		SymbolExpr* pSymbol = *varItr;
		
		// Declare a flag to indicate if the variable's storage status is unknown at any branch point
		bool unknownStatus = false;
		
		// Declare a flag to indicate if the variable is locally stored at any branch point
		bool locallyStored = false;
		
		// For each branch point
		for (BranchList::const_iterator pointItr = branchPoints.begin(); pointItr != branchPoints.end(); ++pointItr)
		{
			// Get a reference to this branch point
			const BranchPoint& branchPoint = *pointItr;
			
			// Attempt to find the symbol in the variable map
			VariableMap::const_iterator mapItr = branchPoint.second.find(pSymbol);
			
			// If the status is unknown at this branch point
			if (mapItr == branchPoint.second.end())
			{
				// Set the unknown status flag
				unknownStatus = true;
			}
			
			// Otherwise, if the symbol is locally stored
			else if (mapItr->second.pValue != NULL)
			{
				// Set the locally stored flag
				locallyStored = true;	
			}			
		}
		
		// If the variable's storage status is unknown at any branch point
		if (unknownStatus)
		{
			// The variable will not appear in the new variable map
			// This will force a safe read the next time it is used
		}
		
		// Otherwise, if the variable is locally stored at any branch point
		else if (locallyStored)
		{
			// Find the type set for this variable
			VarTypeMap::const_iterator typeMapItr = varTypes.find(pSymbol);
			TypeSet typeSet = (typeMapItr != varTypes.end())? typeMapItr->second:TypeSet();
			
			// Get the appropriate storage mode for this variable
			DataObject::Type objectType;
			llvm::Type* storageMode = getStorageMode(typeSet, objectType);
			
			// Create a phi node in the variable map
			varMapping[pSymbol] = Value(destBuilder.CreatePHI(storageMode, 2), objectType);
		}
		
		// Otherwise, the variable is never locally stored
		else
		{
			// The symbol will remain stored in the environment
			varMapping[pSymbol].pValue = NULL;
		}
	}
	
	// For each branch point
	for (BranchList::const_iterator pointItr = branchPoints.begin(); pointItr != branchPoints.end(); ++pointItr)
	{
		// Get a reference to this branch point
		const BranchPoint& branchPoint = *pointItr;
		
		// Create an IR builder for the branch point
		llvm::IRBuilder<> pointBuilder(branchPoint.first);
		
		// For each symbol in the set
		for (Expression::SymbolSet::iterator varItr = variables.begin(); varItr != variables.end(); ++varItr)
		{
			// Get a pointer to this symbol
			SymbolExpr* pSymbol = *varItr;
			
			// Attempt to find the variable at this branch point
			VariableMap::const_iterator mapItr = branchPoint.second.find(pSymbol); 
			
			// Attempt to find the symbol in the variable mapping
			VariableMap::iterator varMapItr = varMapping.find(pSymbol);
			
			// If the status of this symbol is unknown at another branch point
			if (varMapItr == varMapping.end())
			{
				// If the variable is locally stored at this branch point
				if (mapItr != branchPoint.second.end() && mapItr->second.pValue != NULL)
				{
					// Write the variable to the environment
					writeVariable(
						pointBuilder,
						function,
						version,
						pSymbol,
						mapItr->second
					);
				}				
				
				// Move to the next symbol
				continue;
			}
			
			// Get a pointer to the phi node for this symbol
			llvm::PHINode* pPhiNode = (llvm::PHINode*)varMapItr->second.pValue;
			
			// If this symbol has no phi node, move to the next symbol
			if (pPhiNode == NULL)
				continue;
			
			// Declare a value object for the variable
			Value value;
			
			// If the variable is locally stored at the branch point
			if (mapItr != branchPoint.second.end() && mapItr->second.pValue != NULL)
			{
				// Get the value at the branch point
				value = mapItr->second;
				
				// If the storage mode of this value does not match that of the phi node
				if (value.pValue->getType() != pPhiNode->getType())
				{
					// Change the storage mode of the value
					llvm::Value* pNewValue = changeStorageMode(
						pointBuilder,
						value.pValue,
						value.objType,
						pPhiNode->getType()
					);
					
					// Update the value pointer
					value.pValue = pNewValue;
				}
			}
			else
			{
				// If we know for sure what the variable is in the environment, allow an unsafe read
				bool unsafeRead = mapItr != branchPoint.second.end() && mapItr->second.pValue == NULL;
				
				// Find the type set for this variable
				VarTypeMap::const_iterator varTypeItr = varTypes.find(pSymbol);
				assert (varTypeItr != varTypes.end());
				
				// Read the variable from the environment
				value = readVariable(
					pointBuilder, 
					function,
					version,
					varTypeItr->second,
					pSymbol,
					!unsafeRead
				);
				
				// Ensure that the storage mode of the value matches that of the phi node
				assert (value.pValue->getType() == pPhiNode->getType());
			}
			
			// Ensure that the incoming type matches that of the phi node
			assert (value.pValue->getType() == pPhiNode->getType());
			
			// Add an incoming value for this branch point to the phi node
			pPhiNode->addIncoming(value.pValue, branchPoint.first);
		}
		
		// Create a branch to the destination block
		pointBuilder.CreateBr(pDestBlock);
	}
	
	// If we are in verbose mode, log that we are done matching branch points
	if (ConfigManager::s_verboseVar)
		std::cout << "Done matching branch points" << std::endl;
	
	// Return the final variable mapping
	return varMapping;
}

/***************************************************************
* Function: JITCompiler::writeVariables()
* Purpose : Write variables to the environment
* Initial : Maxime Chevalier-Boisvert on March 18, 2009
****************************************************************
Revisions and bug fixes:
*/
void JITCompiler::writeVariables(
	llvm::IRBuilder<>& irBuilder,
	CompFunction& function,
	CompVersion& version,
	VariableMap& varMap,
	const Expression::SymbolSet& variables
)
{
	// For each variable
	for (Expression::SymbolSet::const_iterator itr = variables.begin(); itr != variables.end(); ++itr)
	{		
		// Get a pointer to the symbol
		SymbolExpr* pSymbol = *itr;
	
		// If we are in verbose mode, log that we are writing the variable
		if (ConfigManager::s_verboseVar)
			std::cout << "Writing var: " << pSymbol->toString() << std::endl;
		
		// Attempt to find the variable in the variable map
		VariableMap::const_iterator varItr = varMap.find(pSymbol); 
		
		// If the variable is locally stored
		if (varItr != varMap.end() && varItr->second.pValue != NULL)
		{
			// Bind the variable in the function's environment
			writeVariable(irBuilder, function, version, pSymbol, varItr->second);
			
			// Set the variable map binding to NULL
			varMap[pSymbol].pValue = NULL;
			
			// If we are in verbose mode, log that the variable was written
			if (ConfigManager::s_verboseVar)
				std::cout << "Var written" << std::endl;
		}
		else
		{
			// If we are in verbose mode
			if (ConfigManager::s_verboseVar)
			{
				// Log whether or not there is an entry for the var
				if (varItr == varMap.end())
					std::cout << "No entry for var" << std::endl;
				else
					std::cout << "Var entry is null" << std::endl;
			}
		}
	}
}

/***************************************************************
* Function: JITCompiler::writeVariable()
* Purpose : Write a single variable to the environment
* Initial : Maxime Chevalier-Boisvert on March 22, 2009
****************************************************************
Revisions and bug fixes:
*/
void JITCompiler::writeVariable(
	llvm::IRBuilder<>& irBuilder,
	CompFunction& function,
	CompVersion& version,
	SymbolExpr* pSymbol,
	const Value& value
)
{
	// Ensure that the value is locally stored
	assert (value.pValue != NULL);
	
	// Get the LLVM value pointer
	llvm::Value* pValue = value.pValue;
	
	// If the value is not stored as an object pointer
	if (pValue->getType() != VOID_PTR_TYPE)
	{
		// Change the value's storage mode
		pValue = changeStorageMode(
			irBuilder,
			pValue,
			value.objType,
			VOID_PTR_TYPE
		);
	}	
	
	// Get a pointer to the current call environment
	llvm::Value* pEnvObj = getCallEnv(function, version);
	
	// Setup thje call arguments 
	LLVMValueVector bindArgs;
	bindArgs.push_back(pEnvObj);
	bindArgs.push_back(createPtrConst(pSymbol));
	bindArgs.push_back(pValue);
	
	// Create the call to bind the variable in the function's environment 
	createNativeCall(
		irBuilder,
		(void*)Environment::bind,
		bindArgs
	);
}

/***************************************************************
* Function: JITCompiler::readVariables()
* Purpose : Read variables to the environment
* Initial : Maxime Chevalier-Boisvert on March 18, 2009
****************************************************************
Revisions and bug fixes:
*/
void JITCompiler::readVariables(
	llvm::IRBuilder<>& irBuilder,
	CompFunction& function,
	CompVersion& version,
	const VarTypeMap& varTypes,
	VariableMap& varMap,
	const Expression::SymbolSet& variables
)
{
	// For each variable
	for (Expression::SymbolSet::const_iterator itr = variables.begin(); itr != variables.end(); ++itr)
	{
		// Get a pointer to the symbol
		SymbolExpr* pSymbol = *itr;
		
		// Attempt to find the variable in the variable map
		VariableMap::const_iterator varItr = varMap.find(pSymbol); 
		
		// If this variable is already local, skip it
		if (varItr != varMap.end() && varItr->second.pValue != NULL)
			continue;
		
		// If we know for sure what the variable is in the environment, allow an unsafe read
		bool unsafeRead = varItr != varMap.end() && varItr->second.pValue == NULL;
		
		// Find the type set for the variable
		VarTypeMap::const_iterator varTypeItr = varTypes.find(pSymbol);
		assert (varTypeItr != varTypes.end());
		
		// Read the variable from the environment
		Value value = readVariable(irBuilder, function, version, varTypeItr->second, pSymbol, !unsafeRead);
		
		// Store the value in the variable map
		varMap[pSymbol] = value;
	}
}

/***************************************************************
* Function: JITCompiler::readVariable()
* Purpose : Read a single variable from the environment
* Initial : Maxime Chevalier-Boisvert on March 22, 2009
****************************************************************
Revisions and bug fixes:
*/
JITCompiler::Value JITCompiler::readVariable(
	llvm::IRBuilder<>& irBuilder,
	CompFunction& function,
	CompVersion& version,
	const TypeSet& typeSet,
	SymbolExpr* pSymbol,
	bool safeRead
)
{	
	// Get a pointer to the current call environment
	llvm::Value* pEnvObj = getCallEnv(function, version);

	// Create a pointer constant for the symbol argument
	llvm::Value* pSymArg = createPtrConst(pSymbol);
	
	// Declare a pointer for the value read
	llvm::Value* pReadValue;
	
	// If this should be a safe read
	if (safeRead == true)
	{
		// Setup the call arguments
		LLVMValueVector evalArgs;
		evalArgs.push_back(pSymArg);
		evalArgs.push_back(pEnvObj);
		
		// If we are in verbose mode, log the safe read
		if (ConfigManager::s_verboseVar)
			std::cout << "Adding safe read of: " << pSymbol->toString() << std::endl;
		
		// Lookup the variable in the environment
		// through the interpreter (the safe way)
		pReadValue = createNativeCall(
			irBuilder,
			(void*)Interpreter::evalSymbol,
			evalArgs
		);
	}
	else
	{
		// Setup the call arguments
		LLVMValueVector lookupArgs;
		lookupArgs.push_back(pEnvObj);
		lookupArgs.push_back(pSymArg);
		
		// If we are in verbose mode, log the unsafe read
		if (ConfigManager::s_verboseVar)
			std::cout << "Adding unsafe read of: " << pSymbol->toString() << std::endl;
		
		// Lookup the variable in the environment
		// directly (the fast way)
		pReadValue = createNativeCall(
			irBuilder,
			(void*)Environment::lookup,
			lookupArgs
		);
	}
	
	// Get the optimal storage mode for this variable
	DataObject::Type objectType;
	llvm::Type* storageMode = getStorageMode(
		typeSet,
		objectType
	);
	
	// If we are in verbose mode
	if (ConfigManager::s_verboseVar)
	{
		// Log the storage mode and object type
      std::cout << "Storage mode is: ";
      storageMode->dump();
      std::cout << "Obj type is: " << DataObject::getTypeName(objectType) << std::endl;
	}
	
	// If the default storage mode is not optimal
	if (pReadValue->getType() != storageMode)
	{
		// Change the read value's storage mode
		pReadValue = changeStorageMode(
			irBuilder,
			pReadValue,
			objectType,
			storageMode
		);
	}
	
	// Return the read value
	return Value(pReadValue, objectType);
}

/***************************************************************
* Function: JITCompiler::markVarsWritten()
* Purpose : Mark variables as written in the environment
* Initial : Maxime Chevalier-Boisvert on March 19, 2009
****************************************************************
Revisions and bug fixes:
*/
void JITCompiler::markVarsWritten(
	VariableMap& varMap,
	const Expression::SymbolSet& variables
)
{
	// For each variable
	for (Expression::SymbolSet::const_iterator itr = variables.begin(); itr != variables.end(); ++itr)
	{
		// Get a pointer to the symbol
		SymbolExpr* pSymbol = *itr;
				
		// Set the variable map binding to NULL
		varMap[pSymbol].pValue = NULL;
	}
}

/***************************************************************
* Function: JITCompiler::printVarMap()
* Purpose : Print the contents of a variable map
* Initial : Maxime Chevalier-Boisvert on June 17, 2009
****************************************************************
Revisions and bug fixes:
*/
void JITCompiler::printVarMap(
	const VariableMap& varMap
)
{
	// Log that we are printing the variable map contents
	std::cout << "**** Variable Map Contents ****" << std::endl;
	
	// For each variable in the map
	for (VariableMap::const_iterator varItr = varMap.begin(); varItr != varMap.end(); ++varItr)
	{
		// Get the symbol for this variable
		SymbolExpr* pSymbol = varItr->first;
		
		// Get the associated value object
		const Value& value = varItr->second;
		
		// Print the symbol
		std::cout << pSymbol->toString() << " ==> ";
		
		// Output the storage mode of the variable
		if (value.pValue == NULL)
			std::cout << "env stored";
		else {
          value.pValue->getType()->dump();
        }
		
		// Output the associated object type
		std::cout << " (" << DataObject::getTypeName(value.objType) << ") " << std::endl;
	}
	
	// Log that this is the end of the dump
	std::cout << "**** End Variable Map Contents ****" << std::endl;
}

/***************************************************************
* Function: JITCompiler::getStorageMode()
* Purpose : Get the storage mode for a given type set
* Initial : Maxime Chevalier-Boisvert on May 22, 2009
****************************************************************
Revisions and bug fixes:
*/
llvm::Type* JITCompiler::getStorageMode(
	const TypeSet& typeSet,
	DataObject::Type& objType
)
{
	// If there is not one element in the type set
	if (typeSet.size() != 1)
	{
		// The type is unknown at this point
		objType = DataObject::UNKNOWN;
		
		// Store the value as an object pointer
		return VOID_PTR_TYPE;
	}
	
	// Get the type information for the object
	const TypeInfo& typeInfo = *typeSet.begin();
		
	// Get the object type
	objType = typeInfo.getObjType();
	
	// If the value is scalar and is not a complex matrix or a cell array
	if (typeInfo.isScalar() && objType != DataObject::MATRIX_C128 && objType != DataObject::CELLARRAY)
	{
		// If the value is a logical array
		if (objType == DataObject::LOGICALARRAY)
		{
			// Store the value as an int1 (boolean)
			return llvm::Type::getInt1Ty(*s_Context);
		}
		
		// If the value is integer
		else if (typeInfo.isInteger())
		{
			// Store the value as an int64
			return llvm::Type::getInt64Ty(*s_Context);
		}
		
		// Otherwise, if the value is not integer
		else
		{
			// Store the value as a float64
			return llvm::Type::getDoubleTy(*s_Context);
		}	
	}
	
	// Store the value as an object pointer
	return VOID_PTR_TYPE;	
}

/***************************************************************
* Function: JITCompiler::widestStorageMode()
* Purpose : Get the widest storage mode among two options
* Initial : Maxime Chevalier-Boisvert on May 22, 2009
****************************************************************
Revisions and bug fixes:
*/
llvm::Type* JITCompiler::widestStorageMode(
	llvm::Type* modeA,
	llvm::Type* modeB
)
{
	// Ensure that the types are not NULL
	assert (modeA != NULL && modeB != NULL);
	
	// If either option is the object pointer mode, return that option
	if (modeA == VOID_PTR_TYPE || modeB == VOID_PTR_TYPE)
		return VOID_PTR_TYPE;
	
	// If either option is the f64 mode, return that option
	else if (modeA == llvm::Type::getDoubleTy(*s_Context) || 
                modeB == llvm::Type::getDoubleTy(*s_Context))
		return llvm::Type::getDoubleTy(*s_Context);
	
	// If either option is the int64 mode, return that option
	else if (modeA == llvm::Type::getInt64Ty(*s_Context) 
                || modeB == llvm::Type::getInt64Ty(*s_Context))
		return llvm::Type::getInt64Ty(*s_Context);
	
	// If either option is the int32 mode, return that option
	else if (modeA == llvm::Type::getInt32Ty(*s_Context) 
                || modeB == llvm::Type::getInt32Ty(*s_Context))
		return llvm::Type::getInt32Ty(*s_Context);	
	
	// If either option is the int1 (boolean) mode, return that option
	else if (modeA == llvm::Type::getInt1Ty(*s_Context) 
                || modeB == llvm::Type::getInt1Ty(*s_Context))
		return llvm::Type::getInt1Ty(*s_Context);
	
	// Invalid mode specified
	assert (false);	
}

/***************************************************************
* Function: JITCompiler::changeStorageMode()
* Purpose : Change the storage mode of a value
* Initial : Maxime Chevalier-Boisvert on May 21, 2009
****************************************************************
Revisions and bug fixes:
*/
llvm::Value* JITCompiler::changeStorageMode(
	llvm::IRBuilder<>& irBuilder,
	llvm::Value* pCurVal,
	DataObject::Type objType,
	llvm::Type* newMode
)
{
	// Ensure that the variable is locally stored
	assert (pCurVal != NULL);
	
	// If no type conversion is needed
	if (pCurVal->getType() == newMode)
	{
		// Return the value unchanged
		return pCurVal;
	}

	// If we are in verbose mode
	if (ConfigManager::s_verboseVar)
	{
		// Log info about the storage mode conversion
		std::cout << "Performing storage mode conversion: ";
		pCurVal->getType()->dump(); 
		std::cout <<  " ==> ";
		newMode->dump();
        std::cout << " (";
		std::cout << DataObject::getTypeName(objType) << ")" << std::endl;
	}

	// If the variable is stored as an boolean value
	if (pCurVal->getType() == llvm::Type::getInt1Ty(*s_Context))
	{
		// If we must convert to an integer type
		if (newMode == llvm::Type::getInt64Ty(*s_Context))
		{
			// Create a type conversion from int1 to int64
			llvm::Value* pNewVal = irBuilder.CreateIntCast(pCurVal, llvm::Type::getInt64Ty(*s_Context), false);
			
			// Return the updated value
			return pNewVal;
		}		
		
		// If we must convert to a floating-point type
		else if (newMode == llvm::Type::getDoubleTy(*s_Context))
		{
			// Create a type conversion from int1 to double			
			llvm::Value* pNewVal = irBuilder.CreateUIToFP(pCurVal, llvm::Type::getDoubleTy(*s_Context));
					
			// Return the updated value
			return pNewVal;
		}
		
		// If we must convert to an object pointer type
		else if (newMode == VOID_PTR_TYPE)
		{
			// If we should create an f64 matrix
			if (objType == DataObject::MATRIX_F64)
			{
				// Create a type conversion from int to double
				llvm::Value* pNewVal = irBuilder.CreateSIToFP(pCurVal, llvm::Type::getDoubleTy(*s_Context));
				
				// Create an f64 matrix object from the new value
				llvm::Value* pNewObj = createNativeCall(
					irBuilder,
					(void*)MatrixF64Obj::makeScalar,
					LLVMValueVector(1, pNewVal)
				);
				
				// Return the new object
				return pNewObj;
			}
			
			// If we should create a character array
			else if (objType == DataObject::CHARARRAY)
			{
				// Create a type conversion from int1 to int8 (char)
				llvm::Value* pNewVal = irBuilder.CreateIntCast(pCurVal, llvm::Type::getInt8Ty(*s_Context), false);
				
				// Create an f64 matrix object from the new value
				llvm::Value* pNewObj = createNativeCall(
					irBuilder,
					(void*)CharArrayObj::makeScalar,
					LLVMValueVector(1, pNewVal)
				);
				
				// Return the new object
				return pNewObj;
			}
			
			// If we should create a logical array
			else if (objType == DataObject::LOGICALARRAY)
			{
				// Create a type conversion from int1 to int8 (bool)
				llvm::Value* pNewVal = irBuilder.CreateIntCast(pCurVal, llvm::Type::getInt8Ty(*s_Context), false);
				
				// Create an f64 matrix object from the new value
				llvm::Value* pNewObj = createNativeCall(
					irBuilder,
					(void*)LogicalArrayObj::makeScalar,
					LLVMValueVector(1, pNewVal)
				);
				
				// Return the new object
				return pNewObj;
			}
		}
	}
	
	// If the variable is stored as an integer value
	else if (pCurVal->getType() == llvm::Type::getInt64Ty(*s_Context))
	{
		// If we must convert to a boolean type
		if (newMode == llvm::Type::getInt1Ty(*s_Context))
		{
			// Get the boolean value of the integer value
			llvm::Value* pNewVal = irBuilder.CreateICmpNE(pCurVal, llvm::ConstantInt::get(llvm::Type::getInt64Ty(*s_Context), 0));
			
			// Return the updated value
			return pNewVal;
		}
		
		// If we must convert to a floating-point type
		else if (newMode == llvm::Type::getDoubleTy(*s_Context))
		{
			// Create a type conversion from int to double
			llvm::Value* pNewVal = irBuilder.CreateSIToFP(pCurVal, llvm::Type::getDoubleTy(*s_Context));
			
			// Return the updated value
			return pNewVal;
		}
		
		// If we must convert to an object pointer type
		else if (newMode == VOID_PTR_TYPE)
		{
			// If we should create an f64 matrix
			if (objType == DataObject::MATRIX_F64)
			{
				// Create a type conversion from int to double
				llvm::Value* pNewVal = irBuilder.CreateSIToFP(pCurVal, llvm::Type::getDoubleTy(*s_Context));
				
				// Create an f64 matrix object from the new value
				llvm::Value* pNewObj = createNativeCall(
					irBuilder,
					(void*)MatrixF64Obj::makeScalar,
					LLVMValueVector(1, pNewVal)
				);
				
				// Return the new object
				return pNewObj;
			}
			
			// If we should create a character array
			else if (objType == DataObject::CHARARRAY)
			{
				// Create a type conversion from int64 to int8 (char)
				llvm::Value* pNewVal = irBuilder.CreateIntCast(pCurVal, llvm::Type::getInt8Ty(*s_Context), false);
				
				// Create an f64 matrix object from the new value
				llvm::Value* pNewObj = createNativeCall(
					irBuilder,
					(void*)CharArrayObj::makeScalar,
					LLVMValueVector(1, pNewVal)
				);
				
				// Return the new object
				return pNewObj;
			}
			
			// If we should create a logical array
			else if (objType == DataObject::LOGICALARRAY)
			{
				// Create a type conversion from int64 to int8 (bool)
				llvm::Value* pNewVal = irBuilder.CreateIntCast(pCurVal, llvm::Type::getInt8Ty(*s_Context), false);
				
				// Create an f64 matrix object from the new value
				llvm::Value* pNewObj = createNativeCall(
					irBuilder,
					(void*)LogicalArrayObj::makeScalar,
					LLVMValueVector(1, pNewVal)
				);
				
				// Return the new object
				return pNewObj;
			}
		}
	}
	
	// If the variable is stored as a floating-point value
	else if (pCurVal->getType() == llvm::Type::getDoubleTy(*s_Context))
	{
		// If we must convert to a boolean type
		if (newMode == llvm::Type::getInt1Ty(*s_Context))
		{
			// Get the boolean value of the floating-point value
			llvm::Value* pNewVal = irBuilder.CreateFCmpONE(pCurVal, llvm::ConstantFP::get(llvm::Type::getDoubleTy(*s_Context), 0));
			
			// Return the updated value
			return pNewVal;
		}
		
		// If we must convert to an integer type
		else if (newMode == llvm::Type::getInt64Ty(*s_Context))
		{
			// Create a type conversion from double to int
			llvm::Value* pNewVal = irBuilder.CreateFPToSI(pCurVal, llvm::Type::getInt64Ty(*s_Context));
			
			// Return the updated value
			return pNewVal;
		}
		
		// If we must convert to an object pointer type
		else if (newMode == VOID_PTR_TYPE)
		{
			// If we should create an f64 matrix
			if (objType == DataObject::MATRIX_F64)
			{
				// Create an f64 matrix object from the current value
				llvm::Value* pNewObj = createNativeCall(
					irBuilder,
					(void*)MatrixF64Obj::makeScalar,
					LLVMValueVector(1, pCurVal)
				);
				
				// Return the new object
				return pNewObj;
			}
			
			// If we should create a character array
			else if (objType == DataObject::CHARARRAY)
			{
				// Create a type conversion from double to int8 (char)
				llvm::Value* pNewVal = irBuilder.CreateFPToSI(pCurVal, llvm::Type::getInt8Ty(*s_Context));
				
				// Create an f64 matrix object from the new value
				llvm::Value* pNewObj = createNativeCall(
					irBuilder,
					(void*)CharArrayObj::makeScalar,
					LLVMValueVector(1, pNewVal)
				);
				
				// Return the new object
				return pNewObj;
			}
			
			// If we should create a logical array
			else if (objType == DataObject::LOGICALARRAY)
			{
				// Create a type conversion from int64 to int8 (bool)
				llvm::Value* pNewVal = irBuilder.CreateFPToSI(pCurVal, llvm::Type::getInt8Ty(*s_Context));
				
				// Create an f64 matrix object from the new value
				llvm::Value* pNewObj = createNativeCall(
					irBuilder,
					(void*)LogicalArrayObj::makeScalar,
					LLVMValueVector(1, pNewVal)
				);
				
				// Return the new object
				return pNewObj;
			}
		}
	}
	
	// If the variable is stored as an object pointer
	else if (pCurVal->getType() == VOID_PTR_TYPE)
	{
		// If we must convert to an boolean type
		if (newMode == llvm::Type::getInt1Ty(*s_Context))
		{
			// If the value is an f64 matrix
			if (objType == DataObject::MATRIX_F64)
			{
				// Get the scalar value of the matrix
				llvm::Value* pScalarVal = createNativeCall(
					irBuilder,
					(void*)MatrixF64Obj::getScalarVal,
					LLVMValueVector(1, pCurVal)
				);	

				// Get the boolean value of the floating-point value
				llvm::Value* pNewVal = irBuilder.CreateICmpNE(pScalarVal, llvm::ConstantFP::get(llvm::Type::getDoubleTy(*s_Context), 0));
				
				// Return the updated value
				return pNewVal;
			}
			
			// If the value is a character array
			else if (objType == DataObject::CHARARRAY)
			{
				// Get the scalar value of the matrix
				llvm::Value* pScalarVal = createNativeCall(
					irBuilder,
					(void*)CharArrayObj::getScalarVal,
					LLVMValueVector(1, pCurVal)
				);	

				// Get the boolean value of the character value
				llvm::Value* pNewVal = irBuilder.CreateICmpNE(pScalarVal, llvm::ConstantInt::get(llvm::Type::getInt8Ty(*s_Context), 0));
				
				// Return the updated value
				return pNewVal;
			}
			
			// If the value is a logical array
			else if (objType == DataObject::LOGICALARRAY)
			{
				// Get the scalar value of the matrix
				llvm::Value* pScalarVal = createNativeCall(
					irBuilder,
					(void*)LogicalArrayObj::getScalarVal,
					LLVMValueVector(1, pCurVal)
				);	

				// Get the boolean value of the character value
				llvm::Value* pNewVal = irBuilder.CreateICmpNE(pScalarVal, llvm::ConstantInt::get(llvm::Type::getInt8Ty(*s_Context), 0));
				
				// Return the updated value
				return pNewVal;
			}
			
			// If the object type is unknown at compile time
			else if (objType == DataObject::UNKNOWN)
			{
				// Get the scalar value of the object
				llvm::Value* pScalarVal = createNativeCall(
					irBuilder,
					(void*)getBoolValue,
					LLVMValueVector(1, pCurVal)
				);
				
				// Convert the boolean (int8) to a native boolean (int1)
				llvm::Value* pNewVal = irBuilder.CreateICmpNE(pScalarVal, llvm::ConstantInt::get(llvm::Type::getInt8Ty(*s_Context), 0));
				
				// Return the scalar value
				return pNewVal;
			}
		}		
		
		// If we must convert to an integer type
		if (newMode == llvm::Type::getInt64Ty(*s_Context))
		{
			// If the value is an f64 matrix
			if (objType == DataObject::MATRIX_F64)
			{
				// Get the scalar value of the matrix
				llvm::Value* pScalarVal = createNativeCall(
					irBuilder,
					(void*)MatrixF64Obj::getScalarVal,
					LLVMValueVector(1, pCurVal)
				);	

				// Create a type conversion from double to int64
				llvm::Value* pNewVal = irBuilder.CreateFPToSI(pScalarVal, llvm::Type::getInt64Ty(*s_Context));

				// Return the updated value
				return pNewVal;
			}
			
			// If the value is a character array
			else if (objType == DataObject::CHARARRAY)
			{
				// Get the scalar value of the matrix
				llvm::Value* pScalarVal = createNativeCall(
					irBuilder,
					(void*)CharArrayObj::getScalarVal,
					LLVMValueVector(1, pCurVal)
				);	

				// Create a type conversion from int8 to int64
				llvm::Value* pNewVal = irBuilder.CreateIntCast(pScalarVal, llvm::Type::getInt64Ty(*s_Context), false);
				
				// Return the updated value
				return pNewVal;
			}
			
			// If the value is a logical array
			else if (objType == DataObject::LOGICALARRAY)
			{
				// Get the scalar value of the matrix
				llvm::Value* pScalarVal = createNativeCall(
					irBuilder,
					(void*)LogicalArrayObj::getScalarVal,
					LLVMValueVector(1, pCurVal)
				);	

				// Create a type conversion from int8 to int64
				llvm::Value* pNewVal = irBuilder.CreateIntCast(pScalarVal, llvm::Type::getInt64Ty(*s_Context), false);
				
				// Return the updated value
				return pNewVal;
			}
			
			// If the object type is unknown at compile time
			else if (objType == DataObject::UNKNOWN)
			{
				// Get the scalar value of the object
				llvm::Value* pScalarVal = createNativeCall(
					irBuilder,
					(void*)getInt64Value,
					LLVMValueVector(1, pCurVal)
				);
				
				// Return the scalar value
				return pScalarVal;
			}
		}
		
		// If we must convert to a floating-point type
		else if (newMode == llvm::Type::getDoubleTy(*s_Context))
		{
			// If the value is an f64 matrix
			if (objType == DataObject::MATRIX_F64)
			{
				// Get the scalar value of the matrix
				llvm::Value* pScalarVal = createNativeCall(
					irBuilder,
					(void*)MatrixF64Obj::getScalarVal,
					LLVMValueVector(1, pCurVal)
				);	
			
				// Return the scalar value
				return pScalarVal;
			}
			
			// If the value is a character array
			else if (objType == DataObject::CHARARRAY)
			{
				// Get the scalar value of the matrix
				llvm::Value* pScalarVal = createNativeCall(
					irBuilder,
					(void*)CharArrayObj::getScalarVal,
					LLVMValueVector(1, pCurVal)
				);	

				// Create a type conversion from int8 to f64
				llvm::Value* pNewVal = irBuilder.CreateSIToFP(pScalarVal, llvm::Type::getDoubleTy(*s_Context));
				
				// Return the updated value
				return pNewVal;
			}
			
			// If the value is a logical array
			else if (objType == DataObject::LOGICALARRAY)
			{
				// Get the scalar value of the matrix
				llvm::Value* pScalarVal = createNativeCall(
					irBuilder,
					(void*)LogicalArrayObj::getScalarVal,
					LLVMValueVector(1, pCurVal)
				);	

				// Create a type conversion from int8 to f64
				llvm::Value* pNewVal = irBuilder.CreateSIToFP(pScalarVal, llvm::Type::getDoubleTy(*s_Context));
				
				// Return the updated value
				return pNewVal;
			}
			
			// If the object type is unknown at compile time
			else if (objType == DataObject::UNKNOWN)
			{
				// Get the scalar value of the object
				llvm::Value* pScalarVal = createNativeCall(
					irBuilder,
					(void*)getFloat64Value,
					LLVMValueVector(1, pCurVal)
				);
				
				// Return the scalar value
				return pScalarVal;
			}
		}		
	}
	
	// Invalid type conversion requested, break an assertion
	assert (false);		
}

/***************************************************************
* Function: JITCompiler::compWrapperFunc()
* Purpose : Compile a call wrapper function
* Initial : Maxime Chevalier-Boisvert on June 11, 2009
****************************************************************
Revisions and bug fixes:
*/
void JITCompiler::compWrapperFunc(
	CompFunction& function,
	CompVersion& version			
)
{
	// Ensure that the JIT compiler was initialized
	assert (s_pModule != NULL && s_pExecEngine != NULL);
	
	// Ensure that a wrapper function is not already compiled
	assert (version.pWrapperPtr == NULL);
	
	// Ensure that the function object was compiled
	assert (version.pLLVMFunc != NULL);
	
	// Get a pointer to the function object
	ProgFunction* pFunction = function.pProgFunc;
	
	// If we are in verbose mode, log the wrapper function compilation
	if (ConfigManager::s_verboseVar)
		std::cout << "Compiling wrapper for function: \"" << pFunction->getFuncName() << "\"" << std::endl;
	
	// Create a name string for the function
	std::string funcName = pFunction->getFuncName() + "_wrapper_" + ::toString((void*)version.pTypeInferInfo);
	
	// Get a function type object with the appropriate signature
	LLVMTypeVector argTypes;
	argTypes.push_back(VOID_PTR_TYPE);
	argTypes.push_back(llvm::Type::getInt64Ty(*s_Context));
	llvm::FunctionType* pFuncType = llvm::FunctionType::get(VOID_PTR_TYPE, argTypes, false);
	
	// Create a function object with a void pointer input type and a void return type
	llvm::Constant* pFuncConst = s_pModule->getOrInsertFunction(funcName, pFuncType);	
	llvm::Function* pFuncObj = llvm::cast<llvm::Function>(pFuncConst);
	
	// Set the calling convention of the function to the C calling convention
    pFuncObj->setCallingConv(llvm::CallingConv::C);
	
    // Store a pointer to the LLVM function object
	version.pLLVMWrapper = pFuncObj;
	
    // Create an entry basic block for the function
	llvm::BasicBlock* pEntryBlock = llvm::BasicBlock::Create(*s_Context, "entry", pFuncObj);
	llvm::IRBuilder<> entryBuilder(pEntryBlock);
	
	// Get an iterator to the function arguments
	llvm::Function::arg_iterator funcArgItr = pFuncObj->arg_begin();
	
	// Get a pointer to the function's input argument
	llvm::Value* pInArrayArg = pFuncObj->arg_begin();
		
	// Allocate memory for the input structure
	llvm::Value* pInStruct = entryBuilder.CreateAlloca(version.pInStructType);
		
	// For each input argument
	for (size_t i = 0; i < version.inArgTypes.size(); ++i)
	{
		// Create the call to get the ith object from the input array
		LLVMValueVector getArgs;
		getArgs.push_back(pInArrayArg);
		getArgs.push_back(llvm::ConstantInt::get(getIntType(sizeof(size_t)), i));
		llvm::Value* pObjPtr = createNativeCall(
			entryBuilder,
			(void*)ArrayObj::getArrayObj,
			getArgs
		);
		
		// Change the storage mode of the value to the appropriate type
		llvm::Value* pArgVal = changeStorageMode(
			entryBuilder,
			pObjPtr,
			version.inArgObjTypes[i],
			version.inArgStoreModes[i]
		);
		
		// Store the value in the input data struct
		llvm::Value* gepValues[2];
		gepValues[0] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*s_Context), 0);
		gepValues[1] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*s_Context), i);
        llvm::ArrayRef<llvm::Value*> gepValuesRef(gepValues, gepValues+2);
		llvm::Value* pArgAddr = entryBuilder.CreateGEP(pInStruct, gepValuesRef);
		entryBuilder.CreateStore(pArgVal, pArgAddr);
	}
	
	// Get a pointer to the function's output argument count argument
	llvm::Value* pOutArgCount = ++pFuncObj->arg_begin();
	
	// Store the "nargout" value in the input data struct
	llvm::Value* gepValsNargout[2];
	gepValsNargout[0] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*s_Context), 0);
	gepValsNargout[1] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*s_Context), version.inArgTypes.size());
    llvm::ArrayRef<llvm::Value*> gepValsNargoutRef(gepValsNargout, gepValsNargout+2);
	llvm::Value* pNargoutAddr = entryBuilder.CreateGEP(pInStruct, gepValsNargoutRef);
	entryBuilder.CreateStore(pOutArgCount, pNargoutAddr);

	// Declare a value for the return data structure
	llvm::Value* pRetStruct = NULL;
	
	// Get the output parameters for the function
	const ProgFunction::ParamVector& outParams = pFunction->getOutParams();
	
	// If the function has output parameters
	if (outParams.size() != 0)
	{
		// Allocate memory for the output structure
		pRetStruct = entryBuilder.CreateAlloca(version.pOutStructType);
	}
	else
	{
		// Set the return struct pointer to NULL
		pRetStruct = createPtrConst(NULL, version.pOutStructType);
	}
	
	// Create a vector for the function call arguments
	LLVMValueVector fnCallArgs;
	
	// Add the input data structure pointer to the call arguments
	fnCallArgs.push_back(pInStruct);
	
	// Add the return data structure pointer to the call arguments
	fnCallArgs.push_back(pRetStruct);
	
	//std::cout << "Creating call to function" << std::endl;
	
	// Call the compiled function
	entryBuilder.CreateCall(version.pLLVMFunc, fnCallArgs);
	
	//std::cout << "Created call to function" << std::endl;

	//std::cout << "Creating arrayobj create call" << std::endl;
	
	// Create the call to create an array object to store the output
	LLVMValueVector createArgs;
	createArgs.push_back(llvm::ConstantInt::get(getIntType(sizeof(size_t)), outParams.size()));
	llvm::Value* pOutArrayObj = createNativeCall(
		entryBuilder,
		(void*)ArrayObj::create,
		createArgs
	);
	
	//std::cout << "Handling return parameters..." << std::endl;
	
	// If the function has no return parameters
	if (outParams.size() == 0)
	{
		// Return the empty array object
		entryBuilder.CreateRet(pOutArrayObj);
	}
	else
	{
		// Get the address of the number struct field
		llvm::Value* gepValuesNum[2];
		gepValuesNum[0] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*s_Context), 0);
		gepValuesNum[1] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*s_Context), outParams.size());	
        llvm::ArrayRef<llvm::Value*> gepValuesRef(gepValuesNum, gepValuesNum+2);
		llvm::Value* pNumFieldAddr = entryBuilder.CreateGEP(pRetStruct, gepValuesRef);
		
		// Load the number of values written into the output structure
		llvm::Value* pNumOutVals = entryBuilder.CreateLoad(pNumFieldAddr);
		
		// Create an initial basic block
		llvm::BasicBlock* pCurrentBlock = llvm::BasicBlock::Create(*s_Context, "", pFuncObj);
		llvm::IRBuilder<> currentBuilder(pCurrentBlock);		
		
		// Jump from the entry block to the current basic block
		entryBuilder.CreateBr(pCurrentBlock);
		
	    // Create a basic block for the function return
		llvm::BasicBlock* pRetBlock = llvm::BasicBlock::Create(*s_Context, "ret", pFuncObj);
		llvm::IRBuilder<> retBuilder(pRetBlock);
				
		// Create a return instruction to return the output structure
		retBuilder.CreateRet(pOutArrayObj);
		
		// For each output parameter
		for (size_t i = 0; i < outParams.size(); ++i)
		{
		    // Create a basic block to write the current value to the structure
			llvm::BasicBlock* pWriteBlock = llvm::BasicBlock::Create(*s_Context, "", pFuncObj);
			llvm::IRBuilder<> writeBuilder(pWriteBlock);			
			
			// Get the address of the struct field
			llvm::Value* gepValuesVal[2];
			gepValuesVal[0] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*s_Context), 0);
			gepValuesVal[1] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*s_Context), i);
            llvm::ArrayRef<llvm::Value*> gepValRef(gepValuesVal, gepValuesVal+2);
			llvm::Value* pValFieldAddr = writeBuilder.CreateGEP(pRetStruct, gepValRef);
							
			// Store the value in the output structure
			llvm::Value* pValue = writeBuilder.CreateLoad(pValFieldAddr);
			
			// Change the storage mode of the value to the object pointer type
			llvm::Value* pValObj = changeStorageMode(
				writeBuilder,
				pValue,
				version.outArgObjTypes[i],
				VOID_PTR_TYPE
			);
			
			// Create the call to store the object in the output array
			LLVMValueVector addArgs;
			addArgs.push_back(pOutArrayObj);
			addArgs.push_back(pValObj);
			createNativeCall(
				writeBuilder,
				(void*)ArrayObj::addObject,
				addArgs
			);
	
			// Compare the current index to the number of output values
			llvm::Value* pCurIndex = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*s_Context), i);
			llvm::Value* pCompVal = currentBuilder.CreateICmpEQ(pCurIndex, pNumOutVals);
				
			// If the pointer is null, stop the process
			currentBuilder.CreateCondBr(pCompVal, pRetBlock, pWriteBlock);
		
			// If this is the last output parameter
			if (i == outParams.size() - 1)
			{
				// Jump to the set number block and stop the process
				writeBuilder.CreateBr(pRetBlock);
			}
			else
			{
				// Create a new current basic block
				pCurrentBlock = llvm::BasicBlock::Create(*s_Context, "", pFuncObj);
				currentBuilder.SetInsertPoint(pCurrentBlock);
				
				// Jump from the write block to the new current basic block
				writeBuilder.CreateBr(pCurrentBlock);
			}
		}
	}
	
	// Run the optimization passes on the function
	s_pFunctionPasses->run(*pFuncObj);
	
	// Get a function pointer to the compiled function
	WRAPPER_FUNC_PTR pFuncPtr = (WRAPPER_FUNC_PTR)s_pExecEngine->getPointerToFunction(pFuncObj);
	
	// Store a pointer to the compiled function code
	version.pWrapperPtr = pFuncPtr;
	
	// If we are in verbose mode
	if (ConfigManager::s_verboseVar)
	{
		// Log that compilation is complete
		std::cout << "Done compiling wrapper func" << std::endl;
		
		// Run the pretty-printer pass on the function
		s_pPrintPass->run(*pFuncObj);
	}
}

/***************************************************************
* Function: JITCompiler::compStmtSeq()
* Purpose : Compile a statement sequence
* Initial : Maxime Chevalier-Boisvert on March 9, 2009
****************************************************************
Revisions and bug fixes:
*/
llvm::BasicBlock* JITCompiler::compStmtSeq(
	StmtSequence* pSequence,
	CompFunction& function,
	CompVersion& version,
	VariableMap varMap,
	BranchPoint& exitPoint,
	BranchList& contPoints,
	BranchList& breakPoints,
	BranchList& returnPoints
)
{
	// Create a basic block to start the sequence
	llvm::BasicBlock* pSeqBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
	
	// Create an IR builder for the current basic block
	llvm::IRBuilder<> seqBuilder(pSeqBlock);
	
	// Store a pointer to the current basic block
	llvm::BasicBlock* pCurBlock = pSeqBlock;
	
	// Get a reference to the statement vector
	const StmtSequence::StmtVector& stmts = pSequence->getStatements();
	
	// For each statement in the sequence
	for (StmtSequence::StmtVector::const_iterator stmtItr = stmts.begin(); stmtItr != stmts.end(); ++stmtItr)
	{
		// Get a pointer to the statement
		Statement* pStatement = *stmtItr;

		// Declare a variable to store the exit point
		BranchPoint stmtExitPoint;
		
		// Compile the statement
		llvm::BasicBlock* pStmtBlock = compStatement(
			pStatement,
			function,
			version,
			varMap,
			stmtExitPoint,
			contPoints,
			breakPoints,
			returnPoints
		);
	
		// Add a branch to the statement's entry block
		seqBuilder.CreateBr(pStmtBlock);
		
		// If there is no exit point for this statement
		if (stmtExitPoint.first == NULL)
		{
			// This block has no exit point
			exitPoint.first = NULL;
			
			// Return early
			return pSeqBlock;
		}
		
		// Update the current variable map
		varMap = stmtExitPoint.second;
		
		// Make the exit point block the current basic block
		pCurBlock = stmtExitPoint.first;
		seqBuilder.SetInsertPoint(stmtExitPoint.first);
	}
	
	// Set the exit point basic block to the current block
	exitPoint.first = pCurBlock; 
	
	// Set the exit point variable map to the current variable map
	exitPoint.second = varMap;
	
	// Return the sequence basic block
	return pSeqBlock;
}

/***************************************************************
* Function: JITCompiler::genCopyCode()
* Purpose : Generate copies for a block of stmts
* Initial : Nurudeen Lameed on December 29, 2009
****************************************************************
Revisions and bug fixes:
*/
void JITCompiler::genCopyCode(
	llvm::IRBuilder<>& irBuilder,
	CompFunction& function,
	CompVersion& version,
	const VarTypeMap& typeMap,
	VariableMap& varMap,
	const Statement* pStmt
)
{
	assert(version.pArrayCopyInfo);
	CPMap::const_iterator iter = 
		version.pArrayCopyInfo->copyMap.find(pStmt);
	if (iter != version.pArrayCopyInfo->copyMap.end() )
	{
		const BlockCopyVecs& cpVecs = iter->second;
		const StmtCopyVec& copies =  cpVecs[0];
		for (size_t i = 0; i < copies.size(); ++i)
		{		
			genCopyCode(irBuilder, function, version, typeMap, varMap, copies[i]);
		}
	}
}

/***************************************************************
* Function: JITCompiler::genCopyCode()
* Purpose : Generate a single copy for a statement
* Initial : Nurudeen Lameed on December 29, 2009
****************************************************************
Revisions and bug fixes:
*/
void JITCompiler::genCopyCode(
		llvm::IRBuilder<>& irBuilder,
		CompFunction& function,
		CompVersion& version,
		const VarTypeMap& typeMap,
		VariableMap& varMap,
		const CopyInfo& cpEntry
)
{
	if (cpEntry.first)	// the symbol has not been moved
	{	
		// const Expression::SymbolSet& members = cpEntry.second;
		SymbolExpr* pSymbol = (SymbolExpr*)cpEntry.first;				
		VariableMap::const_iterator iter = varMap.find(pSymbol);
		assert(iter != varMap.end());
		VarTypeMap::const_iterator tIter = typeMap.find(pSymbol);
		assert(tIter != typeMap.end());
		Value lVal = iter->second;
		
		if (!lVal.pValue)
		{	
			// read from the environment
			lVal = readVariable(irBuilder, function, version, tIter->second, pSymbol, false);
		}
		
		//if (mcvmVal.objType >=  DataObject::MATRIX_I32 && mcvmVal.objType <= DataObject::CELLARRAY)	
		if (lVal.pValue->getType() == VOID_PTR_TYPE)
		{
			// generate a check code and then the copy
			//if ( members.size() > 0)
			//{
			  // TODO:: generate check code here and the copy code 
		   //}

		  //else	always generate copies for now
			{
				LLVMValueVector callArgs;
				callArgs.push_back(lVal.pValue);
				Value newVal(0, lVal.objType);
				newVal.pValue = createNativeCall(
					irBuilder,
					(void*)DataObject::copyObject,
					callArgs
				);
				
				varMap[pSymbol] = newVal;
			}
		}
	}
}

/***************************************************************
* Function: JITCompiler::genLoopHeaderCopy()
* Purpose : copy code at a loop's header
* Initial : Nurudeen Lameed on December 29, 2009
****************************************************************
Revisions and bug fixes:
*/
llvm::BasicBlock* JITCompiler::genLoopHeaderCopy(
	llvm::BasicBlock* curBlock,
	StmtSequence* pTestSeq,
	CompFunction& function,
	CompVersion& version,
	LoopStmt* pLoopStmt,
	SymbolExpr* pTestVar,
	VariableMap& postInitVarMap
)
{
	assert(version.pArrayCopyInfo);
	CPMap::const_iterator iter = version.pArrayCopyInfo->copyMap.find(pLoopStmt);
	if (iter != version.pArrayCopyInfo->copyMap.end() )
	{
		const BlockCopyVecs& cpVecs = iter->second;
		const StmtCopyVec& copies = cpVecs[1];
		if (copies.size() > 0)
		{
			llvm::IRBuilder<>copyBuilder(curBlock);
			
			// 	generate test code first  ...
			TypeInfoMap::const_iterator typeItr = version.pTypeInferInfo->preTypeMap.find(pLoopStmt);
			assert (typeItr != version.pTypeInferInfo->preTypeMap.end());
			
			BranchList breakPoints;
			BranchList returnPoints;
			BranchList contPoints;
			BranchPoint testExit;
				
			// Compile the test sequence
			llvm::BasicBlock* pTestBlock = JITCompiler::compStmtSeq(
				pTestSeq,
				function,
				version,
				postInitVarMap,
				testExit,
				contPoints,
				breakPoints,
				returnPoints
			);	
			
			copyBuilder.CreateBr(pTestBlock);						// link with the test block
			copyBuilder.SetInsertPoint(testExit.first);
			VariableMap& oVarValueMap =  testExit.second;           //    postInitVarMap;
			VariableMap nVarValueMap =  testExit.second; 
			
			// Get the type information after the test sequence
			TypeInfoMap::const_iterator postTestTypeItr = version.pTypeInferInfo->postTypeMap.find(pTestSeq);
			assert (postTestTypeItr != version.pTypeInferInfo->postTypeMap.end());
			const VarTypeMap& postTestVarTypes = postTestTypeItr->second;
			
			// Read the loop test variable, if needed
			Expression::SymbolSet testVarSet;
			testVarSet.insert(pTestVar);
			readVariables(copyBuilder, function, version, postTestVarTypes, oVarValueMap, testVarSet);
			
			// Get the value of the loop test variable
			Value testValue = oVarValueMap[pTestVar];
			
			// Get the boolean value of the test value
			llvm::Value* pBoolVal = changeStorageMode(
				copyBuilder,
				testValue.pValue,
				testValue.objType,
				llvm::Type::getInt1Ty(*s_Context)
			);

			llvm::BasicBlock* pCopyTrue = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
			llvm::BasicBlock* pCopyFalse = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
			llvm::BasicBlock* pCopyExit = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
			
			copyBuilder.CreateCondBr(pBoolVal, pCopyTrue, pCopyFalse);  // if (cond)
			copyBuilder.SetInsertPoint(pCopyTrue);
			
			// Get the type information before the test sequence
			TypeInfoMap::const_iterator preTestTypeItr = version.pTypeInferInfo->preTypeMap.find(pTestSeq);
			assert (preTestTypeItr != version.pTypeInferInfo->preTypeMap.end());
			const VarTypeMap& preTestVarTypes = preTestTypeItr->second;
			
			// generate code, if true			
			for (size_t i = 0; i < copies.size(); ++i)
			{		
				if (copies[i].first)
				{
					// generate copy code
					genCopyCode(copyBuilder, function, version, preTestVarTypes, nVarValueMap , copies[i]);
				}
			}
			
			copyBuilder.CreateBr(pCopyExit);
			copyBuilder.SetInsertPoint(pCopyFalse);
			
			// read the old values into this block
			for (size_t i = 0; i < copies.size(); ++i)
			{	
				if (copies[i].first)
				{
					SymbolExpr* pSymbol = const_cast<SymbolExpr*>(copies[i].first);
					Value oValue = oVarValueMap[pSymbol]; 
					
					if (oValue.pValue == 0)
					{	
						VarTypeMap::const_iterator tIter = preTestVarTypes.find(pSymbol);
						assert(tIter != preTestVarTypes.end());
						
						// read from the environment
						oVarValueMap[pSymbol] = readVariable(copyBuilder, function, version, tIter->second, pSymbol, false);
					}
				}
			}
						
			copyBuilder.CreateBr(pCopyExit);
			copyBuilder.SetInsertPoint(pCopyExit);
			
			// for each copy created, create a phi node 			
			for (size_t i = 0; i < copies.size(); ++i)
			{	
				if (copies[i].first)
				{
					SymbolExpr* pSymbol = const_cast<SymbolExpr*>(copies[i].first);
					Value oValue = oVarValueMap[pSymbol]; 
					Value nValue = nVarValueMap[pSymbol];
					llvm::PHINode* pPhiNode = copyBuilder.CreatePHI(oValue.pValue->getType(), 2);
					pPhiNode->addIncoming(oValue.pValue, pCopyFalse);
					pPhiNode->addIncoming(nValue.pValue, pCopyTrue);
					postInitVarMap[pSymbol] = Value(pPhiNode, oValue.objType);
				}
			}
			
			curBlock = pCopyExit;
		}	
	}
	
	return curBlock;
}
/***************************************************************
* Function: JITCompiler::compStatement()
* Purpose : Compile a statement
* Initial : Maxime Chevalier-Boisvert on March 10, 2009
****************************************************************
Revisions and bug fixes:
*/
llvm::BasicBlock* JITCompiler::compStatement(
	Statement* pStatement,
	CompFunction& function,
	CompVersion& version,
	VariableMap varMap,
	BranchPoint& exitPoint,
	BranchList& contPoints,
	BranchList& breakPoints,
	BranchList& returnPoints
)
{
	// If we are in verbose mode, log the statement compilation
	if (ConfigManager::s_verboseVar)
		std::cout << "Compiling statement: " << pStatement->toString() << std::endl;
	
	// Switch on the statement type
	switch (pStatement->getStmtType())
	{
		// Assignment statement
		case Statement::ASSIGN:
		{
			// Compile the assignment statement
			return compAssignStmt(
				(AssignStmt*)pStatement,
				function,
				version,
				varMap,
				exitPoint
			);
			
		}
		break;
		
		// Expression statement
		case Statement::EXPR:
		{
			// Compile the expression statement
			return compExprStmt(
				(ExprStmt*)pStatement,
				function,
				version,
				varMap,
				exitPoint
			);
		}
		break;
	
		// Break statement
		case Statement::BREAK:
		{
			// Create a basic block for the statement
			llvm::BasicBlock* pStmtBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
			
			// Add the basic block and variable map to the break point list
			breakPoints.push_back(BranchPoint(pStmtBlock, varMap));
			
			// Make the exit point basic block null, there is none
			exitPoint.first = NULL;
			
			// Return the statement's block
			return pStmtBlock;
		}
		break;
			
		// Continue statement
		case Statement::CONTINUE:
		{
			// Create a basic block for the statement
			llvm::BasicBlock* pStmtBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);

			// Add the basic block and variable map to the continue point list
			contPoints.push_back(BranchPoint(pStmtBlock, varMap));
			
			// Make the exit point basic block null, there is none
			exitPoint.first = NULL;
			
			// Return the statement's block
			return pStmtBlock;
		}
		break;

		// Return statement
		case Statement::RETURN:
		{
			// Create a basic block for the statement
			llvm::BasicBlock* pStmtBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);

			// Add the basic block and variable map to the return point list
			returnPoints.push_back(BranchPoint(pStmtBlock, varMap));
			
			// Make the exit point basic block null, there is none
			exitPoint.first = NULL;
			
			// Return the statement's block
			return pStmtBlock;
		}
		break;
		
		// If-else statement
		case Statement::IF_ELSE:
		{
			
			// Compile the if-else statement
			return compIfElseStmt(
				(IfElseStmt*)pStatement,
				function,
				version,
				varMap,
				exitPoint,
				contPoints,
				breakPoints,
				returnPoints
			);
			
			
		}
		break;
	
		// Loop statement
		case Statement::LOOP:
		{
			// Compile the loop statement
			return compLoopStmt(
				(LoopStmt*)pStatement,
				function,
				version,
				varMap,
				exitPoint,
				returnPoints
			);
		}
		break;
			
		// Any other statement type
		default:
		{
			// Get a pointer to the current call environment
			llvm::Value* pEnvObject = getCallEnv(function, version);			
			
			// Create an llvm pointer constant for the statement pointer
			llvm::Value* pStmtPtr = createPtrConst(pStatement);
			
			// Get the function object for the "execStatement" function
			const NativeFunc& execStmtFunc = s_nativeMap[(void*)Interpreter::execStatement];
			
			// Create a basic block for the statement
			llvm::BasicBlock* pStmtBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
			llvm::IRBuilder<> builder(pStmtBlock);
			
			// Write the symbols used by the statement to the environment
			writeVariables(builder,	function, version, varMap, pStatement->getSymbolUses());
			
			// Create a call to execute this statement
			builder.CreateCall2(execStmtFunc.pLLVMFunc, pStmtPtr, pEnvObject);
			
			// Mark the symbols defined by the statement as written to the environment
			markVarsWritten(varMap,	pStatement->getSymbolDefs());
			
			// Set the exit point parameters
			exitPoint.first = pStmtBlock;
			exitPoint.second = varMap;			
			
			// Return the statement's block
			return pStmtBlock;
		}
	}
}

/***************************************************************
* Function: JITCompiler::compExprStmt()
* Purpose : Compile an expression statement
* Initial : Maxime Chevalier-Boisvert on March 22, 2009
****************************************************************
Revisions and bug fixes:
*/
llvm::BasicBlock* JITCompiler::compExprStmt(
	ExprStmt* pExprStmt,
	CompFunction& function,
	CompVersion& version,
	VariableMap varMap,
	BranchPoint& exitPoint
)
{
	// If the output is suppressed
	if (pExprStmt->getSuppressFlag() == true)
	{
		// Create entry and exit basic blocks for the expression compilation
		llvm::BasicBlock* pEntryBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
		llvm::BasicBlock* pExitBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);

		// Find the live variables after this statement
		LiveVarMap::const_iterator liveItr = version.pLiveVarInfo->liveVarMap.find(pExprStmt);
		assert (liveItr != version.pLiveVarInfo->liveVarMap.end());
		
		// Find the reaching definitions before this statement
		ReachDefMap::const_iterator defItr = version.pReachDefInfo->reachDefMap.find(pExprStmt);
		assert (defItr != version.pReachDefInfo->reachDefMap.end());

		// Find the type information before this statement
		TypeInfoMap::const_iterator typeItr = version.pTypeInferInfo->preTypeMap.find(pExprStmt);
		assert (typeItr != version.pTypeInferInfo->preTypeMap.end());
		
		// Get a pointer to the contained expression
		Expression* pExpr = pExprStmt->getExpression();
		
		// If this is a symbol expression
		if (pExpr->getExprType() == Expression::SYMBOL)
		{
			// Compile the symbol expression requesting 0 output values
			compSymbolExpr(
				(SymbolExpr*)pExpr,
				0,
				function,
				version,
				liveItr->second,
				defItr->second,
				typeItr->second,
				varMap,
				pEntryBlock,
				pExitBlock
			);
		}
		
		// Otherwise, if this is a parameterized expression
		else if (pExpr->getExprType() == Expression::PARAM)
		{
			// Compile the parameterized expression requesting 0 output values
			compParamExpr(
				(ParamExpr*)pExpr,
				0,
				function,
				version,
				liveItr->second,
				defItr->second,
				typeItr->second,
				varMap,
				pEntryBlock,
				pExitBlock
			);
		}
		
		// For any other expression type
		else
		{
			// Evaluate the expression
			compExpression(
				pExpr,
				function,
				version,
				liveItr->second,
				defItr->second,
				typeItr->second,
				varMap,
				pEntryBlock,
				pExitBlock
			);
		}
	
		// Set-up the exit point parameters
		exitPoint.first = pExitBlock;
		exitPoint.second = varMap;
		
		// Return a pointer to the entry block
		return pEntryBlock;
	}
	
	// Get a pointer to the function's environment
	llvm::Value* pEnvArg = getCallEnv(function, version);
	
	// Create an llvm pointer constant for the statement pointer
	llvm::Value* pStmtPtr = createPtrConst(pExprStmt);
		
	// Create a basic block for the statement
	llvm::BasicBlock* pStmtBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
	llvm::IRBuilder<> builder(pStmtBlock);
	
	// Write the symbols used by the statement to the environment
	writeVariables(builder,	function, version, varMap, pExprStmt->getSymbolUses());
	
	// Get the function object for the "evalAssignStmt" function
	const NativeFunc& execStmtFunc = s_nativeMap[(void*)Interpreter::evalExprStmt];
	
	// Create a call to execute this statement
	builder.CreateCall2(execStmtFunc.pLLVMFunc, pStmtPtr, pEnvArg);
	
	// Set the exit point parameters
	exitPoint.first = pStmtBlock;
	exitPoint.second = varMap;
	
	// Return the statement's block
	return pStmtBlock;
}

/***************************************************************
* Function: JITCompiler::compAssignStmt()
* Purpose : Compile an assignment statement
* Initial : Maxime Chevalier-Boisvert on March 22, 2009
****************************************************************
Revisions and bug fixes:
*/
llvm::BasicBlock* JITCompiler::compAssignStmt(
	AssignStmt* pAssignStmt,
	CompFunction& function,
	CompVersion& version,
	VariableMap varMap,
	BranchPoint& exitPoint
)
{
	// If the output is not suppressed
	if (pAssignStmt->getSuppressFlag() == false)
	{
		// Create a basic block for the statement
		llvm::BasicBlock* pStmtBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
		llvm::IRBuilder<> builder(pStmtBlock);
			
		// Write the symbols used by the statement to the environment
		writeVariables(builder, function, version, varMap, pAssignStmt->getSymbolUses());
		
		// Create a call to execute this statement
		LLVMValueVector callArgs;
		callArgs.push_back(createPtrConst(pAssignStmt));
		callArgs.push_back(getCallEnv(function, version));
		createNativeCall(
			builder,
			(void*)Interpreter::evalAssignStmt,
			callArgs
		);	
		
		// Mark the symbols defined by the statement as written to the environment
		markVarsWritten(varMap,	pAssignStmt->getSymbolDefs());
		
		// Set the exit point parameters
		exitPoint.first = pStmtBlock;
		exitPoint.second = varMap;
		
		// Return the statement's block
		return pStmtBlock;		
	}

	// Get the list of left expressions
	const AssignStmt::ExprVector& leftExprs = pAssignStmt->getLeftExprs();
	
	// Get the right expression
	Expression* pRightExpr = pAssignStmt->getRightExpr();	

	// Find the live variables after this statement
	LiveVarMap::const_iterator liveItr = version.pLiveVarInfo->liveVarMap.find(pAssignStmt);
	assert (liveItr != version.pLiveVarInfo->liveVarMap.end());
	
	// Find the reaching definitions before this statement
	ReachDefMap::const_iterator defItr = version.pReachDefInfo->reachDefMap.find(pAssignStmt);
	assert (defItr != version.pReachDefInfo->reachDefMap.end());
	
	// Find the type information before this statement
	TypeInfoMap::const_iterator typeItr = version.pTypeInferInfo->preTypeMap.find(pAssignStmt);
	assert (typeItr != version.pTypeInferInfo->preTypeMap.end());

	// Create entry and exit basic blocks for the expression compilation
	llvm::BasicBlock* pEntryBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
	llvm::BasicBlock* pAfterBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
	
	// Declare a vector to store the values for the right expression
	ValueVector rightValues;	
	
	// If the right expression is a symbol expression
	if (pRightExpr->getExprType() == Expression::SYMBOL)
	{
		// Compile the symbol expression
		rightValues = compSymbolExpr(
			(SymbolExpr*)pRightExpr,
			leftExprs.size(),
			function,
			version,
			liveItr->second,
			defItr->second,
			typeItr->second,
			varMap,
			pEntryBlock,
			pAfterBlock
		);
	}
	
	// Otherwise, if this is a parameterized expression
	else if (pRightExpr->getExprType() == Expression::PARAM)
	{
		// Compile the parameterized expression
		rightValues = compParamExpr(
			(ParamExpr*)pRightExpr,
			leftExprs.size(),
			function,
			version,
			liveItr->second,
			defItr->second,
			typeItr->second,
			varMap,
			pEntryBlock,
			pAfterBlock
		);
	}
	
	// For any other expression type
	else
	{
		// If there is more than one left expression
		if (leftExprs.size() > 1)
		{
			// Throw a compilation error
			throw CompError("the expression does not produce enough output values", pAssignStmt);
		}
		
		// Evaluate the expression
		Value value = compExpression(
			pRightExpr,
			function,
			version,
			liveItr->second,
			defItr->second,
			typeItr->second,
			varMap,
			pEntryBlock,
			pAfterBlock
		);
		
		// Add the single value to the vector
		rightValues.push_back(value);
	}

	// Setup an IR builder for the current basic block
	llvm::IRBuilder<> currentBuilder(pAfterBlock);
	
	StmtCopyVec copies;
	if (s_jitCopyEnableVar)
	{
		assert(version.pArrayCopyInfo);
		CPMap::const_iterator iter = 
			version.pArrayCopyInfo->copyMap.find(pAssignStmt);
		
		if (iter != version.pArrayCopyInfo->copyMap.end() )
		{
			copies = (iter->second)[0];
		}
	}
	
	// For each left expression
	for (size_t i = 0; i < leftExprs.size(); ++i)
	{			
		// Get a pointer to the left-expression
		Expression* pLeftExpr = leftExprs[i];
		
		// Get a reference to the associated value
		const Value& rightVal = rightValues[i];	
		
		// If the left-expression is a symbol
		if (pLeftExpr->getExprType() == Expression::SYMBOL)
		{
			// Get a typed pointer to the symbol expression
			SymbolExpr* pSymbol = (SymbolExpr*)pLeftExpr;

			// Set the variable map binding for this symbol
			varMap[pSymbol] = rightVal;

			if (s_jitCopyEnableVar && i < copies.size())
			{
				genCopyCode(currentBuilder, function, version,  typeItr->second, varMap, copies[i]);
			}
		}
		
		// If the left-expression is a parameterized expression
		else if (pLeftExpr->getExprType() == Expression::PARAM)
		{
			
			if (s_jitCopyEnableVar && i < copies.size())
			{
				// generate copy code
				genCopyCode(currentBuilder, function, version, typeItr->second,  varMap, copies[i]);
			}
					
			// Get a typed pointer to the parameterized expression
			ParamExpr* pParamExpr = (ParamExpr*)pLeftExpr;
			
			// Get the symbol expression
			SymbolExpr* pSymbol = pParamExpr->getSymExpr();
			
			// Get the parameterized expression arguments
			const Expression::ExprVector& arguments = pParamExpr->getArguments();
					
			// Get the type set associated with the symbol
			VarTypeMap::const_iterator symTypeItr = typeItr->second.find(pSymbol);
			TypeSet symTypes = (symTypeItr != typeItr->second.end())? symTypeItr->second:TypeSet();
			
			// Get the storage mode for this variable
			DataObject::Type symObjType;
			getStorageMode(
				symTypes,
				symObjType
			);
			
			// Declare a flag to indicate that all arguments are scalars
			bool argsScalar = true;
			
			// For each argument
			for (ParamExpr::ExprVector::const_iterator argItr = arguments.begin(); argItr != arguments.end(); ++argItr)
			{
				// Get a pointer to the argument expression
				Expression* pArgExpr = *argItr;
				
				// Declare a variable for the storage mode
				llvm::Type* storageMode;
				
				//std::cout << "Finding arg type for expr: " << pArgExpr->toString() << std::endl;
				
				// If the expression is a symbol 
				if (pArgExpr->getExprType() == Expression::SYMBOL)
				{
					// Get the type set associated with the symbol
					VarTypeMap::const_iterator symTypeItr = typeItr->second.find((SymbolExpr*)pArgExpr);
					TypeSet symTypes = (symTypeItr != typeItr->second.end())? symTypeItr->second:TypeSet();
					
					// Get the storage mode for this variable
					DataObject::Type objType;
					storageMode = getStorageMode(
						symTypes,
						objType
					);
				}
				else
				{
					// Get the type set associated with the expression
					ExprTypeMap::const_iterator typeItr = version.pTypeInferInfo->exprTypeMap.find(pArgExpr);
					assert (typeItr != version.pTypeInferInfo->exprTypeMap.end());
					TypeSet exprTypes = (typeItr->second.size() == 1)? typeItr->second[0]:TypeSet();
					
					// Get the storage mode for this expression
					DataObject::Type objType;
					storageMode = getStorageMode(
						exprTypes,
						objType
					);			
				}
				
				// If the storage mode is not int64 or float64, set the scalar arguments flag to false
				if (storageMode != llvm::Type::getInt64Ty(*s_Context) && 
                                        storageMode != llvm::Type::getDoubleTy(*s_Context))
					argsScalar = false;		
			}		
			
			// If we can establish that the symbol object is a matrix
			// and all the arguments are scalar
			// and the right expression is a non-complex scalar value
			// and array access optimizations are enabled
			if (symObjType >= DataObject::MATRIX_I32 && symObjType <= DataObject::CHARARRAY && 
				symObjType != DataObject::MATRIX_C128 && argsScalar &&
				rightVal.pValue->getType() != VOID_PTR_TYPE && rightVal.objType != DataObject::MATRIX_C128 &&
				s_jitUseArrayOpts.getBoolValue() == true)
			{
				// Create a basic block for the symbol evaluation exit
				llvm::BasicBlock* pSymExitBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);			
				
				// Compile the symbol evaluation
				Value symValue = compSymbolEval(
					(SymbolExpr*)pSymbol,
					function,
					version,
					liveItr->second,
					defItr->second,
					typeItr->second,
					varMap,
					currentBuilder.GetInsertBlock(),
					pSymExitBlock
				);			
				
				// Make the symbol eval exit block the current basic block
				currentBuilder.SetInsertPoint(pSymExitBlock);
				
				// Set the storage mode of the variable to the object pointer type
				llvm::Value* pSymObject = changeStorageMode(
					currentBuilder,
					symValue.pValue,
					symValue.objType,
					VOID_PTR_TYPE
				);
				
				// Create a vector to store the argument values
				std::vector<llvm::Value*> argValues;
				
				// For each argument
				for (ParamExpr::ExprVector::const_iterator argItr = arguments.begin(); argItr != arguments.end(); ++argItr)
				{
					// Get a pointer to the argument expression
					Expression* pArgExpr = *argItr;
					
					// Create a basic block for the argument evaluation exit
					llvm::BasicBlock* pArgExitBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
					
					// Compile the argument expression to get its value
					Value argValue = compExpression(
						pArgExpr,
						function,
						version,
						liveItr->second,
						defItr->second,
						typeItr->second,
						varMap,
						currentBuilder.GetInsertBlock(),
						pArgExitBlock
					);
					
					// Update the current IR builder
					currentBuilder.SetInsertPoint(pArgExitBlock);
					
					// Set the storage mode of the argument to int64
					llvm::Value* pIntArgVal = changeStorageMode(
						currentBuilder,
						argValue.pValue,
						argValue.objType,
						llvm::Type::getInt64Ty(*s_Context)
					);
					
					// Add the value to the vector
					argValues.push_back(pIntArgVal);
				}	
				
				// Create a basic block for the array write exit
				llvm::BasicBlock* pWriteExitBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);				
				
				// Generate code for the scalar array write operation 
				compArrayWrite(
					pSymObject,
					symObjType,
					rightVal.pValue,
					rightVal.objType,
					argValues,
					pParamExpr,
					function,
					version,
					currentBuilder.GetInsertBlock(),
					pWriteExitBlock
				);
				
				// Make the symbol eval exit block the current basic block
				currentBuilder.SetInsertPoint(pWriteExitBlock);
			}
			
			// Otherwise, if we cannot compile this parameterized assignment directly
			else
			{
				// Set the storage mode of the right object to the object pointer type
				llvm::Value* pRightObject = changeStorageMode(
					currentBuilder,
					rightVal.pValue,
					rightVal.objType,
					VOID_PTR_TYPE
				);
			
				// Write the symbols used by the parameterized left expression to the environment
				writeVariables(currentBuilder, function, version, varMap, pParamExpr->getSymbolUses());
				
				// Create a call to perform the object assignment
				LLVMValueVector assignArgs;
				assignArgs.push_back(createPtrConst(pLeftExpr));
				assignArgs.push_back(pRightObject);
				assignArgs.push_back(getCallEnv(function, version));
				assignArgs.push_back(llvm::ConstantInt::get(llvm::Type::getInt8Ty(*s_Context), 0));
				createNativeCall(
					currentBuilder,
					(void*)Interpreter::assignObject,
					assignArgs
				);	
			}			
		}
		
		// If the left-expression is a cell-indexing expression
		else if (pLeftExpr->getExprType() == Expression::CELL_INDEX)
		{
			// Set the storage mode of the right object to the object pointer type
			llvm::Value* pRightObject = changeStorageMode(
				currentBuilder,
				rightVal.pValue,
				rightVal.objType,
				VOID_PTR_TYPE
			);
		
			// Write the symbols used by the left expression to the environment
			writeVariables(currentBuilder, function, version, varMap, pLeftExpr->getSymbolUses());
			
			// Create a call to perform the object assignment
			LLVMValueVector assignArgs;
			assignArgs.push_back(createPtrConst(pLeftExpr));
			assignArgs.push_back(pRightObject);
			assignArgs.push_back(getCallEnv(function, version));
			assignArgs.push_back(llvm::ConstantInt::get(llvm::Type::getInt8Ty(*s_Context), 0));
			createNativeCall(
				currentBuilder,
				(void*)Interpreter::assignObject,
				assignArgs
			);
		}
		
		// Other left-expression types
		else
		{
			// Throw an exception
			throw CompError("unsupported left-expression type in assignment", pLeftExpr);
		}		
	}
	
	// Set-up the exit point parameters
	exitPoint.first = currentBuilder.GetInsertBlock();
	exitPoint.second = varMap;
	
	// Return a pointer to the entry block
	return pEntryBlock;
}

/***************************************************************
* Function: JITCompiler::compIfElseStmt()
* Purpose : Compile an if-else statement
* Initial : Maxime Chevalier-Boisvert on March 10, 2009
****************************************************************
Revisions and bug fixes:
*/
llvm::BasicBlock* JITCompiler::compIfElseStmt(
	IfElseStmt* pIfStmt,
	CompFunction& function,
	CompVersion& version,
	VariableMap varMap,
	BranchPoint& exitPoint,
	BranchList& contPoints,
	BranchList& breakPoints,
	BranchList& returnPoints
)
{

	// generate code for any copies before this statement
	// Create a basic block for the statement
	llvm::BasicBlock* pCopyBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
		
	// Create basic blocks for the condition evaluation
	llvm::BasicBlock* pEntryBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
	llvm::BasicBlock* pTestBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);

	// Get the condition expression
	Expression* pTestExpr = pIfStmt->getCondition();
	
	// Find the live variables after the condition expression
	LiveVarMap::const_iterator testLiveItr = version.pLiveVarInfo->liveVarMap.find(pTestExpr);
	assert (testLiveItr != version.pLiveVarInfo->liveVarMap.end());
	
	// Find the reaching definitions before the condition expression
	ReachDefMap::const_iterator testDefItr = version.pReachDefInfo->reachDefMap.find(pTestExpr);
	assert (testDefItr != version.pReachDefInfo->reachDefMap.end());
	
	// Find the type information before this statement
	TypeInfoMap::const_iterator testTypeItr = version.pTypeInferInfo->preTypeMap.find(pTestExpr);
	assert (testTypeItr != version.pTypeInferInfo->preTypeMap.end());
	
	llvm::IRBuilder<> copyBuilder(pCopyBlock);
	if (s_jitCopyEnableVar)
	{
		genCopyCode(
			copyBuilder,
			function,
			version,
			testTypeItr->second,
			varMap,
			(IfElseStmt*)pIfStmt
		);
	}
	copyBuilder.CreateBr(pEntryBlock);
	
	llvm::IRBuilder<> testBuilder(pTestBlock);
	
	// Compile the condition expression
	Value condVal = compExpression(
		pTestExpr,
		function,
		version,
		testLiveItr->second,
		testDefItr->second,
		testTypeItr->second,
		varMap,
		pEntryBlock,
		pTestBlock
	);
	
	// Get the boolean value of the condition value
	llvm::Value* pBoolVal = changeStorageMode(
		testBuilder,
		condVal.pValue,
		condVal.objType,
		llvm::Type::getInt1Ty(*s_Context)
	);
	
	// Declare branch points for the if and else block exit points
	BranchPoint ifBlockExit;
	BranchPoint elseBlockExit;
	
	// Compile the if block
	llvm::BasicBlock* pIfBlock = JITCompiler::compStmtSeq(
		pIfStmt->getIfBlock(),
		function,
		version,
		varMap,
		ifBlockExit,
		contPoints,
		breakPoints,
		returnPoints
	);
	
	// Compile the else block
	llvm::BasicBlock* pElseBlock = JITCompiler::compStmtSeq(
		pIfStmt->getElseBlock(),
		function,
		version,
		varMap,
		elseBlockExit,
		contPoints,
		breakPoints,
		returnPoints
	);

	// Create a conditional branch to select which block to execute based on the test value
	testBuilder.CreateCondBr(pBoolVal, pIfBlock, pElseBlock);

	// Build the exit point list
	BranchList exitPoints;
	if (ifBlockExit.first != NULL) 
		exitPoints.push_back(ifBlockExit);
	if (elseBlockExit.first != NULL)
		exitPoints.push_back(elseBlockExit);
	
	// If there are no exit points
	if (exitPoints.empty())
	{
		// This statement has no exit point
		exitPoint.first = NULL;
	}
	else
	{
		// Create a basic block for the exit point
		llvm::BasicBlock* pExitBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
		
		// Find the live set for the if-else statement
		LiveVarMap::const_iterator liveSetItr = version.pLiveVarInfo->liveVarMap.find(pIfStmt);
		assert (liveSetItr != version.pLiveVarInfo->liveVarMap.end());
		
		// Find the variable type map after the if-else statement
		TypeInfoMap::const_iterator typeInfoItr = version.pTypeInferInfo->postTypeMap.find(pIfStmt);
		assert (typeInfoItr != version.pTypeInferInfo->postTypeMap.end());
		
		// Match the variable mappings at the exit point
		VariableMap exitVarMap = matchBranchPoints(
			function,
			version,
			liveSetItr->second,
			typeInfoItr->second,
			exitPoints,
			pExitBlock
		);
		
		// Set the exit point parameters
		exitPoint.first = pExitBlock;
		exitPoint.second = exitVarMap;
	}
	
	// Return a pointer to the entry block
	//return pEntryBlock;
	
	return pCopyBlock;
}

/***************************************************************
* Function: JITCompiler::compLoopStmt()
* Purpose : Compile a loop statement
* Initial : Maxime Chevalier-Boisvert on March 10, 2009
****************************************************************
Revisions and bug fixes:
*/
llvm::BasicBlock* JITCompiler::compLoopStmt(
	LoopStmt* pLoopStmt,
	CompFunction& function,
	CompVersion& version,
	VariableMap varMap,
	BranchPoint& exitPoint,
	BranchList& returnPoints
)
{
	// generate code for any copies before this statement
	// Create a basic block for the statement
	llvm::BasicBlock* pCopyBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
	llvm::IRBuilder<> copyBuilder(pCopyBlock);

	// Find the type information before this statement
	TypeInfoMap::const_iterator preLoopTypeItr = version.pTypeInferInfo->preTypeMap.find(pLoopStmt);
	assert (preLoopTypeItr != version.pTypeInferInfo->preTypeMap.end());	
	
	if (s_jitCopyEnableVar)
	{
		// generate copies before the loop
		genCopyCode(copyBuilder, function, version, preLoopTypeItr->second, varMap, pLoopStmt);
	}
	
	// Get the statement sequences associated with the loop
	StmtSequence* pInitSeq = pLoopStmt->getInitSeq();
	StmtSequence* pTestSeq = pLoopStmt->getTestSeq();
	StmtSequence* pBodySeq = pLoopStmt->getBodySeq();
	StmtSequence* pIncrSeq = pLoopStmt->getIncrSeq();
	
	// Get the test variable of the loop
	SymbolExpr* pTestVar = pLoopStmt->getTestVar();
	
	/***********************************/
	/***********************************/
	
	// Declare branch point lists for the continue and break points
	BranchList contPoints;
	BranchList breakPoints;
	
	// Declare branch points for the statement sequence exits
	BranchPoint initExit;
	BranchPoint testExit;
	BranchPoint bodyExit;
	BranchPoint incrExit;
	
	/************************************
	* Initialization sequence compilation
	************************************/
	
	// Compile the initialization sequence
	llvm::BasicBlock* pInitBlock = JITCompiler::compStmtSeq(
		pInitSeq,
		function,
		version,
		varMap,
		initExit,
		contPoints,
		breakPoints,
		returnPoints
	);
	
	// Ensure the initialization block has only one exit
	assert (initExit.first != NULL && contPoints.empty() && breakPoints.empty());
	copyBuilder.CreateBr(pInitBlock);
		
	/***********************
	* Loop entry point setup
	***********************/
	
	// Create an IR builder for the loop init exit point
	llvm::IRBuilder<> initExitBuilder(initExit.first);
	
	// Create a basic block for the loop entry point
	llvm::BasicBlock* pLoopEntryBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
	llvm::IRBuilder<> loopEntryBuilder(pLoopEntryBlock);
	
	// Declare a variable map for the loop entry
	VariableMap loopEntryVarMap;
	
	// Get the live variables after the initialization sequence
	LiveVarMap::const_iterator postInitLiveItr = version.pLiveVarInfo->liveVarMap.find(pInitSeq);
	assert (postInitLiveItr != version.pLiveVarInfo->liveVarMap.end());
	const Expression::SymbolSet& postInitLiveVars = postInitLiveItr->second;

	// Get the type information before the test sequence
	TypeInfoMap::const_iterator preTestTypeItr = version.pTypeInferInfo->preTypeMap.find(pTestSeq);
	assert (preTestTypeItr != version.pTypeInferInfo->preTypeMap.end());
	const VarTypeMap& preTestVarTypes = preTestTypeItr->second;
	
	// For each variable in the post-initialization variable map
	for (VariableMap::iterator mapItr = initExit.second.begin(); mapItr != initExit.second.end(); ++mapItr)
	{
		// Get a pointer to the symbol
		SymbolExpr* pSymbol = mapItr->first;
		
		// If this variable is not live at this point, skip it
		if (postInitLiveVars.find(pSymbol) == postInitLiveVars.end())
			continue;

		// If this symbol is in the environment
		if (mapItr->second.pValue == NULL)
		{	
			// The symbol will remain in the environment at the loop entry
			loopEntryVarMap[pSymbol] = Value(NULL, DataObject::UNKNOWN);	
		}
		else
		{
			// Find the type set for this symbol
			VarTypeMap::const_iterator typeItr = preTestVarTypes.find(pSymbol);
			TypeSet typeSet = (typeItr != preTestVarTypes.end())? typeItr->second:TypeSet();
			
			// Find the appropriate storage mode for the variable
			DataObject::Type objectType;
			llvm::Type* storageMode = getStorageMode(
				typeSet,
				objectType
			);
					
			// Get the current value of the variable
			llvm::Value* pValue = mapItr->second.pValue;
						
			// If the storage mode of the variable does not match that of the phi node
			if (pValue->getType() != storageMode)
			{
				//std::cout << "Performing storage mode conversion for: " << pSymbol->toString() << std::endl;
				
				// Change the storage mode of the variable
				pValue = changeStorageMode(
					initExitBuilder,
					pValue,
					mapItr->second.objType,
					storageMode
				);
			}
			
			// Create a phi node for this variable
			llvm::PHINode* pPhiNode = loopEntryBuilder.CreatePHI(storageMode, 2);
			
			// Add an entry point for the loop initialization block
			pPhiNode->addIncoming(pValue, initExit.first);
			
			// Add the phi node to the loop entry variable map
			loopEntryVarMap[pSymbol] = Value(pPhiNode, objectType);
			
			assert (pPhiNode->getType() == VOID_PTR_TYPE || objectType != DataObject::UNKNOWN);
		}
	}
	
	// Link the init exit block to the loop entry block
	initExitBuilder.CreateBr(pLoopEntryBlock);
	
	/**************************
	* Test sequence compilation
	**************************/
	
	// Compile the test sequence
	llvm::BasicBlock* pTestBlock = JITCompiler::compStmtSeq(
		pTestSeq,
		function,
		version,
		loopEntryVarMap,
		testExit,
		contPoints,
		breakPoints,
		returnPoints
	);
	
	// Ensure the test block has only one exit
	assert (testExit.first != NULL && contPoints.empty() && breakPoints.empty());
	
	/***********************************
	* Loop decision point implementation 
	***********************************/
		
	// Get the variable map at the decision point
	VariableMap decVarMap = testExit.second;
	
	// Create a basic block to implement the loop decision point
	llvm::BasicBlock* pDecBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
	llvm::IRBuilder<> decBuilder(pDecBlock);
	
	// Create an IR builder for the test exit point
	llvm::IRBuilder<> postTestBuilder(testExit.first);
	
	// Link the test exit block to the decision point block
	postTestBuilder.CreateBr(pDecBlock);
	
	// Get the type information after the test sequence
	TypeInfoMap::const_iterator postTestTypeItr = version.pTypeInferInfo->postTypeMap.find(pTestSeq);
	assert (postTestTypeItr != version.pTypeInferInfo->postTypeMap.end());
	const VarTypeMap& postTestVarTypes = postTestTypeItr->second;
	
	// Read the loop test variable, if needed
	Expression::SymbolSet testVarSet;
	testVarSet.insert(pTestVar);
	readVariables(decBuilder, function, version, postTestVarTypes, decVarMap, testVarSet);
	
	// Get the value of the loop test variable
	Value testValue = decVarMap[pTestVar];
		
	// Get the boolean value of the test value
	llvm::Value* pBoolVal = changeStorageMode(
		decBuilder,
		testValue.pValue,
		testValue.objType,
		llvm::Type::getInt1Ty(*s_Context)
	);
	
	// Add the decision point block to the break points
	breakPoints.push_back(BranchPoint(pDecBlock, decVarMap));
	
	
	/*******************************
	* Loop body sequence compilation 
	*******************************/
	
	// Compile the body sequence
	llvm::BasicBlock* pBodyBlock = JITCompiler::compStmtSeq(
		pBodySeq,
		function,
		version,
		decVarMap,
		bodyExit,
		contPoints,
		breakPoints,
		returnPoints
	);	
	
	// Add the body exit to the continue points, if specified
	if (bodyExit.first != NULL)
		contPoints.push_back(bodyExit);
	
	// Create a basic block to go after the loop body
	llvm::BasicBlock* pAfterBodyBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
	llvm::IRBuilder<> afterBodyBuilder(pAfterBodyBlock);
	
	// Find the live variable set after the body
	LiveVarMap::const_iterator afterBodyLiveSet = version.pLiveVarInfo->liveVarMap.find(pBodySeq);
	assert (afterBodyLiveSet != version.pLiveVarInfo->liveVarMap.end());
	
	// Find the variable type map after the body
	TypeInfoMap::const_iterator afterBodyTypeMap = version.pTypeInferInfo->postTypeMap.find(pBodySeq);
	assert (afterBodyTypeMap != version.pTypeInferInfo->postTypeMap.end());
	
	// Match the variable mappings after the loop body
	VariableMap afterBodyVarMap = matchBranchPoints(
		function,
		version,
		afterBodyLiveSet->second,
		afterBodyTypeMap->second,
		contPoints,
		pAfterBodyBlock
	);
	
	/************************************
	* Incrementation sequence compilation
	************************************/	

	// Compile the incrementation sequence
	llvm::BasicBlock* pIncrBlock = JITCompiler::compStmtSeq(
		pIncrSeq,
		function,
		version,
		afterBodyVarMap,
		incrExit,
		contPoints,
		breakPoints,
		returnPoints
	);
	
	// Ensure the incrementation sequence has a proper exit point
	assert (incrExit.first != NULL);
	
	/****************************************
	* Entry vs exit variable mapping matching
	****************************************/
	
	// Make the after body block jump to the incrementation sequence block
	afterBodyBuilder.CreateBr(pIncrBlock);
	
	// Create an IR builder for the post-incrementation block
	llvm::IRBuilder<> postIncrBuilder(incrExit.first);
	
	// Get the type information after the incrementation sequence
	TypeInfoMap::const_iterator postIncrTypeItr = version.pTypeInferInfo->postTypeMap.find(pIncrSeq);
	assert (postIncrTypeItr != version.pTypeInferInfo->postTypeMap.end());
	const VarTypeMap& postIncrVarTypes = postIncrTypeItr->second;
	
	// For each variable in the post-incrementation variable map
	for (VariableMap::iterator mapItr = incrExit.second.begin(); mapItr != incrExit.second.end(); ++mapItr)
	{
		// Get the symbol for this variable
		SymbolExpr* pSymbol = mapItr->first;
		
		// Lookup the variable in the post-init variable map
		VariableMap::iterator postInitItr = initExit.second.find(pSymbol);
		
		// Determine if the variable is local in the post-init variable map
		bool postInitLocal = postInitItr != initExit.second.end() && postInitItr->second.pValue != NULL;
		
		// Determine if the variable is local in the post-incr variable map
		bool postIncrLocal = mapItr->second.pValue != NULL;
		
		// If the states do not match
		if (postInitLocal != postIncrLocal)
		{	
			// If the variable is local
			if (postIncrLocal)
			{
				// Write the variable to the environment
				writeVariable(postIncrBuilder, function, version, pSymbol, mapItr->second);
			}
			else
			{
				// Find the type set for this variable
				VarTypeMap::const_iterator typeItr = postIncrVarTypes.find(pSymbol);
				assert (typeItr != postIncrVarTypes.end());
				
				// Read the variable from the environment
				mapItr->second = readVariable(postIncrBuilder, function, version, typeItr->second, pSymbol, false);
			}
		}
		
		// If the variable is local at the post-init point
		if (postInitLocal)
		{
			// Get the phi node for this variable
			llvm::PHINode* pPhiNode = (llvm::PHINode*)loopEntryVarMap[pSymbol].pValue;
			
			// Get the value of the variable
			llvm::Value* pValue = mapItr->second.pValue;
			
			// If the storage mode does not match that of the phi node
			if (pValue->getType() != pPhiNode->getType())
			{
				// Change the storage mode to match the phi node
				pValue = changeStorageMode(
					postIncrBuilder,
					pValue,
					mapItr->second.objType,
					pPhiNode->getType()
				);
			}
			
			// Add an incoming value to the phi node for this variable
			pPhiNode->addIncoming(pValue, incrExit.first);
		}
	}
	
	//std::cout << "Done matching var mapping" << std::endl;
	
	// Link the incrementation exit point to the loop entry block
	postIncrBuilder.CreateBr(pLoopEntryBlock);
	
	// Link the loop entry block to the loop test block
	loopEntryBuilder.CreateBr(pTestBlock);

	
	/*************************
	* Final loop break linkage
	*************************/
	
	// Create a basic block for the loop break point
	llvm::BasicBlock* pBreakBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
	llvm::IRBuilder<> breakBuilder(pBreakBlock);
	
	// Find the live variable set after the loop statement
	LiveVarMap::const_iterator afterLoopLiveSet = version.pLiveVarInfo->liveVarMap.find(pLoopStmt);
	assert (afterLoopLiveSet != version.pLiveVarInfo->liveVarMap.end());
	
	// Find the variable type map after the loop statement
	TypeInfoMap::const_iterator afterLoopTypeMap = version.pTypeInferInfo->postTypeMap.find(pLoopStmt);
	assert (afterLoopTypeMap != version.pTypeInferInfo->postTypeMap.end());
	
	// Match the variable mappings at the break point
	VariableMap breakVarMap = matchBranchPoints(
		function,
		version,
		afterLoopLiveSet->second,
		afterLoopTypeMap->second,
		breakPoints,
		pBreakBlock
	);
	
	// Erase the final break instruction from the decision block
	pDecBlock->getInstList().erase(--pDecBlock->end());
	
	// Create a conditional branch to select which block to execute based on the loop test value
	decBuilder.CreateCondBr(pBoolVal, pBodyBlock, pBreakBlock);
		
	// Set the exit point parameters
	exitPoint.first = pBreakBlock;
	exitPoint.second = breakVarMap;
	
	// Return a pointer to the loop initialization block
	// return pInitBlock;
	return pCopyBlock;
}

/***************************************************************
* Function: JITCompiler::compExpression()
* Purpose : Compile an expression
* Initial : Maxime Chevalier-Boisvert on March 22, 2009
****************************************************************
Revisions and bug fixes:
*/
JITCompiler::Value JITCompiler::compExpression(
	Expression* pExpression,
	CompFunction& function,
	CompVersion& version,
	const Expression::SymbolSet& liveVars,
	const VarDefMap& reachDefs,
	const VarTypeMap& varTypes,
	VariableMap& varMap,
	llvm::BasicBlock* pEntryBlock,
	llvm::BasicBlock* pExitBlock
)
{
	// Switch on the expression type
	switch (pExpression->getExprType())
	{
		// Symbol expression
		case Expression::SYMBOL:
		{
			// Get a typed pointer to the symbol expression
			SymbolExpr* pSymbolExpr = (SymbolExpr*)pExpression;
			
			// Compile the symbol expression
			ValueVector values = compSymbolExpr(
				pSymbolExpr,
				1,
				function,
				version,
				liveVars,
				reachDefs,
				varTypes,
				varMap,
				pEntryBlock,
				pExitBlock
			);
			
			// Return the first value
			return values.front();
		}
		break;
		
		// Integer constant expression
		case Expression::INT_CONST:
		{
			// Get a typed pointer to the expression
			IntConstExpr* pIntExpr = (IntConstExpr*)pExpression;
			
			// Create an integer value for the constant
			llvm::Value* pConstValue = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*s_Context), pIntExpr->getValue());
			
			// Create an IR builder for the entry block
			llvm::IRBuilder<> entryBuilder(pEntryBlock);
			entryBuilder.CreateBr(pExitBlock);
			
			// Return a value object
			return Value(pConstValue, DataObject::MATRIX_F64);
		}
		break;
		
		// Floating-point constant expression
		case Expression::FP_CONST:
		{
			// Get a typed pointer to the expression
			FPConstExpr* pFPExpr = (FPConstExpr*)pExpression;
			
			// Create a floating-point value for the constant
			llvm::Value* pConstValue = llvm::ConstantFP::get(llvm::Type::getDoubleTy(*s_Context), pFPExpr->getValue());
			
			// Create an IR builder for the entry block
			llvm::IRBuilder<> entryBuilder(pEntryBlock);
			entryBuilder.CreateBr(pExitBlock);
			
			// Return a value object
			return Value(pConstValue, DataObject::MATRIX_F64);
		}
		break;

		// Unary operation expression
		case Expression::UNARY_OP:
		{
			// Compile the unary expression
			return compUnaryExpr(
				(UnaryOpExpr*)pExpression,
				function,
				version,
				liveVars,
				reachDefs,
				varTypes,
				varMap,
				pEntryBlock,
				pExitBlock
			);
		}
		break;
		
		// Binary operation expression
		case Expression::BINARY_OP:
		{
			// Compile the binary expression
			return compBinaryExpr(
				(BinaryOpExpr*)pExpression,
				function,
				version,
				liveVars,
				reachDefs,
				varTypes,
				varMap,
				pEntryBlock,
				pExitBlock
			);
		}
		break;
		
		
		// Parameterized expression
		case Expression::PARAM:
		{
			// Get a typed pointer to the parameterized expression
			ParamExpr* pParamExpr = (ParamExpr*)pExpression;
			
			// Compile the parameterized expression
			ValueVector values = compParamExpr(
				pParamExpr,
				1,
				function,
				version,
				liveVars,
				reachDefs,
				varTypes,
				varMap,
				pEntryBlock,
				pExitBlock
			);
			
			// Return the first value
			return values.front();
		}
		break;
	
		// Any other expression type
		default:
		{
			// Generate interpreter fallback code
			return exprFallback(
				pExpression,
				(void*)Interpreter::evalExpression,
				function,
				version,
				liveVars,
				reachDefs,
				varTypes,
				varMap,
				pEntryBlock,
				pExitBlock
			);
		}
	}
}

/***************************************************************
* Function: JITCompiler::compUnaryExpr()
* Purpose : Compile a unary expression
* Initial : Maxime Chevalier-Boisvert on June 12, 2009
****************************************************************
Revisions and bug fixes:
*/
JITCompiler::Value JITCompiler::compUnaryExpr(
	UnaryOpExpr* pUnaryExpr,
	CompFunction& function,
	CompVersion& version,
	const Expression::SymbolSet& liveVars,
	const VarDefMap& reachDefs,
	const VarTypeMap& varTypes,
	VariableMap& varMap,
	llvm::BasicBlock* pEntryBlock,
	llvm::BasicBlock* pExitBlock
)
{
	// Switch on the binary operator type
	switch (pUnaryExpr->getOperator())
	{
		/*
		// Unary plus operator
		case UnaryOpExpr::PLUS:
		{	
		}
		break;
		*/

		// Unary negation operator
		case UnaryOpExpr::MINUS:
		{	
			// Compile this as a multiplication of the value with -1
			return JITCompiler::compBinaryExpr(
				new BinaryOpExpr(BinaryOpExpr::MULT,
					pUnaryExpr->getOperand(),
					new IntConstExpr(-1)
				),
				function,
				version,
				liveVars,
				reachDefs,
				varTypes,
				varMap,
				pEntryBlock,
				pExitBlock
			);
		}
		break;

		// All other operator types
		default:
		{
			// Generate interpreter fallback code
			return exprFallback(
				pUnaryExpr,
				(void*)Interpreter::evalUnaryExpr,
				function,
				version,
				liveVars,
				reachDefs,
				varTypes,
				varMap,
				pEntryBlock,
				pExitBlock
			);
		}
	}	
}

/***************************************************************
* Function: JITCompiler::compBinaryExpr()
* Purpose : Compile a binary expression
* Initial : Maxime Chevalier-Boisvert on March 24, 2009
****************************************************************
Revisions and bug fixes:
*/
JITCompiler::Value JITCompiler::compBinaryExpr(
	BinaryOpExpr* pBinaryExpr,
	CompFunction& function,
	CompVersion& version,
	const Expression::SymbolSet& liveVars,
	const VarDefMap& reachDefs,
	const VarTypeMap& varTypes,
	VariableMap& varMap,
	llvm::BasicBlock* pEntryBlock,
	llvm::BasicBlock* pExitBlock
)
{
	// If optimized binary operations are disabled
	if (s_jitUseBinOpOpts.getBoolValue() == false)
	{
		// Generate interpreter fallback code
		return exprFallback(
			pBinaryExpr,
			(void*)Interpreter::evalBinaryExpr,
			function,
			version,
			liveVars,
			reachDefs,
			varTypes,
			varMap,
			pEntryBlock,
			pExitBlock
		);
	}
	
	// Switch on the binary operator type
	switch (pBinaryExpr->getOperator())
	{
		// Addition operator
		case BinaryOpExpr::PLUS:
		{			
			// Generate code for the operation
			return compBinaryOp(
				pBinaryExpr->getLeftExpr(),
				pBinaryExpr->getRightExpr(),
				NULL,
				createAddInstr,
				createAddInstr,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				(void*)(MatrixF64Obj::F64_SCALAR_BINOP_FUNC)MatrixF64Obj::lhsScalarArrayOp<AddOp<float64>, float64, float64>,
				(void*)(MatrixF64Obj::F64_SCALAR_BINOP_FUNC)MatrixF64Obj::lhsScalarArrayOp<AddOp<float64>, float64, float64>,
				(void*)(MatrixC128Obj::F64_SCALAR_BINOP_FUNC)MatrixC128Obj::lhsScalarArrayOp<AddOp<Complex128>, Complex128, float64>,
				(void*)(MatrixC128Obj::F64_SCALAR_BINOP_FUNC)MatrixC128Obj::lhsScalarArrayOp<AddOp<Complex128>, Complex128, float64>,
				(void*)(SCALAR_BINOP_FUNC)lhsScalarArithOp<AddOp, float64>,
				(void*)(SCALAR_BINOP_FUNC)lhsScalarArithOp<AddOp, float64>,
				NULL,
				NULL,
				(void*)(MatrixF64Obj::MATRIX_BINOP_FUNC)MatrixF64Obj::binArrayOp<AddOp<float64>, float64>,
				(void*)(MatrixC128Obj::MATRIX_BINOP_FUNC)MatrixC128Obj::binArrayOp<AddOp<Complex128>, Complex128>,
				(void*)(MATRIX_BINOP_FUNC)arrayArithOp<AddOp>,
				false,
				function,
				version,
				liveVars,
				reachDefs,
				varTypes,
				varMap,
				pEntryBlock,
				pExitBlock
			);
		}
		break;
	
		// Subtraction operator
		case BinaryOpExpr::MINUS:
		{			
			// Generate code for the operation
			return compBinaryOp(
				pBinaryExpr->getLeftExpr(),
				pBinaryExpr->getRightExpr(),
				NULL,
				createSubInstr,
				createSubInstr,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				(void*)(MatrixF64Obj::F64_SCALAR_BINOP_FUNC)MatrixF64Obj::lhsScalarArrayOp<SubOp<float64>, float64, float64>,
				(void*)(MatrixF64Obj::F64_SCALAR_BINOP_FUNC)MatrixF64Obj::rhsScalarArrayOp<SubOp<float64>, float64, float64>,
				(void*)(MatrixC128Obj::F64_SCALAR_BINOP_FUNC)MatrixC128Obj::lhsScalarArrayOp<SubOp<Complex128>, Complex128, float64>,
				(void*)(MatrixC128Obj::F64_SCALAR_BINOP_FUNC)MatrixC128Obj::rhsScalarArrayOp<SubOp<Complex128>, Complex128, float64>,
				(void*)(SCALAR_BINOP_FUNC)lhsScalarArithOp<SubOp, float64>,
				(void*)(SCALAR_BINOP_FUNC)rhsScalarArithOp<SubOp, float64>,
				NULL,
				NULL,
				(void*)(MatrixF64Obj::MATRIX_BINOP_FUNC)MatrixF64Obj::binArrayOp<SubOp<float64>, float64>,
				(void*)(MatrixC128Obj::MATRIX_BINOP_FUNC)MatrixC128Obj::binArrayOp<SubOp<Complex128>, Complex128>,
				(void*)(MATRIX_BINOP_FUNC)arrayArithOp<SubOp>,
				false,
				function,
				version,
				liveVars,
				reachDefs,
				varTypes,
				varMap,
				pEntryBlock,
				pExitBlock
			);
		}
		break;
		
		// Multiplication operator
		case BinaryOpExpr::MULT:
		{
			// Generate code for the operation
			return compBinaryOp(
				pBinaryExpr->getLeftExpr(),
				pBinaryExpr->getRightExpr(),
				NULL,
				createMulInstr,
				createMulInstr,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				(void*)(MatrixF64Obj::F64_SCALAR_BINOP_FUNC)MatrixF64Obj::scalarMult<float64>,
				(void*)(MatrixF64Obj::F64_SCALAR_BINOP_FUNC)MatrixF64Obj::scalarMult<float64>,
				(void*)(MatrixC128Obj::F64_SCALAR_BINOP_FUNC)MatrixC128Obj::scalarMult<float64>,
				(void*)(MatrixC128Obj::F64_SCALAR_BINOP_FUNC)MatrixC128Obj::scalarMult<float64>,
				(void*)(SCALAR_BINOP_FUNC)scalarMultOp,
				(void*)(SCALAR_BINOP_FUNC)scalarMultOp,
				NULL,
				NULL,
				(void*)(MATRIX_BINOP_FUNC)matrixMultOp, // TODO: specialize for f64 and c128 matrices?
				(void*)(MATRIX_BINOP_FUNC)matrixMultOp, //
				(void*)(MATRIX_BINOP_FUNC)matrixMultOp,
				false,
				function,
				version,
				liveVars,
				reachDefs,
				varTypes,
				varMap,
				pEntryBlock,
				pExitBlock
			);
		}
		break;

		// Array multiplication operator
		case BinaryOpExpr::ARRAY_MULT:
		{
			// Generate code for the operation
			return compBinaryOp(
				pBinaryExpr->getLeftExpr(),
				pBinaryExpr->getRightExpr(),
				NULL,
				createMulInstr,
				createMulInstr,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				(void*)(MatrixF64Obj::F64_SCALAR_BINOP_FUNC)MatrixF64Obj::scalarMult<float64>,
				(void*)(MatrixF64Obj::F64_SCALAR_BINOP_FUNC)MatrixF64Obj::scalarMult<float64>,
				(void*)(MatrixC128Obj::F64_SCALAR_BINOP_FUNC)MatrixC128Obj::scalarMult<float64>,
				(void*)(MatrixC128Obj::F64_SCALAR_BINOP_FUNC)MatrixC128Obj::scalarMult<float64>,
				(void*)(SCALAR_BINOP_FUNC)scalarMultOp,
				(void*)(SCALAR_BINOP_FUNC)scalarMultOp,
				NULL,
				NULL,
				(void*)(MatrixF64Obj::MATRIX_BINOP_FUNC)MatrixF64Obj::binArrayOp<MultOp<float64>, float64>,
				(void*)(MatrixC128Obj::MATRIX_BINOP_FUNC)MatrixC128Obj::binArrayOp<MultOp<Complex128>, Complex128>,
				(void*)(MATRIX_BINOP_FUNC)arrayArithOp<MultOp>,
				false,
				function,
				version,
				liveVars,
				reachDefs,
				varTypes,
				varMap,
				pEntryBlock,
				pExitBlock
			);			
		}
		break;

		// Division operator
		case BinaryOpExpr::DIV:
		{
			// Generate code for the operation
			return compBinaryOp(
				pBinaryExpr->getLeftExpr(),
				pBinaryExpr->getRightExpr(),
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				(void*)DivOp<float64>::op,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				(void*)(MatrixF64Obj::F64_SCALAR_BINOP_FUNC)MatrixF64Obj::rhsScalarArrayOp<DivOp<float64>, float64, float64>,
				NULL,
				(void*)(MatrixC128Obj::F64_SCALAR_BINOP_FUNC)MatrixC128Obj::rhsScalarArrayOp<DivOp<Complex128>, Complex128, float64>,
				NULL,
				(void*)(SCALAR_BINOP_FUNC)rhsScalarArithOp<DivOp, float64>,
				NULL,
				NULL,
				(void*)(MatrixF64Obj::MATRIX_BINOP_FUNC)MatrixF64Obj::matrixRightDiv,
				(void*)(MatrixC128Obj::MATRIX_BINOP_FUNC)MatrixC128Obj::matrixRightDiv,
				(void*)(MATRIX_BINOP_FUNC)matrixRightDivOp,
				false,
				function,
				version,
				liveVars,
				reachDefs,
				varTypes,
				varMap,
				pEntryBlock,
				pExitBlock
			);
		}
		break;
		
		// Array division operator
		case BinaryOpExpr::ARRAY_DIV:
		{
			// Generate code for the operation
			return compBinaryOp(
				pBinaryExpr->getLeftExpr(),
				pBinaryExpr->getRightExpr(),
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				(void*)DivOp<float64>::op,
				NULL,
				NULL,
				NULL,
				NULL,
				(void*)(MatrixF64Obj::F64_SCALAR_BINOP_FUNC)MatrixF64Obj::lhsScalarArrayOp<DivOp<float64>, float64, float64>,
				(void*)(MatrixF64Obj::F64_SCALAR_BINOP_FUNC)MatrixF64Obj::rhsScalarArrayOp<DivOp<float64>, float64, float64>,
				(void*)(MatrixC128Obj::F64_SCALAR_BINOP_FUNC)MatrixC128Obj::lhsScalarArrayOp<DivOp<Complex128>, Complex128, float64>,
				(void*)(MatrixC128Obj::F64_SCALAR_BINOP_FUNC)MatrixC128Obj::rhsScalarArrayOp<DivOp<Complex128>, Complex128, float64>,
				(void*)(SCALAR_BINOP_FUNC)lhsScalarArithOp<DivOp, float64>,
				(void*)(SCALAR_BINOP_FUNC)rhsScalarArithOp<DivOp, float64>,
				NULL,
				NULL,
				(void*)(MatrixF64Obj::MATRIX_BINOP_FUNC)MatrixF64Obj::binArrayOp<DivOp<float64>, float64>,
				(void*)(MatrixC128Obj::MATRIX_BINOP_FUNC)MatrixC128Obj::binArrayOp<DivOp<Complex128>, Complex128>,
				(void*)(MATRIX_BINOP_FUNC)arrayArithOp<DivOp>,
				false,
				function,
				version,
				liveVars,
				reachDefs,
				varTypes,
				varMap,
				pEntryBlock,
				pExitBlock
			);		
		}
		break;
		
		// Array-OR logical operator
		case BinaryOpExpr::ARRAY_OR:
		{
			// Generate code for the operation
			return compBinaryOp(
				pBinaryExpr->getLeftExpr(),
				pBinaryExpr->getRightExpr(),
				createOrInstr,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				(void*)(LogicalArrayObj::F64_SCALAR_LOGIC_OP_FUNC)LogicalArrayObj::lhsScalarArrayOp<OrOp<bool>, bool, float64>,
				(void*)(LogicalArrayObj::F64_SCALAR_LOGIC_OP_FUNC)LogicalArrayObj::lhsScalarArrayOp<OrOp<bool>, bool, float64>,
				(void*)(CharArrayObj::F64_SCALAR_LOGIC_OP_FUNC)CharArrayObj::lhsScalarArrayOp<OrOp<char>, bool, float64>,
				(void*)(CharArrayObj::F64_SCALAR_LOGIC_OP_FUNC)CharArrayObj::lhsScalarArrayOp<OrOp<char>, bool, float64>,
				(void*)(MatrixF64Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixF64Obj::lhsScalarArrayOp<OrOp<float64>, bool, float64>,
				(void*)(MatrixF64Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixF64Obj::lhsScalarArrayOp<OrOp<float64>, bool, float64>,
				(void*)(MatrixC128Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixC128Obj::lhsScalarArrayOp<OrOp<Complex128>, bool, float64>,
				(void*)(MatrixC128Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixC128Obj::lhsScalarArrayOp<OrOp<Complex128>, bool, float64>,
				(void*)(SCALAR_BINOP_FUNC)lhsScalarLogicOp<OrOp, float64>,
				(void*)(SCALAR_BINOP_FUNC)lhsScalarLogicOp<OrOp, float64>,
				(void*)(LogicalArrayObj::MATRIX_LOGIC_OP_FUNC)LogicalArrayObj::binArrayOp<OrOp<bool>, bool>, 
				(void*)(CharArrayObj::MATRIX_LOGIC_OP_FUNC)CharArrayObj::binArrayOp<OrOp<char>, bool>,
				(void*)(MatrixF64Obj::MATRIX_LOGIC_OP_FUNC)MatrixF64Obj::binArrayOp<OrOp<float64>, bool>,
				(void*)(MatrixC128Obj::MATRIX_LOGIC_OP_FUNC)MatrixC128Obj::binArrayOp<OrOp<Complex128>, bool>,
				(void*)(MATRIX_BINOP_FUNC)matrixLogicOp<OrOp>,
				true,
				function,
				version,
				liveVars,
				reachDefs,
				varTypes,
				varMap,
				pEntryBlock,
				pExitBlock
			);
		}
		break;
		
		// Array-AND logical operator
		case BinaryOpExpr::ARRAY_AND:
		{
			// Generate code for the operation
			return compBinaryOp(
				pBinaryExpr->getLeftExpr(),
				pBinaryExpr->getRightExpr(),
				createAndInstr,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				(void*)(LogicalArrayObj::F64_SCALAR_LOGIC_OP_FUNC)LogicalArrayObj::lhsScalarArrayOp<AndOp<bool>, bool, float64>,
				(void*)(LogicalArrayObj::F64_SCALAR_LOGIC_OP_FUNC)LogicalArrayObj::lhsScalarArrayOp<AndOp<bool>, bool, float64>,
				(void*)(CharArrayObj::F64_SCALAR_LOGIC_OP_FUNC)CharArrayObj::lhsScalarArrayOp<AndOp<char>, bool, float64>,
				(void*)(CharArrayObj::F64_SCALAR_LOGIC_OP_FUNC)CharArrayObj::lhsScalarArrayOp<AndOp<char>, bool, float64>,
				(void*)(MatrixF64Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixF64Obj::lhsScalarArrayOp<AndOp<float64>, bool, float64>,
				(void*)(MatrixF64Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixF64Obj::lhsScalarArrayOp<AndOp<float64>, bool, float64>,
				(void*)(MatrixC128Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixC128Obj::lhsScalarArrayOp<AndOp<Complex128>, bool, float64>,
				(void*)(MatrixC128Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixC128Obj::lhsScalarArrayOp<AndOp<Complex128>, bool, float64>,
				(void*)(SCALAR_BINOP_FUNC)lhsScalarLogicOp<AndOp, float64>,
				(void*)(SCALAR_BINOP_FUNC)lhsScalarLogicOp<AndOp, float64>,
				(void*)(LogicalArrayObj::MATRIX_LOGIC_OP_FUNC)LogicalArrayObj::binArrayOp<AndOp<bool>, bool>, 
				(void*)(CharArrayObj::MATRIX_LOGIC_OP_FUNC)CharArrayObj::binArrayOp<AndOp<char>, bool>,
				(void*)(MatrixF64Obj::MATRIX_LOGIC_OP_FUNC)MatrixF64Obj::binArrayOp<AndOp<float64>, bool>,
				(void*)(MatrixC128Obj::MATRIX_LOGIC_OP_FUNC)MatrixC128Obj::binArrayOp<AndOp<Complex128>, bool>,
				(void*)(MATRIX_BINOP_FUNC)matrixLogicOp<AndOp>,
				true,
				function,
				version,
				liveVars,
				reachDefs,
				varTypes,
				varMap,
				pEntryBlock,
				pExitBlock
			);
		}
		break;
			
		// Equality comparison operator
		case BinaryOpExpr::EQUAL:
		{
			// Generate code for the operation
			return compBinaryOp(
				pBinaryExpr->getLeftExpr(),
				pBinaryExpr->getRightExpr(),
				createICmpEQInstr,
				createICmpEQInstr,
				createFCmpOEQInstr,
				NULL,
				NULL,
				NULL,
				(void*)(LogicalArrayObj::F64_SCALAR_LOGIC_OP_FUNC)LogicalArrayObj::lhsScalarArrayOp<EqualOp<bool>, bool, float64>,
				(void*)(LogicalArrayObj::F64_SCALAR_LOGIC_OP_FUNC)LogicalArrayObj::lhsScalarArrayOp<EqualOp<bool>, bool, float64>,
				(void*)(CharArrayObj::F64_SCALAR_LOGIC_OP_FUNC)CharArrayObj::lhsScalarArrayOp<EqualOp<char>, bool, float64>,
				(void*)(CharArrayObj::F64_SCALAR_LOGIC_OP_FUNC)CharArrayObj::lhsScalarArrayOp<EqualOp<char>, bool, float64>,
				(void*)(MatrixF64Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixF64Obj::lhsScalarArrayOp<EqualOp<float64>, bool, float64>,
				(void*)(MatrixF64Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixF64Obj::lhsScalarArrayOp<EqualOp<float64>, bool, float64>,
				(void*)(MatrixC128Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixC128Obj::lhsScalarArrayOp<EqualOp<Complex128>, bool, float64>,
				(void*)(MatrixC128Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixC128Obj::lhsScalarArrayOp<EqualOp<Complex128>, bool, float64>,
				(void*)(SCALAR_BINOP_FUNC)lhsScalarLogicOp<EqualOp, float64>,
				(void*)(SCALAR_BINOP_FUNC)lhsScalarLogicOp<EqualOp, float64>,
				(void*)(LogicalArrayObj::MATRIX_LOGIC_OP_FUNC)LogicalArrayObj::binArrayOp<EqualOp<bool>, bool>, 
				(void*)(CharArrayObj::MATRIX_LOGIC_OP_FUNC)CharArrayObj::binArrayOp<EqualOp<char>, bool>,
				(void*)(MatrixF64Obj::MATRIX_LOGIC_OP_FUNC)MatrixF64Obj::binArrayOp<EqualOp<float64>, bool>,
				(void*)(MatrixC128Obj::MATRIX_LOGIC_OP_FUNC)MatrixC128Obj::binArrayOp<EqualOp<Complex128>, bool>,
				(void*)(MATRIX_BINOP_FUNC)matrixLogicOp<EqualOp>,
				true,
				function,
				version,
				liveVars,
				reachDefs,
				varTypes,
				varMap,
				pEntryBlock,
				pExitBlock
			);
		}
		break;

		// Greater-than comparison operator
		case BinaryOpExpr::GREATER_THAN:
		{
			// Generate code for the operation
			return compBinaryOp(
				pBinaryExpr->getLeftExpr(),
				pBinaryExpr->getRightExpr(),
				NULL,
				createICmpSGTInstr,
				createFCmpOGTInstr,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				(void*)(CharArrayObj::F64_SCALAR_LOGIC_OP_FUNC)CharArrayObj::lhsScalarArrayOp<GreaterThanOp<char>, bool, float64>,
				(void*)(CharArrayObj::F64_SCALAR_LOGIC_OP_FUNC)CharArrayObj::lhsScalarArrayOp<LessThanOp<char>, bool, float64>,
				(void*)(MatrixF64Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixF64Obj::lhsScalarArrayOp<GreaterThanOp<float64>, bool, float64>,
				(void*)(MatrixF64Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixF64Obj::lhsScalarArrayOp<LessThanOp<float64>, bool, float64>,
				(void*)(MatrixC128Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixC128Obj::lhsScalarArrayOp<GreaterThanOp<Complex128>, bool, float64>,
				(void*)(MatrixC128Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixC128Obj::lhsScalarArrayOp<LessThanOp<Complex128>, bool, float64>,
				(void*)(SCALAR_BINOP_FUNC)lhsScalarLogicOp<GreaterThanOp, float64>,
				(void*)(SCALAR_BINOP_FUNC)lhsScalarLogicOp<LessThanOp, float64>,
				NULL, 
				(void*)(CharArrayObj::MATRIX_LOGIC_OP_FUNC)CharArrayObj::binArrayOp<GreaterThanOp<char>, bool>,
				(void*)(MatrixF64Obj::MATRIX_LOGIC_OP_FUNC)MatrixF64Obj::binArrayOp<GreaterThanOp<float64>, bool>,
				(void*)(MatrixC128Obj::MATRIX_LOGIC_OP_FUNC)MatrixC128Obj::binArrayOp<GreaterThanOp<Complex128>, bool>,
				(void*)(MATRIX_BINOP_FUNC)matrixLogicOp<GreaterThanOp>,
				true,
				function,
				version,
				liveVars,
				reachDefs,
				varTypes,
				varMap,
				pEntryBlock,
				pExitBlock
			);
		}
		break;
		
		// Less-than comparison operator
		case BinaryOpExpr::LESS_THAN:
		{
			// Generate code for the operation
			return compBinaryOp(
				pBinaryExpr->getLeftExpr(),
				pBinaryExpr->getRightExpr(),
				NULL,
				createICmpSLTInstr,
				createFCmpOLTInstr,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				(void*)(CharArrayObj::F64_SCALAR_LOGIC_OP_FUNC)CharArrayObj::lhsScalarArrayOp<LessThanOp<char>, bool, float64>,
				(void*)(CharArrayObj::F64_SCALAR_LOGIC_OP_FUNC)CharArrayObj::lhsScalarArrayOp<GreaterThanOp<char>, bool, float64>,
				(void*)(MatrixF64Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixF64Obj::lhsScalarArrayOp<LessThanOp<float64>, bool, float64>,
				(void*)(MatrixF64Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixF64Obj::lhsScalarArrayOp<GreaterThanOp<float64>, bool, float64>,
				(void*)(MatrixC128Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixC128Obj::lhsScalarArrayOp<LessThanOp<Complex128>, bool, float64>,
				(void*)(MatrixC128Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixC128Obj::lhsScalarArrayOp<GreaterThanOp<Complex128>, bool, float64>,
				(void*)(SCALAR_BINOP_FUNC)lhsScalarLogicOp<LessThanOp, float64>,
				(void*)(SCALAR_BINOP_FUNC)lhsScalarLogicOp<GreaterThanOp, float64>,
				NULL, 
				(void*)(CharArrayObj::MATRIX_LOGIC_OP_FUNC)CharArrayObj::binArrayOp<LessThanOp<char>, bool>,
				(void*)(MatrixF64Obj::MATRIX_LOGIC_OP_FUNC)MatrixF64Obj::binArrayOp<LessThanOp<float64>, bool>,
				(void*)(MatrixC128Obj::MATRIX_LOGIC_OP_FUNC)MatrixC128Obj::binArrayOp<LessThanOp<Complex128>, bool>,
				(void*)(MATRIX_BINOP_FUNC)matrixLogicOp<LessThanOp>,
				true,
				function,
				version,
				liveVars,
				reachDefs,
				varTypes,
				varMap,
				pEntryBlock,
				pExitBlock
			);
		}
		break;
		
		// Greater-than or equal comparison operator
		case BinaryOpExpr::GREATER_THAN_EQ:
		{
			// Generate code for the operation
			return compBinaryOp(
				pBinaryExpr->getLeftExpr(),
				pBinaryExpr->getRightExpr(),
				NULL,
				createICmpSGEInstr,
				createFCmpOGEInstr,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				(void*)(CharArrayObj::F64_SCALAR_LOGIC_OP_FUNC)CharArrayObj::lhsScalarArrayOp<GreaterThanEqOp<char>, bool, float64>,
				(void*)(CharArrayObj::F64_SCALAR_LOGIC_OP_FUNC)CharArrayObj::lhsScalarArrayOp<LessThanEqOp<char>, bool, float64>,
				(void*)(MatrixF64Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixF64Obj::lhsScalarArrayOp<GreaterThanEqOp<float64>, bool, float64>,
				(void*)(MatrixF64Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixF64Obj::lhsScalarArrayOp<LessThanEqOp<float64>, bool, float64>,
				(void*)(MatrixC128Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixC128Obj::lhsScalarArrayOp<GreaterThanEqOp<Complex128>, bool, float64>,
				(void*)(MatrixC128Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixC128Obj::lhsScalarArrayOp<LessThanEqOp<Complex128>, bool, float64>,
				(void*)(SCALAR_BINOP_FUNC)lhsScalarLogicOp<GreaterThanEqOp, float64>,
				(void*)(SCALAR_BINOP_FUNC)lhsScalarLogicOp<LessThanEqOp, float64>,
				NULL, 
				(void*)(CharArrayObj::MATRIX_LOGIC_OP_FUNC)CharArrayObj::binArrayOp<GreaterThanEqOp<char>, bool>,
				(void*)(MatrixF64Obj::MATRIX_LOGIC_OP_FUNC)MatrixF64Obj::binArrayOp<GreaterThanEqOp<float64>, bool>,
				(void*)(MatrixC128Obj::MATRIX_LOGIC_OP_FUNC)MatrixC128Obj::binArrayOp<GreaterThanEqOp<Complex128>, bool>,
				(void*)(MATRIX_BINOP_FUNC)matrixLogicOp<GreaterThanEqOp>,
				true,
				function,
				version,
				liveVars,
				reachDefs,
				varTypes,
				varMap,
				pEntryBlock,
				pExitBlock
			);
		}
		break;
		
		// Less-than or equal comparison operator
		case BinaryOpExpr::LESS_THAN_EQ:
		{
			// Generate code for the operation
			return compBinaryOp(
				pBinaryExpr->getLeftExpr(),
				pBinaryExpr->getRightExpr(),
				NULL,
				createICmpSLEInstr,
				createFCmpOLEInstr,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				(void*)(CharArrayObj::F64_SCALAR_LOGIC_OP_FUNC)CharArrayObj::lhsScalarArrayOp<LessThanEqOp<char>, bool, float64>,
				(void*)(CharArrayObj::F64_SCALAR_LOGIC_OP_FUNC)CharArrayObj::lhsScalarArrayOp<GreaterThanEqOp<char>, bool, float64>,
				(void*)(MatrixF64Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixF64Obj::lhsScalarArrayOp<LessThanEqOp<float64>, bool, float64>,
				(void*)(MatrixF64Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixF64Obj::lhsScalarArrayOp<GreaterThanEqOp<float64>, bool, float64>,
				(void*)(MatrixC128Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixC128Obj::lhsScalarArrayOp<LessThanEqOp<Complex128>, bool, float64>,
				(void*)(MatrixC128Obj::F64_SCALAR_LOGIC_OP_FUNC)MatrixC128Obj::lhsScalarArrayOp<GreaterThanEqOp<Complex128>, bool, float64>,
				(void*)(SCALAR_BINOP_FUNC)lhsScalarLogicOp<LessThanEqOp, float64>,
				(void*)(SCALAR_BINOP_FUNC)lhsScalarLogicOp<GreaterThanEqOp, float64>,
				NULL, 
				(void*)(CharArrayObj::MATRIX_LOGIC_OP_FUNC)CharArrayObj::binArrayOp<LessThanEqOp<char>, bool>,
				(void*)(MatrixF64Obj::MATRIX_LOGIC_OP_FUNC)MatrixF64Obj::binArrayOp<LessThanEqOp<float64>, bool>,
				(void*)(MatrixC128Obj::MATRIX_LOGIC_OP_FUNC)MatrixC128Obj::binArrayOp<LessThanEqOp<Complex128>, bool>,
				(void*)(MATRIX_BINOP_FUNC)matrixLogicOp<LessThanEqOp>,
				true,
				function,
				version,
				liveVars,
				reachDefs,
				varTypes,
				varMap,
				pEntryBlock,
				pExitBlock
			);
		}
		break;
		
		// All other operator types
		default:
		{
			// Generate interpreter fallback code
			return exprFallback(
				pBinaryExpr,
				(void*)Interpreter::evalBinaryExpr,
				function,
				version,
				liveVars,
				reachDefs,
				varTypes,
				varMap,
				pEntryBlock,
				pExitBlock
			);
		}
	}
}

/***************************************************************
* Function: JITCompiler::compBinaryOp()
* Purpose : Generate code for binary expression operations
* Initial : Maxime Chevalier-Boisvert on June 1, 2009
****************************************************************
Revisions and bug fixes:
*/
JITCompiler::Value JITCompiler::compBinaryOp(
	Expression* pLeftExpr,
	Expression* pRightExpr,
	BINOP_FACTORY_FUNC p2ScalarInstrBool,
	BINOP_FACTORY_FUNC p2ScalarInstrI64,
	BINOP_FACTORY_FUNC p2ScalarInstrF64,
	void* p2ScalarFuncBool,
	void* p2ScalarFuncI64,
	void* p2ScalarFuncF64,
	void* pLScalarFuncBool,
	void* pRScalarFuncBool,
	void* pLScalarFuncChar,
	void* pRScalarFuncChar,
	void* pLScalarFuncF64,
	void* pRScalarFuncF64,
	void* pLScalarFuncC128,
	void* pRScalarFuncC128,
	void* pLScalarFuncDef,
	void* pRScalarFuncDef,
	void* p2MatrixFuncBool,
	void* p2MatrixFuncChar,
	void* p2MatrixFuncF64,
	void* p2MatrixFuncC128,
	void* p2MatrixFuncDef,
	bool boolOutput,
	CompFunction& function,
	CompVersion& version,
	const Expression::SymbolSet& liveVars,
	const VarDefMap& reachDefs,
	const VarTypeMap& varTypes,
	VariableMap& varMap,
	llvm::BasicBlock* pEntryBlock,
	llvm::BasicBlock* pExitBlock
)
{
	// If we are in verbose mode, log the binary op code generation
	if (ConfigManager::s_verboseVar)
		std::cout << "Generating code for binary op" << std::endl;
	
	// Create a basic block for the left expression exit point
	llvm::BasicBlock* pLeftExitBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
	
	// Compile the left expression to get its value
	Value leftVal = compExpression(
		pLeftExpr,
		function,
		version,
		liveVars,
		reachDefs,
		varTypes,
		varMap,
		pEntryBlock,
		pLeftExitBlock
	);

	// Create a basic block for the right expression exit point
	llvm::BasicBlock* pRightExitBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
	
	// Compile the left expression to get its value
	Value rightVal = compExpression(
		pRightExpr,
		function,
		version,
		liveVars,
		reachDefs,
		varTypes,
		varMap,
		pLeftExitBlock,
		pRightExitBlock
	);
	
	// If we are in verbose mode
	if (ConfigManager::s_verboseVar)
	{
		// Log the operand types
      std::cout << "Left val type : ";
      leftVal.pValue->getType()->dump();
      std::cout << " (" << DataObject::getTypeName(leftVal.objType) << ")" << std::endl;
      std::cout << "Right val type: ";
      rightVal.pValue->getType()->dump();
      std::cout  << " (" << DataObject::getTypeName(rightVal.objType) << ")" << std::endl;
	}
	
	// Create an IR builder for the current basic block
	llvm::IRBuilder<> irBuilder(pRightExitBlock);
	
	// If the values are stored as scalars
	if (leftVal.pValue->getType() != VOID_PTR_TYPE && rightVal.pValue->getType() != VOID_PTR_TYPE)
	{
		// By default, use the widest storage mode of the two operands
		llvm::Type* outMode = widestStorageMode(leftVal.pValue->getType(), rightVal.pValue->getType());
		
		// If the output mode is boolean but there is no boolean type, use the int64 mode instead
		if (outMode == llvm::Type::getInt1Ty(*s_Context) && !p2ScalarInstrBool && !p2ScalarFuncBool)
			outMode = llvm::Type::getInt64Ty(*s_Context);
		
		// If the output mode is integer but there is no integer path, use the float64 mode instead
		if (outMode == llvm::Type::getInt64Ty(*s_Context) && !p2ScalarInstrI64 && !p2ScalarFuncI64)
			outMode = llvm::Type::getDoubleTy(*s_Context);
		
		// If the output mode is float64 but there is no floating-point path, use the boolean mode
		if (outMode == llvm::Type::getDoubleTy(*s_Context) && !p2ScalarInstrF64 && !p2ScalarFuncF64)
			outMode = llvm::Type::getInt1Ty(*s_Context);
		
		// If we are in verbose mode, log the selected storage mode
		if (ConfigManager::s_verboseVar) {

          std::cout << "Selected storage mode for bin op: ";
          outMode->dump();
        }
		
		// Determine the output type
		DataObject::Type outType = boolOutput? DataObject::LOGICALARRAY:DataObject::MATRIX_F64;
		
		// Convert the left value to the output storage mode
		llvm::Value* pLeftVal = changeStorageMode(
			irBuilder,
			leftVal.pValue,
			leftVal.objType,
			outMode
		);

		// Convert the right value to the output storage mode
		llvm::Value* pRightVal = changeStorageMode(
			irBuilder,
			rightVal.pValue,
			rightVal.objType,
			outMode
		);
		
		// Declare an object for the output value
		Value outValue;
		
		// Store the output object type
		outValue.objType = outType;
		
		// If we are operating on boolean values
		if (outMode == llvm::Type::getInt1Ty(*s_Context))
		{
			// Ensure that a boolean computation path was specified
			assert (p2ScalarInstrBool || p2ScalarFuncBool);
			
			// If we should use a boolean instruction directly
			if (p2ScalarInstrBool)
			{
				// Insert the instruction to compute the output
				outValue.pValue = p2ScalarInstrBool(irBuilder, pLeftVal, pRightVal);
			}
			else
			{
				// Create a call to the operator function
				LLVMValueVector opArgs;
				opArgs.push_back(pLeftVal);
				opArgs.push_back(pRightVal);
				outValue.pValue = createNativeCall(
					irBuilder,
					p2ScalarFuncBool,
					opArgs
				);
			}
		}
		
		// If we are operating on integer values
		else if (outMode == llvm::Type::getInt64Ty(*s_Context))
		{
			// Ensure that an integer computation path was specified
			assert (p2ScalarInstrI64 || p2ScalarFuncI64);
			
			// If we should use a boolean instruction directly
			if (p2ScalarInstrI64)
			{
				// Insert the instruction to compute the output
				outValue.pValue = p2ScalarInstrI64(irBuilder, pLeftVal, pRightVal);
			}
			else
			{
				// Create a call to the operator function
				LLVMValueVector opArgs;
				opArgs.push_back(pLeftVal);
				opArgs.push_back(pRightVal);
				outValue.pValue = createNativeCall(
					irBuilder,
					p2ScalarFuncI64,
					opArgs
				);
			}
		}
		
		// If we are operating on floating-point values
		else if (outMode == llvm::Type::getDoubleTy(*s_Context))
		{
			// Ensure that a floating-point computation path was specified
			assert (p2ScalarInstrF64 || p2ScalarFuncF64);
			
			// If we should use a boolean instruction directly
			if (p2ScalarInstrF64)
			{
				// Insert the instruction to compute the output
				outValue.pValue = p2ScalarInstrF64(irBuilder, pLeftVal, pRightVal);
			}
			else
			{
				// Create a call to the operator function
				LLVMValueVector opArgs;
				opArgs.push_back(pLeftVal);
				opArgs.push_back(pRightVal);
				outValue.pValue = createNativeCall(
					irBuilder,
					p2ScalarFuncF64,
					opArgs
				);
			}
		}
		
		// Branch to the exit block
		irBuilder.CreateBr(pExitBlock);
		
		// Return the output value
		return outValue;
	}
	
	// If left value is stored as a scalar
	if (leftVal.pValue->getType() != VOID_PTR_TYPE)
	{
		//std::cout << "Compiling scalar lhs op" << std::endl;
		
		// If both values are logical arrays
		// NOTE: should not convert float values to booleans before operating on them
		if (leftVal.objType == DataObject::LOGICALARRAY && rightVal.objType == DataObject::LOGICALARRAY)
		{
			// If a boolean computation path is specified
			if (pLScalarFuncBool)
			{
				// Convert the left value to the f64 storage mode
				llvm::Value* pLeftVal = changeStorageMode(
					irBuilder,
					leftVal.pValue,
					leftVal.objType,
					llvm::Type::getDoubleTy(*s_Context)
				);
				
				// Create an object for the output value
				Value outValue;
				
				// Store the output object type
				outValue.objType = DataObject::LOGICALARRAY;
				
				// Create a call to the operator function
				LLVMValueVector opArgs;
				opArgs.push_back(rightVal.pValue);
				opArgs.push_back(pLeftVal);
				outValue.pValue = createNativeCall(
					irBuilder,
					pLScalarFuncBool,
					opArgs
				);
					
				// Branch to the exit block
				irBuilder.CreateBr(pExitBlock);
				
				// Return the output value
				return outValue;
			}
			else
			{
				// Convert the right value to an f64 matrix
				LLVMValueVector convArgs;
				convArgs.push_back(rightVal.pValue);
				convArgs.push_back(getObjType(DataObject::MATRIX_F64));
				rightVal.pValue = createNativeCall(
					irBuilder,
					(void*)DataObject::convertType,
					convArgs
				);
				
				// Update the right object type
				rightVal.objType = DataObject::MATRIX_F64;
			}		
		}
		
		// If the right value is a floating-point matrix and the f64 computational path was specified
		// NOTE: the left (scalar) value is always convertible to float64
		if (rightVal.objType == DataObject::MATRIX_F64 && pLScalarFuncF64)
		{			
			// Convert the left value to the f64 storage mode
			llvm::Value* pLeftVal = changeStorageMode(
				irBuilder,
				leftVal.pValue,
				leftVal.objType,
				llvm::Type::getDoubleTy(*s_Context)
			);
			
			// Create an object for the output value
			Value outValue;
			
			// Store the output object type
			outValue.objType = boolOutput? DataObject::LOGICALARRAY:DataObject::MATRIX_F64;
			
			// Create a call to the operator function
			LLVMValueVector opArgs;
			opArgs.push_back(rightVal.pValue);
			opArgs.push_back(pLeftVal);
			outValue.pValue = createNativeCall(
				irBuilder,
				pLScalarFuncF64,
				opArgs
			);
			
			// Branch to the exit block
			irBuilder.CreateBr(pExitBlock);
			
			// Return the output value
			return outValue;
		}

		// If the right value is a character array matrix and the char array computational path was specified
		if (rightVal.objType == DataObject::CHARARRAY && pLScalarFuncChar)
		{
			// Convert the left value to the f64 storage mode
			llvm::Value* pLeftVal = changeStorageMode(
				irBuilder,
				leftVal.pValue,
				leftVal.objType,
				llvm::Type::getDoubleTy(*s_Context)
			);
			
			// Create an object for the output value
			Value outValue;
			
			// Store the output object type
			outValue.objType = boolOutput? DataObject::LOGICALARRAY:DataObject::MATRIX_F64;
			
			// Create a call to the operator function
			LLVMValueVector opArgs;
			opArgs.push_back(rightVal.pValue);
			opArgs.push_back(pLeftVal);
			outValue.pValue = createNativeCall(
				irBuilder,
				pLScalarFuncChar,
				opArgs
			);
			
			// Branch to the exit block
			irBuilder.CreateBr(pExitBlock);
			
			// Return the output value
			return outValue;
		}
		
		// If the right value is a complex matrix and the complex computational path was specified
		if (rightVal.objType == DataObject::MATRIX_C128 && pLScalarFuncC128)
		{
			// Convert the left value to the f64 storage mode
			llvm::Value* pLeftVal = changeStorageMode(
				irBuilder,
				leftVal.pValue,
				leftVal.objType,
				llvm::Type::getDoubleTy(*s_Context)
			);
			
			// Create an object for the output value
			Value outValue;
			
			// Store the output object type
			outValue.objType = boolOutput? DataObject::LOGICALARRAY:DataObject::MATRIX_C128;
			
			// Create a call to the operator function
			LLVMValueVector opArgs;
			opArgs.push_back(rightVal.pValue);
			opArgs.push_back(pLeftVal);
			outValue.pValue = createNativeCall(
				irBuilder,
				pLScalarFuncC128,
				opArgs
			);
			
			// Branch to the exit block
			irBuilder.CreateBr(pExitBlock);
			
			// Return the output value
			return outValue;
		}
		
		// The right value type is unknown or other cases did not apply
		// If the default computational path was specified
		if (pLScalarFuncDef)
		{
			// Convert the left value to the f64 storage mode
			llvm::Value* pLeftVal = changeStorageMode(
				irBuilder,
				leftVal.pValue,
				leftVal.objType,
				llvm::Type::getDoubleTy(*s_Context)
			);
			
			// Create an object for the output value
			Value outValue;
			
			// Store the output object type
			outValue.objType = boolOutput? DataObject::LOGICALARRAY:DataObject::UNKNOWN;
			
			// Create a call to the operator function
			LLVMValueVector opArgs;
			opArgs.push_back(rightVal.pValue);
			opArgs.push_back(pLeftVal);
			outValue.pValue = createNativeCall(
				irBuilder,
				pLScalarFuncDef,
				opArgs
			);
			
			// Branch to the exit block
			irBuilder.CreateBr(pExitBlock);
			
			// Return the output value
			return outValue;			
		}
		else
		{
			// Convert the left value to the object pointer storage mode
			leftVal.pValue = changeStorageMode(
				irBuilder,
				leftVal.pValue,
				leftVal.objType,
				VOID_PTR_TYPE
			);
		}
	}
	
	// If the right value is stored as a scalar
	if (rightVal.pValue->getType() != VOID_PTR_TYPE)
	{
		//std::cout << "Compiling scalar rhs op" << std::endl;
		
		// If both values are logical arrays
		// NOTE: should not convert float values to booleans before operating on them
		if (leftVal.objType == DataObject::LOGICALARRAY && rightVal.objType == DataObject::LOGICALARRAY)
		{
			// If a boolean computation path is specified
			if (pRScalarFuncBool)
			{
				// Convert the right value to the boolean (int1) storage mode
				llvm::Value* pRightVal = changeStorageMode(
					irBuilder,
					rightVal.pValue,
					rightVal.objType,
					llvm::Type::getInt1Ty(*s_Context)
				);
				
				// Create an object for the output value
				Value outValue;
				
				// Store the output object type
				outValue.objType = DataObject::LOGICALARRAY;
				
				// Create a call to the operator function
				LLVMValueVector opArgs;
				opArgs.push_back(leftVal.pValue);
				opArgs.push_back(pRightVal);
				outValue.pValue = createNativeCall(
					irBuilder,
					pRScalarFuncBool,
					opArgs
				);
					
				// Branch to the exit block
				irBuilder.CreateBr(pExitBlock);
				
				// Return the output value
				return outValue;
			}
			else
			{
				// Convert the left value to an f64 matrix
				LLVMValueVector convArgs;
				convArgs.push_back(leftVal.pValue);
				convArgs.push_back(getObjType(DataObject::MATRIX_F64));
				leftVal.pValue = createNativeCall(
					irBuilder,
					(void*)DataObject::convertType,
					convArgs
				);
				
				// Update the right object type
				leftVal.objType = DataObject::MATRIX_F64;
			}		
		}
		
		// If the left value is a floating-point matrix and the f64 computational path was specified
		// NOTE: the right (scalar) value is always convertible to float64
		if (leftVal.objType == DataObject::MATRIX_F64 && pRScalarFuncF64)
		{
			//std::cout << "In R scalar F64 case..." << std::endl;
			
			// Convert the right value to the f64 storage mode
			llvm::Value* pRightVal = changeStorageMode(
				irBuilder,
				rightVal.pValue,
				rightVal.objType,
				llvm::Type::getDoubleTy(*s_Context)
			);
			
			// Create an object for the output value
			Value outValue;
			
			// Store the output object type
			outValue.objType = boolOutput? DataObject::LOGICALARRAY:DataObject::MATRIX_F64;

			// Create a call to the operator function
			LLVMValueVector opArgs;
			opArgs.push_back(leftVal.pValue);
			opArgs.push_back(pRightVal);
			outValue.pValue = createNativeCall(
				irBuilder,
				pRScalarFuncF64,
				opArgs
			);
			
			// Branch to the exit block
			irBuilder.CreateBr(pExitBlock);
			
			// Return the output value
			return outValue;
		}

		// If the left value is a char array and the char array computational path was specified
		if (leftVal.objType == DataObject::CHARARRAY && pRScalarFuncChar)
		{
			// Convert the right value to the f64 storage mode
			llvm::Value* pRightVal = changeStorageMode(
				irBuilder,
				rightVal.pValue,
				rightVal.objType,
				llvm::Type::getDoubleTy(*s_Context)
			);
			
			// Create an object for the output value
			Value outValue;
			
			// Store the output object type
			outValue.objType = boolOutput? DataObject::LOGICALARRAY:DataObject::MATRIX_F64;
			
			// Create a call to the operator function
			LLVMValueVector opArgs;
			opArgs.push_back(leftVal.pValue);
			opArgs.push_back(pRightVal);
			outValue.pValue = createNativeCall(
				irBuilder,
				pRScalarFuncChar,
				opArgs
			);
			
			// Branch to the exit block
			irBuilder.CreateBr(pExitBlock);
			
			// Return the output value
			return outValue;
		}
		
		// If the left value is a complex matrix and the complex computational path was specified
		if (leftVal.objType == DataObject::MATRIX_C128 && pRScalarFuncC128)
		{
			// Convert the right value to the f64 storage mode
			llvm::Value* pRightVal = changeStorageMode(
				irBuilder,
				rightVal.pValue,
				rightVal.objType,
				llvm::Type::getDoubleTy(*s_Context)
			);
			
			// Create an object for the output value
			Value outValue;
			
			// Store the output object type
			outValue.objType = boolOutput? DataObject::LOGICALARRAY:DataObject::MATRIX_C128;
			
			// Create a call to the operator function
			LLVMValueVector opArgs;
			opArgs.push_back(leftVal.pValue);
			opArgs.push_back(pRightVal);
			outValue.pValue = createNativeCall(
				irBuilder,
				pRScalarFuncC128,
				opArgs
			);
			
			// Branch to the exit block
			irBuilder.CreateBr(pExitBlock);
			
			// Return the output value
			return outValue;
		}
		
		// The left value type is unknown or other cases did not apply
		// If the default computational path was specified
		if (pRScalarFuncDef)
		{
			// Convert the right value to the f64 storage mode
			llvm::Value* pRightVal = changeStorageMode(
				irBuilder,
				rightVal.pValue,
				rightVal.objType,
				llvm::Type::getDoubleTy(*s_Context)
			);
			
			// Create an object for the output value
			Value outValue;		
			
			// Create a call to the operator function
			LLVMValueVector opArgs;
			opArgs.push_back(leftVal.pValue);
			opArgs.push_back(pRightVal);
			outValue.pValue = createNativeCall(
				irBuilder,
				pRScalarFuncDef,
				opArgs
			);
			
			// Branch to the exit block
			irBuilder.CreateBr(pExitBlock);
			
			// Return the output value
			return outValue;			
		}
		else
		{
			// Convert the right value to the object pointer storage mode
			rightVal.pValue = changeStorageMode(
				irBuilder,
				rightVal.pValue,
				rightVal.objType,
				VOID_PTR_TYPE
			);
		}
	}
		
	// Otherwise, both values are stored as matrices
	// Or if other cases did not apply
	{
		// If both values are logical arrays and the logical array computational path was specified
		if (leftVal.objType == DataObject::LOGICALARRAY && rightVal.objType == DataObject::LOGICALARRAY && p2MatrixFuncBool)
		{
			// Create an object for the output value
			Value outValue;
			
			// Store the output object type
			outValue.objType = DataObject::LOGICALARRAY;
			
			// Create a call to the operator function
			LLVMValueVector opArgs;
			opArgs.push_back(leftVal.pValue);
			opArgs.push_back(rightVal.pValue);
			outValue.pValue = createNativeCall(
				irBuilder,
				p2MatrixFuncBool,
				opArgs
			);
			
			// Branch to the exit block
			irBuilder.CreateBr(pExitBlock);
			
			// Return the output value
			return outValue;			
		}
		
		// If both values are char arrays and the char array computational path was specified
		if (leftVal.objType == DataObject::CHARARRAY && rightVal.objType == DataObject::CHARARRAY && p2MatrixFuncChar)
		{
			// Create an object for the output value
			Value outValue;
			
			// Store the output object type
			outValue.objType = boolOutput? DataObject::LOGICALARRAY:DataObject::MATRIX_F64;
			
			// Create a call to the operator function
			LLVMValueVector opArgs;
			opArgs.push_back(leftVal.pValue);
			opArgs.push_back(rightVal.pValue);
			outValue.pValue = createNativeCall(
				irBuilder,
				p2MatrixFuncChar,
				opArgs
			);
			
			// Branch to the exit block
			irBuilder.CreateBr(pExitBlock);
			
			// Return the output value
			return outValue;			
		}
		
		// If either of the values are floating-point matrices, but neither of them is complex
		if ((leftVal.objType == DataObject::MATRIX_F64 || rightVal.objType == DataObject::MATRIX_F64) &&
			(leftVal.objType != DataObject::MATRIX_C128 && rightVal.objType != DataObject::MATRIX_C128) &&
			(leftVal.objType != DataObject::UNKNOWN && rightVal.objType != DataObject::UNKNOWN))
		{
			// Ensure that the floating-point computational path was specified
			assert (p2MatrixFuncF64);
			
			// If the left value is not an f64 matrix
			if (leftVal.objType != DataObject::MATRIX_F64)
			{
				// Convert the left value to an f64 matrix
				LLVMValueVector convArgs;
				convArgs.push_back(leftVal.pValue);
				convArgs.push_back(getObjType(DataObject::MATRIX_F64));
				leftVal.pValue = createNativeCall(
					irBuilder,
					(void*)DataObject::convertType,
					convArgs
				);
				
				// Update the right object type
				leftVal.objType = DataObject::MATRIX_F64;
			}
			
			// If the right value is not an f64 matrix
			if (rightVal.objType != DataObject::MATRIX_F64)
			{
				// Convert the left value to an f64 matrix
				LLVMValueVector convArgs;
				convArgs.push_back(rightVal.pValue);
				convArgs.push_back(getObjType(DataObject::MATRIX_F64));
				rightVal.pValue = createNativeCall(
					irBuilder,
					(void*)DataObject::convertType,
					convArgs
				);
				
				// Update the right object type
				rightVal.objType = DataObject::MATRIX_F64;
			}
			
			// Create an object for the output value
			Value outValue;
			
			// Store the output object type
			outValue.objType = boolOutput? DataObject::LOGICALARRAY:DataObject::MATRIX_F64;
			
			// Create a call to the operator function
			LLVMValueVector opArgs;
			opArgs.push_back(leftVal.pValue);
			opArgs.push_back(rightVal.pValue);
			outValue.pValue = createNativeCall(
				irBuilder,
				p2MatrixFuncF64,
				opArgs
			);
			
			// Branch to the exit block
			irBuilder.CreateBr(pExitBlock);
			
			// Return the output value
			return outValue;
		}
		
		// If either of the values are complex matrices and the complex computational path was specified
		if ((leftVal.objType == DataObject::MATRIX_C128 || rightVal.objType == DataObject::MATRIX_C128) && p2MatrixFuncC128)
		{
			// If the left value is not a complex matrix
			if (leftVal.objType != DataObject::MATRIX_C128)
			{
				// Convert the left value to a complex matrix
				LLVMValueVector convArgs;
				convArgs.push_back(leftVal.pValue);
				convArgs.push_back(getObjType(DataObject::MATRIX_C128));
				leftVal.pValue = createNativeCall(
					irBuilder,
					(void*)DataObject::convertType,
					convArgs
				);
				
				// Update the right object type
				leftVal.objType = DataObject::MATRIX_C128;
			}
			
			// If the right value is not a complex matrix
			if (rightVal.objType != DataObject::MATRIX_C128)
			{
				// Convert the left value to a complex matrix
				LLVMValueVector convArgs;
				convArgs.push_back(rightVal.pValue);
				convArgs.push_back(getObjType(DataObject::MATRIX_C128));
				rightVal.pValue = createNativeCall(
					irBuilder,
					(void*)DataObject::convertType,
					convArgs
				);
				
				// Update the right object type
				rightVal.objType = DataObject::MATRIX_C128;
			}
			
			// Create an object for the output value
			Value outValue;
			
			// Store the output object type
			outValue.objType = boolOutput? DataObject::LOGICALARRAY:DataObject::MATRIX_C128;
			
			// Create a call to the operator function
			LLVMValueVector opArgs;
			opArgs.push_back(leftVal.pValue);
			opArgs.push_back(rightVal.pValue);
			outValue.pValue = createNativeCall(
				irBuilder,
				p2MatrixFuncC128,
				opArgs
			);
			
			// Branch to the exit block
			irBuilder.CreateBr(pExitBlock);
			
			// Return the output value
			return outValue;
		}
	
		// Otherwise, both values are of unknown types
		// Or other cases did not apply
		{
			// Ensure that the default computational path was specified
			assert (p2MatrixFuncDef);
			
			// Create an object for the output value
			Value outValue;
			
			// Store the output object type
			outValue.objType = boolOutput? DataObject::LOGICALARRAY:DataObject::UNKNOWN;
			
			// Create a call to the operator function
			LLVMValueVector opArgs;
			opArgs.push_back(leftVal.pValue);
			opArgs.push_back(rightVal.pValue);
			outValue.pValue = createNativeCall(
				irBuilder,
				p2MatrixFuncDef,
				opArgs
			);
			
			// Branch to the exit block
			irBuilder.CreateBr(pExitBlock);
			
			// Return the output value
			return outValue;			
		}
	}
}

/***************************************************************
* Function: JITCompiler::compParamExpr()
* Purpose : Compile a parameterized expression
* Initial : Maxime Chevalier-Boisvert on March 24, 2009
****************************************************************
Revisions and bug fixes:
*/
JITCompiler::ValueVector JITCompiler::compParamExpr(
	ParamExpr* pParamExpr,
	size_t nargout,
	CompFunction& function,
	CompVersion& version,
	const Expression::SymbolSet& liveVars,
	const VarDefMap& reachDefs,
	const VarTypeMap& varTypes,
	VariableMap& varMap,
	llvm::BasicBlock* pEntryBlock,
	llvm::BasicBlock* pExitBlock
)
{
	// If we are in verbose mode, log the parameterized expression compilation
	if (ConfigManager::s_verboseVar)
		std::cout << "Compiling parameterized expression: " << pParamExpr->toString() << std::endl;
	
	// Get the symbol for the parameterized expression
	SymbolExpr* pSymbol = pParamExpr->getSymExpr();
	
	// Get a reference to the argument vector
	const ParamExpr::ExprVector& arguments = pParamExpr->getArguments();
	
	// Lookup the symbol in the reaching definitions
	VarDefMap::const_iterator symDefItr = reachDefs.find(pSymbol);
	assert (symDefItr != reachDefs.end());
	const VarDefSet& symDefSet = symDefItr->second;
	
	// If the only reaching definition is the definition from the environment
	if (symDefSet.size() == 1 && symDefSet.find(NULL) != symDefSet.end())
	{
		// Declare a pointer for the symbol's binding
		DataObject* pObject = NULL;
		
		// Setup a try block to catch errors
		try
		{
			// Lookup the symbol in the local environment
			pObject = Interpreter::evalSymbol(pSymbol, ProgFunction::getLocalEnv(function.pProgFunc));
		}
		
		// Catch any run-time error
		catch (RunError error)
		{
			// If the symbol lookup fails, do nothing
		}		
		
		// If the object is a function
		if (pObject != NULL && pObject->getType() == DataObject::FUNCTION)
		{
			// Compile the function call
			return compFunctionCall(
				(Function*)pObject,
				pParamExpr->getArguments(),
				nargout,
				pParamExpr,
				(void*)Interpreter::evalParamExpr,
				function,
				version,
				liveVars,
				reachDefs,
				varTypes,
				varMap,
				pEntryBlock,
				pExitBlock
			);			
		}
	}

	//std::cout << "Looking up symbol type for: " << pSymbol->toString() << std::endl;
	
	// Get the type set associated with the symbol
	VarTypeMap::const_iterator typeItr = varTypes.find(pSymbol);
	TypeSet symTypes = (typeItr != varTypes.end())? typeItr->second:TypeSet();
	
	// Get the storage mode for this variable
	DataObject::Type symObjType;
	getStorageMode(
		symTypes,
		symObjType
	);
	
	// Declare a flag to indicate that all arguments are scalars
	bool argsScalar = true;
	
	// For each argument
	for (ParamExpr::ExprVector::const_iterator argItr = arguments.begin(); argItr != arguments.end(); ++argItr)
	{
		// Get a pointer to the argument expression
		Expression* pArgExpr = *argItr;
		
		// Declare a variable for the storage mode
		llvm::Type* storageMode;
		
		//std::cout << "Finding arg type for expr: " << pArgExpr->toString() << std::endl;
		
		// If the expression is a symbol 
		if (pArgExpr->getExprType() == Expression::SYMBOL)
		{
			// Get the type set associated with the symbol
			VarTypeMap::const_iterator typeItr = varTypes.find((SymbolExpr*)pArgExpr);
			TypeSet symTypes = (typeItr != varTypes.end())? typeItr->second:TypeSet();
			
			// Get the storage mode for this variable
			DataObject::Type objType;
			storageMode = getStorageMode(
				symTypes,
				objType
			);
		}
		else
		{
			// Get the type set associated with the expression
			ExprTypeMap::const_iterator typeItr = version.pTypeInferInfo->exprTypeMap.find(pArgExpr);
			assert (typeItr != version.pTypeInferInfo->exprTypeMap.end());
			TypeSet exprTypes = (typeItr->second.size() == 1)? typeItr->second[0]:TypeSet();
			
			// Get the storage mode for this expression
			DataObject::Type objType;
			storageMode = getStorageMode(
				exprTypes,
				objType
			);			
		}
		
		// If the storage mode is not int64 or float64, set the scalar arguments flag to false
		if (storageMode != llvm::Type::getInt64Ty(*s_Context) 
                        && storageMode != llvm::Type::getDoubleTy(*s_Context))
			argsScalar = false;		
	}		
	
	// If we can establish that this object is a non-complex matrix
	// and all arguments are scalar
	// and array access optimizations are enabled
	if (symObjType >= DataObject::MATRIX_I32 && symObjType <= DataObject::CHARARRAY && 
		symObjType != DataObject::MATRIX_C128 && argsScalar &&
		s_jitUseArrayOpts.getBoolValue() == true)
	{
		// Create a basic block for the symbol evaluation exit
		llvm::BasicBlock* pSymExitBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);			
		
		// Compile the symbol expression to get its value
		Value symValue = compSymbolEval(
			(SymbolExpr*)pSymbol,
			function,
			version,
			liveVars,
			reachDefs,
			varTypes,
			varMap,
			pEntryBlock,
			pSymExitBlock
		);
		
		// Create an IR builder for the symbol evaluation exit block
		llvm::IRBuilder<> symExitBuilder(pSymExitBlock);
		
		// Set the storage mode of the variable to the object pointer type
		llvm::Value* pSymObject = changeStorageMode(
			symExitBuilder,
			symValue.pValue,
			symValue.objType,
			VOID_PTR_TYPE
		);
		
		// Set the current IR builder to the symbol exit builder
		llvm::IRBuilder<> currentBuilder = symExitBuilder;
		
		// Create a vector to store the argument values
		std::vector<llvm::Value*> argValues;
		
		// For each argument
		for (ParamExpr::ExprVector::const_iterator argItr = arguments.begin(); argItr != arguments.end(); ++argItr)
		{
			// Get a pointer to the argument expression
			Expression* pArgExpr = *argItr;
			
			// Create a basic block for the argument evaluation exit
			llvm::BasicBlock* pArgExitBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
			
			// Compile the argument expression to get its value
			Value argValue = compExpression(
				pArgExpr,
				function,
				version,
				liveVars,
				reachDefs,
				varTypes,
				varMap,
				currentBuilder.GetInsertBlock(),
				pArgExitBlock
			);
			
			// Update the current IR builder
			currentBuilder.SetInsertPoint(pArgExitBlock);
			
			// Set the storage mode of the argument to int64
			llvm::Value* pIntArgVal = changeStorageMode(
				currentBuilder,
				argValue.pValue,
				argValue.objType,
				llvm::Type::getInt64Ty(*s_Context)
			);
			
			// Add the value to the vector
			argValues.push_back(pIntArgVal);
		}
		
		// Generate code for the scalar array read operation
		Value readValue = compArrayRead(
			pSymObject,
			symObjType,
			argValues,
			pParamExpr,
			function,
			version,
			currentBuilder.GetInsertBlock(),
			pExitBlock
		);
		
		// Return the read value
		return ValueVector(1, readValue);
	}

	// Generate interpreter fallback code
	return arrayExprFallback(
		pParamExpr,
		(void*)Interpreter::evalParamExpr,
		nargout,
		function,
		version,
		liveVars,
		reachDefs,
		varTypes,
		varMap,
		pEntryBlock,
		pExitBlock
	);
}

/***************************************************************
* Function: JITCompiler::compFunctionCall()
* Purpose : Compile a function call
* Initial : Maxime Chevalier-Boisvert on June 15, 2009
****************************************************************
Revisions and bug fixes:
*/
JITCompiler::ValueVector JITCompiler::compFunctionCall(Function* pCalleeFunc, 
	const Expression::ExprVector& arguments, size_t nargout, Expression* pOrigExpr,
	void* pFallbackFunc, CompFunction& callerFunction, CompVersion& callerVersion,
	const Expression::SymbolSet& liveVars, const VarDefMap& reachDefs, const VarTypeMap& varTypes,
	VariableMap& varMap, llvm::BasicBlock* pEntryBlock, llvm::BasicBlock* pExitBlock)
{	
	// Add the callee function to the callee set
	callerFunction.callees.insert(pCalleeFunc);
	
	// Declare a set for the variables to be written to the local environment
	Expression::SymbolSet writeSet;
	
	// If the callee is a program function nested inside the caller function
	if (pCalleeFunc->isProgFunction() == true && ((ProgFunction*)pCalleeFunc)->getParent() == callerFunction.pProgFunc)
	{
		// Get a typed pointer to the callee function
		ProgFunction* pProgFunc = (ProgFunction*)pCalleeFunc;
		
		// Create an IR builder for the current basic block
		llvm::IRBuilder<> currentBuilder(pEntryBlock);
	
		// Get the defs of the caller function
		Expression::SymbolSet callerDefs = callerFunction.pProgFunc->getSymbolDefs();
		
		// Get the uses of the callee function
		Expression::SymbolSet calleeUses = pProgFunc->getSymbolUses();
		
		//std::cout << "computing intersection" << std::endl;
		
		// Compute the intersection of both sets
		for (Expression::SymbolSet::const_iterator itr = calleeUses.begin(); itr != calleeUses.end(); ++itr)
			if (callerDefs.find(*itr) != callerDefs.end()) writeSet.insert(*itr);
		
		//std::cout << "computed intersection" << std::endl;
		
		// If the intersection is not empty or the caller is also a nested function
		if (writeSet.empty() == false || callerFunction.pProgFunc->getParent() != NULL)
		{
			//std::cout << "WRITING VARS TO ENV" << std::endl;
			
			// Write the variable the callee may use in the current environment
			writeVariables(currentBuilder, callerFunction, callerVersion, varMap, writeSet);
		}
	}
	
	// If fallback code should be generated
	// TODO: why can't the caller be nested? check reaching def analysis
	if (callerFunction.pProgFunc->getParent() != NULL)	
	{	
		// Generate interpreter fallback code
		return arrayExprFallback(pOrigExpr, pFallbackFunc, nargout, callerFunction, callerVersion,
			liveVars, reachDefs, varTypes, varMap, pEntryBlock, pExitBlock);
	}
	
	// If the callee is a program function nested inside the caller function
	if (pCalleeFunc->isProgFunction() == true && ((ProgFunction*)pCalleeFunc)->getParent() == callerFunction.pProgFunc)
	{
		// Create an IR builder for the current basic block
		llvm::IRBuilder<> currentBuilder(pEntryBlock);
		
		// If the intersection is not empty or the caller is also a nested function
		if (writeSet.empty() == false || callerFunction.pProgFunc->getParent() != NULL)
		{
			//std::cout << "SETTING CALLEE LOCAL ENV" << std::endl;
			
			// Set the callee's local environment as the current call environment
			LLVMValueVector setArgs;
			setArgs.push_back(createPtrConst(pCalleeFunc));
			setArgs.push_back(getCallEnv(callerFunction, callerVersion));
			createNativeCall(
				currentBuilder,
				(void*)ProgFunction::setLocalEnv,
				setArgs
			);
		}
	}
	
	// Declare a type set string for the input argument types
	TypeSetString inArgTypes;

	// Declare a flag to indicate that the argument count is fixed
	bool argCountFixed = true;
	
	// For each input argument
	for (size_t i = 0; i < arguments.size(); ++i)
	{
		// Get a pointer to this argument expression
		Expression* pArgExpr = arguments[i];
		
		// Declare a type set for the possible expression types
		TypeSet exprTypes;
		
		// If the expression is a symbol
		if (pArgExpr->getExprType() == Expression::SYMBOL)
		{
			// Get the type set associated with the symbol
			VarTypeMap::const_iterator typeItr = varTypes.find((SymbolExpr*)pArgExpr);
			exprTypes = (typeItr != varTypes.end())? typeItr->second:TypeSet();

			// Add the expression types to the type set string
			inArgTypes.push_back(exprTypes);
		}
		else
		{
			// Get the type set associated with the expression
			ExprTypeMap::const_iterator typeItr = callerVersion.pTypeInferInfo->exprTypeMap.find(pArgExpr);
			assert (typeItr != callerVersion.pTypeInferInfo->exprTypeMap.end());			
			
			// If this is a cell-indexing expression and the number of values is not 1, the argument count is not fixed
			if (pArgExpr->getExprType() == Expression::CELL_INDEX && typeItr->second.size() != 1)
				argCountFixed = false;
			// Add the type sets to the input argument types
			inArgTypes.insert(inArgTypes.end(), typeItr->second.begin(), typeItr->second.end());
		}
	}
	
	// If the function is a library function with a fixed argument count and optimized library functions should be used
	if (pCalleeFunc->isProgFunction() == false && argCountFixed == true && s_jitUseLibOpts == true)
	{
		
		// Get the possible return types
		ExprTypeMap::const_iterator typeItr = callerVersion.pTypeInferInfo->exprTypeMap.find(pOrigExpr);
		assert (typeItr != callerVersion.pTypeInferInfo->exprTypeMap.end());
		TypeSet returnTypes = (typeItr->second.size() == 1)? typeItr->second[0]:TypeSet();
		
		// Create a key object for this function
		LibFuncKey key((LibFunction*)pCalleeFunc, inArgTypes, returnTypes);
		
		// Attempt to find the key in the optimized library function map
		LibFuncMap::iterator funcItr = s_libFuncMap.find(key);
		
		// If an entry was found
		if (funcItr != s_libFuncMap.end())
		{
			//std::cout << "LIB FUNC FOUND" << std::endl;
			
			// Create an IR builder for the current basic block
			llvm::IRBuilder<> currentBuilder(pEntryBlock);
			
			// Get a pointer to the native function
			void* pNativeFunc = funcItr->second;
			
			// Create a vector for the argument values
			LLVMValueVector argValues;
			
			// For each input argument
			for (size_t i = 0; i < arguments.size(); ++i)
			{
				// Get a pointer to this argument expression
				Expression* pArgExpr = arguments[i];
				
				// Create a basic block for the expression evaluation exit
				llvm::BasicBlock* pAfterBlock = llvm::BasicBlock::Create(*s_Context, "", callerVersion.pLLVMFunc);
				
				// Compile the expression to obtain its value
				Value argValue = compExpression(pArgExpr, callerFunction, callerVersion, liveVars,
						reachDefs, varTypes, varMap, currentBuilder.GetInsertBlock(), pAfterBlock);
				
				// Update the current basic block
				currentBuilder.SetInsertPoint(pAfterBlock);
				
				// Get the storage mode for this argument
				DataObject::Type argObjType;
				llvm::Type* argType = getStorageMode(inArgTypes[i], argObjType);			
				
				// Change the storage mode of the value to fit the input parameter type
				llvm::Value* pArgValue = changeStorageMode(currentBuilder, argValue.pValue, argValue.objType, argType);
				
				// Add the argument value to the vector
				argValues.push_back(pArgValue);				
			}
			
			// Get the object type for the return type
			DataObject::Type retObjType;
			getStorageMode(returnTypes, retObjType);	
			
			// Create a native call to the optimized library function
			llvm::Value* pRetVal = createNativeCall(currentBuilder, pNativeFunc, argValues);	
			
			// Branch to the exit block
			currentBuilder.CreateBr(pExitBlock);
			
			// Create a value object for the return value
			Value retVal(pRetVal, retObjType);
			
			// Return the return value
			return ValueVector(1, retVal);			
		}
	}
	
	// If the function is a library function, or it should not be JIT compiled, 
	// or the argument count is not fixed, or direct calls should not be used
	if (pCalleeFunc->isProgFunction() == false || ((ProgFunction*)pCalleeFunc)->isScript() == true ||
		argCountFixed == false || s_jitUseDirectCalls == false
	)
	{		
		// Create an IR builder for the current basic block
		llvm::IRBuilder<> currentBuilder(pEntryBlock);
		
		// Create an array object to store the input arguments
		llvm::Value* pInArray = createNativeCall(
			currentBuilder,
			(void*)ArrayObj::create,
			LLVMValueVector(1,llvm::ConstantInt::get(getIntType(sizeof(size_t)),arguments.size())));
		
		// For each argument
		for (ParamExpr::ExprVector::const_iterator argItr = arguments.begin(); argItr != arguments.end(); ++argItr)
		{
			// Get a pointer to the argument expression
			Expression* pArgExpr = *argItr;
			
			// If this is a cell indexing expression
			if (pArgExpr->getExprType() == Expression::CELL_INDEX)
			{
				// Write the symbols used by the expression to the environment
				writeVariables(currentBuilder, callerFunction, callerVersion, varMap, pArgExpr->getSymbolUses());
				
				// Create a native call to evaluate the expresssion
				LLVMValueVector evalArgs;
				evalArgs.push_back(createPtrConst(pArgExpr));
				evalArgs.push_back(getCallEnv(callerFunction, callerVersion));
				llvm::Value* pArgValue = createNativeCall(currentBuilder,
					(void*)Interpreter::evalCellIndexExpr,
					evalArgs);
				
				// Append the argument value to the input array object
				LLVMValueVector appendArgs;
				appendArgs.push_back(pInArray);
				appendArgs.push_back(pArgValue);
				createNativeCall(currentBuilder, (void*)ArrayObj::append, appendArgs);
			}
			else
			{
				// Create basic blocks for the expression compilation
				llvm::BasicBlock* pEntryBlock = llvm::BasicBlock::Create(*s_Context, "", callerVersion.pLLVMFunc);
				llvm::BasicBlock* pExitBlock = llvm::BasicBlock::Create(*s_Context, "", callerVersion.pLLVMFunc);

				// Compile the expression to get the argument value
				Value argValue = compExpression(pArgExpr, callerFunction, callerVersion, liveVars,
						reachDefs, varTypes, varMap, pEntryBlock, pExitBlock);

				// Jump to the entry block
				currentBuilder.CreateBr(pEntryBlock);

				// Update the current IR builder
				currentBuilder.SetInsertPoint(pExitBlock);

				// Store the argument as an object pointer
				llvm::Value* pArgObj = changeStorageMode(currentBuilder, argValue.pValue, argValue.objType, VOID_PTR_TYPE);
				
				// Add the argument value to the input array object
				LLVMValueVector addArgs;
				addArgs.push_back(pInArray);
				addArgs.push_back(pArgObj);
				createNativeCall(
					currentBuilder,
					(void*)ArrayObj::addObject,
					addArgs
				);
			}
		}
						
		// Create an integer constant for the output argument count
		llvm::Value* pOutArgCount = llvm::ConstantInt::get(
			getIntType(sizeof(size_t)),
			nargout
		);
		
		// Perform the function call
		LLVMValueVector callArgs;
		callArgs.push_back(createPtrConst(pCalleeFunc));
		callArgs.push_back(pInArray);
		callArgs.push_back(pOutArgCount);
		llvm::Value* pOutArray = createNativeCall(
			currentBuilder,
			(void*)Interpreter::callFunction,
			callArgs
		);
		
		// Find the type set for this expression
		ExprTypeMap::const_iterator typeItr = callerVersion.pTypeInferInfo->exprTypeMap.find(pOrigExpr);
		assert (typeItr != callerVersion.pTypeInferInfo->exprTypeMap.end());
		const TypeSetString& exprTypes = typeItr->second;
			
		// Create vectors for the storage modes and object types of the output values
		LLVMTypeVector storeModes(nargout, VOID_PTR_TYPE);
		std::vector<DataObject::Type> objTypes(nargout, DataObject::UNKNOWN);
		
		// If type information about the values is available
		if (exprTypes.size() >= nargout)
		{
			// For each value
			for (size_t i = 0; i < nargout; ++i)
			{
				// Get the optimal storage mode for the value
				storeModes[i] = getStorageMode(
					exprTypes[i],
					objTypes[i]
				);
			}
		}

		// Extract the output values from the array object
		return getArrayValues(
			pOutArray,
			nargout,
			true,
			"insufficient number of return values",
			pOrigExpr,
			storeModes,
			objTypes,
			callerVersion.pLLVMFunc,
			currentBuilder.GetInsertBlock(),
			pExitBlock
		);
	}
	
	// Otherwise, if the callee function can be JIT-compiled
	else
	{
		ValueVector valueVector;
		compFuncCallJIT(pCalleeFunc, arguments, nargout, pOrigExpr, pFallbackFunc, callerFunction, callerVersion,
			liveVars, reachDefs, varTypes, varMap, pEntryBlock, pExitBlock, inArgTypes, valueVector);
		
		return valueVector;
	}
}

/***************************************************************
* Function: JITCompiler::compFuncCallJIT()
* Purpose : Compile (JIT) a function call
* Initial : Maxime Chevalier-Boisvert on June 15, 2009
****************************************************************
Revisions and bug fixes:
*/
void JITCompiler::compFuncCallJIT(
	Function* pCalleeFunc,
	const Expression::ExprVector& arguments,
	size_t nargout,
	Expression* pOrigExpr,
	void* pFallbackFunc,
	CompFunction& callerFunction,
	CompVersion& callerVersion,
	const Expression::SymbolSet& liveVars,
	const VarDefMap& reachDefs,
	const VarTypeMap& varTypes,
	VariableMap& varMap,
	llvm::BasicBlock* pEntryBlock,
	llvm::BasicBlock* pExitBlock,
	const TypeSetString& inArgTypes,
	ValueVector& valueVector
)
{
	// Get a typed pointer to the callee function
	ProgFunction* pProgFunc = (ProgFunction*)pCalleeFunc;
	
	// Attempt to find the function in the function map
	FunctionMap::iterator funcItr = s_functionMap.find(pProgFunc);
	
	// If the version of this function is not yet compiled
	if (funcItr == s_functionMap.end() || funcItr->second.versions.find(inArgTypes) == funcItr->second.versions.end())
	{
		// Compile the requested function version
		compileFunction(pProgFunc, inArgTypes);
	}
	
	// Find the compiled function object
	funcItr = s_functionMap.find(pProgFunc);
	assert (funcItr != s_functionMap.end());
	
	// Get a reference to the compiled function object
	CompFunction& calleeFunction = funcItr->second;
	
	// Find a version of this function matching the argument types
	VersionMap::iterator versionItr = calleeFunction.versions.find(inArgTypes);
	assert (versionItr != calleeFunction.versions.end());
	
	// Get a reference to the compiled function version
	CompVersion& calleeVersion = versionItr->second;
	
	// Get the call input/output structures
	LLVMValuePair callStructs = getCallStructs(callerFunction, callerVersion, calleeFunction, calleeVersion);
	
	// Create an IR builder for the current basic block
	llvm::IRBuilder<> currentBuilder(pEntryBlock);
	
	// Cast the in/out struct pointers to the proper type
	llvm::Value* pInStructPtr = currentBuilder.CreateBitCast(callStructs.first, llvm::PointerType::getUnqual(calleeVersion.pInStructType));
	llvm::Value* pOutStructPtr = currentBuilder.CreateBitCast(callStructs.second, llvm::PointerType::getUnqual(calleeVersion.pOutStructType));
	
	// If there are too many input arguments
	if (arguments.size() > pProgFunc->getInParams().size())
	{
		// Throw a compilation error
		throw CompError("too many input arguments", pOrigExpr);
	}		
	
	// For each input argument
	for (size_t i = 0; i < arguments.size(); ++i)
	{
		// Get a pointer to this argument expression
		Expression* pArgExpr = arguments[i];
	
		// Create a basic block for the expression evaluation exit
		llvm::BasicBlock* pAfterBlock = llvm::BasicBlock::Create(*s_Context, "", callerVersion.pLLVMFunc);
		
		// Compile the expression to obtain its value
		Value argValue = compExpression(
			pArgExpr,
			callerFunction,
			callerVersion,
			liveVars,
			reachDefs,
			varTypes,
			varMap,
			currentBuilder.GetInsertBlock(),
			pAfterBlock
		);
		
		// Update the current basic block
		currentBuilder.SetInsertPoint(pAfterBlock);
		
		// Change the storage mode of the value to fit the input parameter type
		llvm::Value* pArgValue = changeStorageMode(
			currentBuilder,
			argValue.pValue,
			argValue.objType,
			calleeVersion.inArgStoreModes[i]
		);
		
		// Write the value to the input data structure
		llvm::Value* gepValues[2];
		gepValues[0] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*s_Context), 0);
		gepValues[1] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*s_Context), i);
        llvm::ArrayRef<llvm::Value*> gepValuesRef(gepValues, gepValues+2);
		llvm::Value* pFieldAddr = currentBuilder.CreateGEP(pInStructPtr, gepValuesRef);
		currentBuilder.CreateStore(pArgValue, pFieldAddr);			
	}
	
	//std::cout << "Storing nargout value" << std::endl;
	
	// Store the number of values to output (nargout) in the input structure
	llvm::Value* gepValuesNargout[2];
	gepValuesNargout[0] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*s_Context), 0);
	gepValuesNargout[1] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*s_Context), arguments.size());
    llvm::ArrayRef<llvm::Value*> gepValNargoutRef(gepValuesNargout, gepValuesNargout+2);
	llvm::Value* pNargoutFieldAddr = currentBuilder.CreateGEP(pInStructPtr, gepValNargoutRef);
	llvm::Value* pNargoutVal = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*s_Context), nargout);
	currentBuilder.CreateStore(pNargoutVal, pNargoutFieldAddr);
			
	//std::cout << "Stored nargout value" << std::endl;
	
	// If this is a recursive call, and thus the function is not yet compiled
	if (calleeFunction.pProgFunc == pProgFunc)
	{
		// Create a direct call to the function
		LLVMValueVector callArgs;
		callArgs.push_back(pInStructPtr);
		callArgs.push_back(pOutStructPtr);
		currentBuilder.CreateCall(calleeVersion.pLLVMFunc, callArgs);
	}
	else
	{
		// Create a native call through the exception handler
		LLVMValueVector callArgs;
		callArgs.push_back(createPtrConst(pProgFunc));
		callArgs.push_back(createPtrConst((void*)calleeVersion.pFuncPtr));
		callArgs.push_back(callerVersion.pInStruct);
		callArgs.push_back(callerVersion.pOutStruct);
		createNativeCall(
			currentBuilder,
			(void*)JITCompiler::callExceptHandler,
			callArgs
		);
	}
	
	// If the number of output values should be tested
	if (nargout > 0)
	{
		//std::cout << "****** nargout for call: " << nargout << std::endl;
		
		// Load the number of values written from the output structure
		llvm::Value* gepValuesNum[2];
		gepValuesNum[0] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*s_Context), 0);
		gepValuesNum[1] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*s_Context), pProgFunc->getOutParams().size());
        llvm::ArrayRef<llvm::Value*> gepValuesRef(gepValuesNum, gepValuesNum+2);
		llvm::Value* pNumFieldAddr = currentBuilder.CreateGEP(pOutStructPtr, gepValuesRef);
		llvm::Value* pNumOutVals = currentBuilder.CreateLoad(pNumFieldAddr);
		
		// Create a basic block to replace the current entry block
		llvm::BasicBlock* pNewCurrentBlock = llvm::BasicBlock::Create(*s_Context, "", callerVersion.pLLVMFunc);
		
		// Create a basic block for the insufficient size case
		llvm::BasicBlock* pFailBlock = llvm::BasicBlock::Create(*s_Context, "", callerVersion.pLLVMFunc);
		llvm::IRBuilder<> failBuilder(pFailBlock);
		
		// Test whether or not the size is less than the requested number of values
		llvm::Value* pCompVal = currentBuilder.CreateICmpSLT(
			pNumOutVals, 
			llvm::ConstantInt::get(
				llvm::Type::getInt64Ty(*s_Context),
				nargout
			)
		);
		
		// If the value is true, branch to the fail block, otherwise to the new entry block
		currentBuilder.CreateCondBr(pCompVal, pFailBlock, pNewCurrentBlock);
		
		// In the fail block, throw an exception
		createThrowError(
			failBuilder,
			"too many output values requested",
			pOrigExpr
		);
		
		// Terminate the basic block to keep the CFG proper
		failBuilder.CreateRetVoid();
		
		// Update the current basic block
		currentBuilder.SetInsertPoint(pNewCurrentBlock);
	}
	
	// For each output value
	for (size_t i = 0; i < nargout; ++i)
	{
		// Load this value from the output structure
		llvm::Value* gepValues[2];
		gepValues[0] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*s_Context), 0);
		gepValues[1] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*s_Context), i);
        llvm::ArrayRef<llvm::Value*> gepValuesRef(gepValues, gepValues+2);
		llvm::Value* pFieldAddr = currentBuilder.CreateGEP(pOutStructPtr, gepValuesRef);
		llvm::Value* pFieldValue = currentBuilder.CreateLoad(pFieldAddr);
		
		// Add the value to the vector
		valueVector.push_back(Value(pFieldValue, calleeVersion.outArgObjTypes[i]));
	}
	
	// Branch to the exit block
	currentBuilder.CreateBr(pExitBlock);
}

/***************************************************************
* Function: JITCompiler::compSymbolExpr()
* Purpose : Compile a symbol expression
* Initial : Maxime Chevalier-Boisvert on June 15, 2009
****************************************************************
Revisions and bug fixes:
*/
JITCompiler::ValueVector JITCompiler::compSymbolExpr(
	SymbolExpr* pSymbolExpr,
	size_t nargout,
	CompFunction& function,
	CompVersion& version,
	const Expression::SymbolSet& liveVars,
	const VarDefMap& reachDefs,
	const VarTypeMap& varTypes,
	VariableMap& varMap,
	llvm::BasicBlock* pEntryBlock,
	llvm::BasicBlock* pExitBlock
)
{
	// Attempt to find the symbol in the variable map
	VariableMap::iterator mapItr = varMap.find(pSymbolExpr);
	
	// If we are in verbose mode, log the symbol expression evaluation
	if (ConfigManager::s_verboseVar)
		std::cout << "Evaluating symbol expr: " << pSymbolExpr->toString() << std::endl;
	
	// Declare a value vector to store the output
	ValueVector valueVector;
	
	// Create an IR builder for the entry block
	llvm::IRBuilder<> entryBuilder(pEntryBlock);
	
	// If the variable is locally stored
	if (mapItr != varMap.end() && mapItr->second.pValue != NULL)
	{
		// If we are in verbose mode, log that the symbol is stored locally
		if (ConfigManager::s_verboseVar)
			std::cout << "Symbol stored locally" << std::endl;
		
		// Retrieve its value from the variable map
		valueVector.push_back(mapItr->second);
		
		// Link the entry block to the exit block
		entryBuilder.CreateBr(pExitBlock);
	}
	else
	{
		// If we are in verbose mode, log that the symbol is not stored locally
		if (ConfigManager::s_verboseVar)
			std::cout << "Symbol not stored locally" << std::endl;
		
		// If we know for a fact that the value is in the environment
		if (mapItr != varMap.end())
		{
			// Find the type set for this symbol
			VarTypeMap::const_iterator typeItr = varTypes.find(pSymbolExpr);
			assert (typeItr != varTypes.end());
			
			// Read the variable's value from the environment
			Value value = readVariable(entryBuilder, function, version, typeItr->second, pSymbolExpr, false);
			
			// Add the value to the value vector
			valueVector.push_back(value);
			
			// Link the entry block to the exit block
			entryBuilder.CreateBr(pExitBlock);
		}
		else
		{
			// Lookup the symbol in the reaching definitions
			VarDefMap::const_iterator symDefItr = reachDefs.find(pSymbolExpr);
			assert (symDefItr != reachDefs.end());
			const VarDefSet& symDefSet = symDefItr->second;

			// Declare a pointer for the symbol's binding
			DataObject* pObject = NULL;
			
			// If the only reaching definition is the definition from the environment
			if (symDefSet.size() == 1 && symDefSet.find(NULL) != symDefSet.end())
			{
				// Setup a try block to catch errors
				try
				{
					// Lookup the symbol in the local environment
					pObject = Interpreter::evalSymbol(pSymbolExpr, ProgFunction::getLocalEnv(function.pProgFunc));
				}
				
				// Catch any run-time error
				catch (RunError error)
				{
					// If the symbol lookup fails, do nothing
				}				
			}
			
			// If the object is a function
			if (pObject != NULL && pObject->getType() == DataObject::FUNCTION)
			{
				// Compile the function call
				valueVector = compFunctionCall(
					(Function*)pObject,
					Expression::ExprVector(),
					nargout,
					pSymbolExpr,
					(void*)Interpreter::evalSymbolExpr,
					function,
					version,
					liveVars,
					reachDefs,
					varTypes,
					varMap,
					pEntryBlock,
					pExitBlock
				);			
			}
			else
			{			
				// Fall back to the evalSymbolExpr method
				valueVector = arrayExprFallback(
					pSymbolExpr,
					(void*)Interpreter::evalSymbolExpr,
					nargout,
					function,
					version,
					liveVars,
					reachDefs,
					varTypes,
					varMap,
					pEntryBlock,
					pExitBlock
				);
			}
		}
	}

	// If too many output values were requested
	if (nargout > valueVector.size())
	{
		// Throw a compilation error
		throw CompError("too many output values requested", pSymbolExpr);
	}
	
	// Return the value vector
	return valueVector;
}

/***************************************************************
* Function: JITCompiler::compSymbolEval()
* Purpose : Compile a symbol evaluation
* Initial : Maxime Chevalier-Boisvert on June 15, 2009
****************************************************************
Revisions and bug fixes:
*/
JITCompiler::Value JITCompiler::compSymbolEval(
	SymbolExpr* pSymbolExpr,
	CompFunction& function,
	CompVersion& version,
	const Expression::SymbolSet& liveVars,
	const VarDefMap& reachDefs,
	const VarTypeMap& varTypes,
	VariableMap& varMap,
	llvm::BasicBlock* pEntryBlock,
	llvm::BasicBlock* pExitBlock
)
{
	// Attempt to find the symbol in the variable map
	VariableMap::iterator mapItr = varMap.find(pSymbolExpr);
	
	// If we are in verbose mode, log the symbol evaluation
	if (ConfigManager::s_verboseVar)
		std::cout << "Evaluating symbol: " << pSymbolExpr->toString() << std::endl;
	
	// Declare a value to store the output
	Value value;
	
	// Create an IR builder for the entry block
	llvm::IRBuilder<> entryBuilder(pEntryBlock);
	
	// If the variable is locally stored
	if (mapItr != varMap.end() && mapItr->second.pValue != NULL)
	{
		// If we are in verbose mode, log that the symbol is stored locally
		if (ConfigManager::s_verboseVar)
			std::cout << "Symbol stored locally" << std::endl;
		
		// Retrieve its value from the variable map
		value = mapItr->second;
	}
	else
	{
		// If we are in verbose mode, log that the symbol is not stored locally
		if (ConfigManager::s_verboseVar)
			std::cout << "Symbol not stored locally" << std::endl;
		
		// If we know for a fact that the value is in the environment
		if (mapItr != varMap.end())
		{
			// Find the type set for this symbol
			VarTypeMap::const_iterator typeItr = varTypes.find(pSymbolExpr);
			assert (typeItr != varTypes.end());
			
			// Read the variable's value from the environment
			value = readVariable(entryBuilder, function, version, typeItr->second, pSymbolExpr, false);
		}
		else
		{
			// Create a native call to evaluate the symbol
			LLVMValueVector evalArgs;
			evalArgs.push_back(createPtrConst(pSymbolExpr));
			evalArgs.push_back(getCallEnv(function, version));
			llvm::Value* pValue = createNativeCall(
				entryBuilder,
				(void*)Interpreter::evalSymbol,
				evalArgs
			);

			// Create a value object
			value = Value(pValue, DataObject::UNKNOWN);				
		}
	}
	
	// Link the entry block to the exit block
	entryBuilder.CreateBr(pExitBlock);
	
	// Return the value 
	return value;
}

/***************************************************************
* Function: JITCompiler::compArrayRead()
* Purpose : Generate code for a scalar array read operation
* Initial : Maxime Chevalier-Boisvert on July 15, 2009
****************************************************************
Revisions and bug fixes:
*/
JITCompiler::Value JITCompiler::compArrayRead(
	llvm::Value* pMatrixObj,
	DataObject::Type matrixType,
	const LLVMValueVector& indices,
	ParamExpr* pOrigExpr,
	CompFunction& function,
	CompVersion& version,
	llvm::BasicBlock* pEntryBlock,
	llvm::BasicBlock* pExitBlock
)
{
	// Create an IR builder for the entry block
	llvm::IRBuilder<> currentBuilder(pEntryBlock);	
	
	// Declare a value for the size array pointer
	llvm::Value* pSizeArrayPtr = NULL;
	
	// If there is more than one index
	if (indices.size() > 1)
	{
		// Get a pointer to the dimension size array
		pSizeArrayPtr = createNativeCall(
			currentBuilder,
			(void*)BaseMatrixObj::getSizeArray,
			LLVMValueVector(1, pMatrixObj)
		);
	}
	
	// Declare a value vector for the zero-indexing indices
	LLVMValueVector zeroIndices;
		
	// For each index
	for (size_t i = 0; i < indices.size(); ++i)
	{
		// Subtract one from the index
		llvm::Value* pZeroIndex = currentBuilder.CreateSub(indices[i], llvm::ConstantInt::get(llvm::Type::getInt64Ty(*s_Context), 1));
		
		// Add the zero index to the list
		zeroIndices.push_back(pZeroIndex);
	}
		
	//std::cout << "Getting matrix dimension sizes" << std::endl;	
	
	// Declare vectors for the matrix dimension size values
	LLVMValueVector nmiSizes;
	LLVMValueVector tmiSizes;
		
	// Create a basic block for the "not too many indices" case
	llvm::BasicBlock* pNMIBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
	llvm::IRBuilder<> nmiBuilder(pNMIBlock);
	
	// Create a basic block for the "too many indices" case
	llvm::BasicBlock* pTMIBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
	llvm::IRBuilder<> tmiBuilder(pTMIBlock);

	// Declare a pointer for the number of matrix dimensions
	llvm::Value* pNumDims = NULL;
	
	// If we are indexing in more than 2 dimensions and bounds checking is required
	if (indices.size() > 2 && s_jitNoReadBoundChecks == false)
	{
		// Get the number of matrix dimensions
		pNumDims = createNativeCall(
			currentBuilder,
			(void*)BaseMatrixObj::getDimCount,
			LLVMValueVector(1, pMatrixObj)
		);
		
		// Test if there are more indices than matrix dimensions
		llvm::Value* pCompVal = currentBuilder.CreateICmpULT(
			pNumDims, 
			llvm::ConstantInt::get(
				getIntType(sizeof(size_t)),
				indices.size()
			)
		);
		
		// Branch based on the test condition
		currentBuilder.CreateCondBr(pCompVal, pTMIBlock, pNMIBlock);
	}
	else
	{
		// Jump directly to the normal case
		currentBuilder.CreateBr(pNMIBlock);
	}
	
	// In the NMI case, for each index (except the last)
	for (size_t i = 0; i < indices.size() - 1; ++i)
	{
		// Extract the size of the current dimension
		llvm::Value* pDimElemPtr = nmiBuilder.CreateGEP(pSizeArrayPtr, llvm::ConstantInt::get(llvm::Type::getInt64Ty(*s_Context), i));
		llvm::Value* pDimSize = nmiBuilder.CreateLoad(pDimElemPtr);
		pDimSize = nmiBuilder.CreateIntCast(pDimSize, llvm::Type::getInt64Ty(*s_Context), false);		
		
		// Add the dimension size to the list
		nmiSizes.push_back(pDimSize);
	}
	
	// In the TMI case, for the first two indices
	for (size_t i = 0; i < 2 && i < indices.size() - 1; ++i)
	{
		// Extract the size of the current dimension
		llvm::Value* pDimElemPtr = tmiBuilder.CreateGEP(pSizeArrayPtr, llvm::ConstantInt::get(llvm::Type::getInt64Ty(*s_Context), i));
		llvm::Value* pDimSize = tmiBuilder.CreateLoad(pDimElemPtr);
		pDimSize = tmiBuilder.CreateIntCast(pDimSize, llvm::Type::getInt64Ty(*s_Context), false);		
		
		// Add the dimension size to the list
		tmiSizes.push_back(pDimSize);
	}	
	
	// In the TMI case, for each remaining index (except the last)
	for (size_t i = 2; i < indices.size() - 1; ++i)
	{
		// Test if this index is past the end of the size array
		llvm::Value* pCompVal = tmiBuilder.CreateICmpULT(
			pNumDims, 
			llvm::ConstantInt::get(
				getIntType(sizeof(size_t)),
				i+1
			)
		);

		// Create a basic block for the "past the end" case
		llvm::BasicBlock* pPastBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
		llvm::IRBuilder<> pastBuilder(pPastBlock);
		
		// Create a basic block for the "within the array" case
		llvm::BasicBlock* pWithinBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
		llvm::IRBuilder<> withinBuilder(pWithinBlock);
		
		// Create a basic block for the resolution
		llvm::BasicBlock* pResBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
		llvm::IRBuilder<> resBuilder(pResBlock);		
		
		// Branch based on the comparison value
		tmiBuilder.CreateCondBr(pCompVal, pPastBlock, pWithinBlock);
		
		// The dimension has size 1 
		llvm::Value* pPastValue = llvm::ConstantInt::get(
			llvm::Type::getInt64Ty(*s_Context),
			1
		);
		
		// Branch to the resolution block
		pastBuilder.CreateBr(pResBlock);
		
		// Extract the size of the current dimension
		llvm::Value* pDimElemPtr = withinBuilder.CreateGEP(pSizeArrayPtr, llvm::ConstantInt::get(llvm::Type::getInt64Ty(*s_Context), i));
		llvm::Value* pDimSize = withinBuilder.CreateLoad(pDimElemPtr);
		pDimSize = withinBuilder.CreateIntCast(pDimSize, llvm::Type::getInt64Ty(*s_Context), false);		
				
		// Branch to the resolution block
		withinBuilder.CreateBr(pResBlock);		
		
		// Create a phi node for the resolution
		llvm::PHINode* pDimPhi = resBuilder.CreatePHI(llvm::Type::getInt64Ty(*s_Context), 2);
		pDimPhi->addIncoming(pPastValue, pPastBlock);
		pDimPhi->addIncoming(pDimSize, pWithinBlock);
		
		// Add the dimension size to the list
		tmiSizes.push_back(pDimPhi);
		
		// Update the IR builder for the TMI case
		tmiBuilder.SetInsertPoint(pResBlock);
	}
	
	// Declare a vector for the matrix dimension size values
	LLVMValueVector dimSizes;
	
	// Create a basic block for the resolution
	llvm::BasicBlock* pResBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
	llvm::IRBuilder<> resBuilder(pResBlock);		

	// Have both the NMI and TMI cases branch to the resolution block
	nmiBuilder.CreateBr(pResBlock);
	tmiBuilder.CreateBr(pResBlock);
	
	// For each index (except the last)
	for (size_t i = 0; i < indices.size() - 1; ++i)
	{
		// Create a phi node for this value
      llvm::PHINode* pPHINode = resBuilder.CreatePHI(llvm::Type::getInt64Ty(*s_Context), 2);
		
		// Add values for both cases
		pPHINode->addIncoming(nmiSizes[i], nmiBuilder.GetInsertBlock());
		pPHINode->addIncoming(tmiSizes[i], tmiBuilder.GetInsertBlock());
		
		// Add the dimension size to the list
		dimSizes.push_back(pPHINode);
	}
			
	// Update the current IR builder
	currentBuilder.SetInsertPoint(pResBlock);
	
	//std::cout << "Got indices and dimension sizes" << std::endl;
	
	// Declare a value for the data index
	llvm::Value* pIndex = zeroIndices[0];
	
	// Declare a value for the offset product
	llvm::Value* pOffset = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*s_Context), 1);
	
	// For each index (excluding the first)
	for (size_t i = 1; i < zeroIndices.size(); ++i)
	{
		// Get the size of the previous dimension
		llvm::Value* pDimSize = dimSizes[i-1];
		
		// Update the offset product
		pOffset = currentBuilder.CreateMul(pOffset, pDimSize);

		// Get the index along the current dimension
		llvm::Value* pDimIndex = zeroIndices[i];
				
		// Update the index sum
		llvm::Value* pDimOffset = currentBuilder.CreateMul(pDimIndex, pOffset);
		pIndex = currentBuilder.CreateAdd(pIndex, pDimOffset);		
	}

	// If bounds checking is required
	if (s_jitNoReadBoundChecks == false)
	{
		// Create a basic block for the out of bounds size case
		llvm::BasicBlock* pFailBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
		llvm::IRBuilder<> failBuilder(pFailBlock);
		
		// In the fail case, throw an exception
		createThrowError(
			failBuilder,
			"index out of bounds in matrix read",
			pOrigExpr
		);
		
		// Terminate the fail block
		failBuilder.CreateRetVoid();
		
		// If bounds checking is required for the last index
		if (version.pBoundsCheckInfo->isBoundsCheckRequired(pOrigExpr, indices.size() - 1, 0) == true ||
			version.pBoundsCheckInfo->isBoundsCheckRequired(pOrigExpr, indices.size() - 1, 1) == true)
		{
			// Load the number of matrix elements
			llvm::Value* pNumElems = loadMemberValue(
				currentBuilder,
				pMatrixObj,
				MEMBER_OFFSET(BaseMatrixObj, m_numElements),
				getIntType(sizeof(size_t))
			);
			pNumElems = currentBuilder.CreateIntCast(pNumElems, llvm::Type::getInt64Ty(*s_Context), false); 
			
			// Test if the final index data is out of bounds
			llvm::Value* pCompVal = currentBuilder.CreateICmpUGE(pIndex, pNumElems);
						
			// Create a basic block for the sufficient size case
			llvm::BasicBlock* pPassBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
			
			// Branch based on the test condition
			currentBuilder.CreateCondBr(pCompVal, pFailBlock, pPassBlock);
			
			// Update the current basic block
			currentBuilder.SetInsertPoint(pPassBlock);
		}
		
		// For each index (excluding the last)
		for (size_t i = 0; i < zeroIndices.size() - 1; ++i)
		{
			// If no bounds checking is required for this index, skip it
			if (version.pBoundsCheckInfo->isBoundsCheckRequired(pOrigExpr, i, 0) == false &&
				version.pBoundsCheckInfo->isBoundsCheckRequired(pOrigExpr, i, 1) == false)
				continue;
			
			// Get the index along the current dimension
			llvm::Value* pDimIndex = zeroIndices[i];
			
			// Get the size of the current dimension
			llvm::Value* pDimSize = dimSizes[i];
			
			// Test if the index data is out of bounds
			llvm::Value* pCompVal = currentBuilder.CreateICmpUGE(pDimIndex, pDimSize);
			
			// Create a basic block for the sufficient size case
			llvm::BasicBlock* pPassBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
			
			// Branch based on the test condition
			currentBuilder.CreateCondBr(pCompVal, pFailBlock, pPassBlock);
			
			// Update the current basic block
			currentBuilder.SetInsertPoint(pPassBlock);
		}		
	}	
	
	//std::cout << "Bounds checking complete" << std::endl;
	
	// Declare a pointer for the read value
	llvm::Value* pReadValue;
	
	// Switch based on the matrix type
	switch (matrixType)
	{
		// Floating-point matrix
		case DataObject::MATRIX_F64:
		{
			// Load the element array pointer
			llvm::Value* pDataPtr = loadMemberValue(
				currentBuilder,
				pMatrixObj,
				MEMBER_OFFSET(MatrixF64Obj, m_pElements),
				llvm::PointerType::getUnqual(llvm::Type::getDoubleTy(*s_Context))
			);
			
			// Compute the address of the value
			llvm::Value* pValAddr = currentBuilder.CreateGEP(pDataPtr, pIndex);
			
			// Read the value from the array
			pReadValue = currentBuilder.CreateLoad(pValAddr);
		}
		break;

		// Character array
		case DataObject::CHARARRAY:
		{
			// Load the element array pointer
			llvm::Value* pDataPtr = loadMemberValue(
				currentBuilder,
				pMatrixObj,
				MEMBER_OFFSET(CharArrayObj, m_pElements),
				llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(*s_Context))
			);
			
			// Compute the address of the value
			llvm::Value* pValAddr = currentBuilder.CreateGEP(pDataPtr, pIndex);
			
			// Read the value from the array
			pReadValue = currentBuilder.CreateLoad(pValAddr);
			
			// Convert the read value to the int64 type
			pReadValue = currentBuilder.CreateIntCast(pReadValue, llvm::Type::getInt64Ty(*s_Context), true);
		}
		break;
		
		// Logical array array
		case DataObject::LOGICALARRAY:
		{
			// Load the element array pointer
			llvm::Value* pDataPtr = loadMemberValue(
				currentBuilder,
				pMatrixObj,
				MEMBER_OFFSET(LogicalArrayObj, m_pElements),
				llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(*s_Context))
			);
			
			// Compute the address of the value
			llvm::Value* pValAddr = currentBuilder.CreateGEP(pDataPtr, pIndex);
			
			// Read the value from the array
			pReadValue = currentBuilder.CreateLoad(pValAddr);
			
			// Get the boolean value of the read value
			pReadValue = currentBuilder.CreateICmpNE(pReadValue, llvm::ConstantInt::get(llvm::Type::getInt8Ty(*s_Context), 0));
		}
		break;
	
		// For any other matrix type
		default:
		{
			// Throw a compilation error
			throw CompError("unsupported matrix type in scalar read \"" + DataObject::getTypeName(matrixType) + "\"");
		}
	}
	
	// Branch to the exit block
	currentBuilder.CreateBr(pExitBlock);
	
	// Return the read value
	return Value(pReadValue, matrixType);
}

/***************************************************************
* Function: JITCompiler::compArrayWrite()
* Purpose : Generate code for a scalar array write operation
* Initial : Maxime Chevalier-Boisvert on July 15, 2009
****************************************************************
Revisions and bug fixes:
*/
void JITCompiler::compArrayWrite(
	llvm::Value* pMatrixObj,
	DataObject::Type matrixType,
	llvm::Value* pValue,
	DataObject::Type valueType,
	const LLVMValueVector& indices,
	ParamExpr* pOrigExpr,
	CompFunction& function,
	CompVersion& version,
	llvm::BasicBlock* pEntryBlock,
	llvm::BasicBlock* pExitBlock
)
{
	//std::cout << "Compiling scalar array write op" << std::endl;
	
	// Create an IR builder for the entry block
	llvm::IRBuilder<> currentBuilder(pEntryBlock);
	
	// Declare a value for the size array pointer
	llvm::Value* pSizeArrayPtr = NULL;
	
	// If there is more than one index
	if (indices.size() > 1)
	{
		// Get a pointer to the dimension size array
		/*pSizeArrayPtr = createNativeCall(
			currentBuilder,
			(void*)BaseMatrixObj::getSizeArray,
			LLVMValueVector(1, pMatrixObj)
		);*/
		//RAHUL: Changed to more efficient that avoids function call
		unsigned int offset = MEMBER_OFFSET(BaseMatrixObj,m_size)+MEMBER_OFFSET(DimVector,m_ptr);
		llvm::Value* intPtrMatrixObj = currentBuilder.CreatePtrToInt(pMatrixObj,getIntType(8),"inttoptr");
		llvm::Value* intPtrSizeArrayPtr = currentBuilder.CreateAdd(intPtrMatrixObj,llvm::ConstantInt::get(getIntType(8),offset));
		llvm::Value* pPtrSizeArrayPtr = currentBuilder.CreateIntToPtr(intPtrSizeArrayPtr,llvm::PointerType::getUnqual(llvm::PointerType::getUnqual(getIntType(sizeof(size_t)))),"ptrtoint");
		pSizeArrayPtr = currentBuilder.CreateLoad(pPtrSizeArrayPtr);


	}
	
	// Declare a value vector for the zero-indexing indices
	LLVMValueVector zeroIndices;
		
	// For each index
	for (size_t i = 0; i < indices.size(); ++i)
	{
		// Subtract one from the index
		llvm::Value* pZeroIndex = currentBuilder.CreateSub(indices[i], llvm::ConstantInt::get(llvm::Type::getInt64Ty(*s_Context), 1));
		
		// Add the zero index to the list
		zeroIndices.push_back(pZeroIndex);
	}
	
	// Create a basic block for the out of bounds case
	llvm::BasicBlock* pOutBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
	llvm::IRBuilder<> outBuilder(pOutBlock);
	
	// If we are indexing in more than 2 dimensions and bounds checking is required
	if (indices.size() > 2 && s_jitNoWriteBoundChecks == false)
	{
		// Get the number of matrix dimensions
		llvm::Value* pNumDims = createNativeCall(
			currentBuilder,
			(void*)BaseMatrixObj::getDimCount,
			LLVMValueVector(1, pMatrixObj)
		);
		
		// Test if there are more indices than matrix dimensions
		llvm::Value* pCompVal = currentBuilder.CreateICmpULT(
			pNumDims, 
			llvm::ConstantInt::get(
				getIntType(sizeof(size_t)),
				indices.size()
			)
		);
		
		// Create a basic block for the sufficient size case
		llvm::BasicBlock* pPassBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
		
		// Branch based on the test condition
		currentBuilder.CreateCondBr(pCompVal, pOutBlock, pPassBlock);
		
		// Update the current basic block
		currentBuilder.SetInsertPoint(pPassBlock);
	}

	// Declare vectors for the matrix dimension size values
	LLVMValueVector nmiSizes;
	LLVMValueVector tmiSizes;
		
	// Create a basic block for the "not too many indices" case
	llvm::BasicBlock* pNMIBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
	llvm::IRBuilder<> nmiBuilder(pNMIBlock);
	
	// Create a basic block for the "too many indices" case
	llvm::BasicBlock* pTMIBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
	llvm::IRBuilder<> tmiBuilder(pTMIBlock);

	// Declare a pointer for the number of matrix dimensions
	llvm::Value* pNumDims = NULL;
	
	// If we are indexing in more than 2 dimensions and bounds checking is required
	if (indices.size() > 2 && s_jitNoReadBoundChecks == false)
	{
		// Get the number of matrix dimensions
		pNumDims = createNativeCall(
			currentBuilder,
			(void*)BaseMatrixObj::getDimCount,
			LLVMValueVector(1, pMatrixObj)
		);
		
		// Test if there are more indices than matrix dimensions
		llvm::Value* pCompVal = currentBuilder.CreateICmpULT(
			pNumDims, 
			llvm::ConstantInt::get(
				getIntType(sizeof(size_t)),
				indices.size()
			)
		);
		
		// Branch based on the test condition
		currentBuilder.CreateCondBr(pCompVal, pTMIBlock, pNMIBlock);
	}
	else
	{
		// Jump directly to the normal case
		currentBuilder.CreateBr(pNMIBlock);
	}
	
	// In the NMI case, for each index (except the last)
	for (size_t i = 0; i < indices.size() - 1; ++i)
	{
		// Extract the size of the current dimension
		llvm::Value* pDimElemPtr = nmiBuilder.CreateGEP(pSizeArrayPtr, llvm::ConstantInt::get(llvm::Type::getInt64Ty(*s_Context), i));
		llvm::Value* pDimSize = nmiBuilder.CreateLoad(pDimElemPtr);
		pDimSize = nmiBuilder.CreateIntCast(pDimSize, llvm::Type::getInt64Ty(*s_Context), false);		
		
		// Add the dimension size to the list
		nmiSizes.push_back(pDimSize);
	}
	
	// In the TMI case, for the first two indices
	for (size_t i = 0; i < 2 && i < indices.size() - 1; ++i)
	{
		// Extract the size of the current dimension
		llvm::Value* pDimElemPtr = tmiBuilder.CreateGEP(pSizeArrayPtr, llvm::ConstantInt::get(llvm::Type::getInt64Ty(*s_Context), i));
		llvm::Value* pDimSize = tmiBuilder.CreateLoad(pDimElemPtr);
		pDimSize = tmiBuilder.CreateIntCast(pDimSize, llvm::Type::getInt64Ty(*s_Context), false);		
		
		// Add the dimension size to the list
		tmiSizes.push_back(pDimSize);
	}	
	
	// In the TMI case, for each remaining index (except the last)
	for (size_t i = 2; i < indices.size() - 1; ++i)
	{
		// Test if this index is past the end of the size array
		llvm::Value* pCompVal = tmiBuilder.CreateICmpULT(
			pNumDims, 
			llvm::ConstantInt::get(
				getIntType(sizeof(size_t)),
				i+1
			)
		);

		// Create a basic block for the "past the end" case
		llvm::BasicBlock* pPastBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
		llvm::IRBuilder<> pastBuilder(pPastBlock);
		
		// Create a basic block for the "within the array" case
		llvm::BasicBlock* pWithinBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
		llvm::IRBuilder<> withinBuilder(pWithinBlock);
		
		// Create a basic block for the resolution
		llvm::BasicBlock* pResBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
		llvm::IRBuilder<> resBuilder(pResBlock);		
		
		// Branch based on the comparison value
		tmiBuilder.CreateCondBr(pCompVal, pPastBlock, pWithinBlock);
		
		// The dimension has size 1 
		llvm::Value* pPastValue = llvm::ConstantInt::get(
			llvm::Type::getInt64Ty(*s_Context),
			1
		);
		
		// Branch to the resolution block
		pastBuilder.CreateBr(pResBlock);
		
		// Extract the size of the current dimension
		llvm::Value* pDimElemPtr = withinBuilder.CreateGEP(pSizeArrayPtr, llvm::ConstantInt::get(llvm::Type::getInt64Ty(*s_Context), i));
		llvm::Value* pDimSize = withinBuilder.CreateLoad(pDimElemPtr);
		pDimSize = withinBuilder.CreateIntCast(pDimSize, llvm::Type::getInt64Ty(*s_Context), false);		
				
		// Branch to the resolution block
		withinBuilder.CreateBr(pResBlock);		
		
		// Create a phi node for the resolution
		llvm::PHINode* pDimPhi = resBuilder.CreatePHI(llvm::Type::getInt64Ty(*s_Context), 2);
		pDimPhi->addIncoming(pPastValue, pPastBlock);
		pDimPhi->addIncoming(pDimSize, pWithinBlock);
		
		// Add the dimension size to the list
		tmiSizes.push_back(pDimPhi);
		
		// Update the IR builder for the TMI case
		tmiBuilder.SetInsertPoint(pResBlock);
	}
	
	// Declare a vector for the matrix dimension size values
	LLVMValueVector dimSizes;
	
	// Create a basic block for the resolution
	llvm::BasicBlock* pResBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
	llvm::IRBuilder<> resBuilder(pResBlock);		

	// Have both the NMI and TMI cases branch to the resolution block
	nmiBuilder.CreateBr(pResBlock);
	tmiBuilder.CreateBr(pResBlock);
	
	// For each index (except the last)
	for (size_t i = 0; i < indices.size() - 1; ++i)
	{
		// Create a phi node for this value
      llvm::PHINode* pPHINode = resBuilder.CreatePHI(llvm::Type::getInt64Ty(*s_Context), 2);
		
		// Add values for both cases
		pPHINode->addIncoming(nmiSizes[i], nmiBuilder.GetInsertBlock());
		pPHINode->addIncoming(tmiSizes[i], tmiBuilder.GetInsertBlock());
		
		// Add the dimension size to the list
		dimSizes.push_back(pPHINode);
	}
			
	// Update the current IR builder
	currentBuilder.SetInsertPoint(pResBlock);	
	
	// Declare a value for the data index
	llvm::Value* pIndex = zeroIndices[0];
	
	// Declare a value for the offset product
	llvm::Value* pOffset = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*s_Context), 1);
	
	// For each index (excluding the first)
	for (size_t i = 1; i < zeroIndices.size(); ++i)
	{
		// Get the size of the previous dimension
		llvm::Value* pDimSize = dimSizes[i-1];
		
		// Update the offset product
		pOffset = currentBuilder.CreateMul(pOffset, pDimSize);

		// Get the index along the current dimension
		llvm::Value* pDimIndex = zeroIndices[i];
				
		// Update the index sum
		llvm::Value* pDimOffset = currentBuilder.CreateMul(pDimIndex, pOffset);
		pIndex = currentBuilder.CreateAdd(pIndex, pDimOffset);		
	}

	// If bounds checking is required
	if (s_jitNoWriteBoundChecks == false)
	{
		// If bounds checking is required for the last index
		if (version.pBoundsCheckInfo->isBoundsCheckRequired(pOrigExpr, indices.size() - 1, 0) == true ||
			version.pBoundsCheckInfo->isBoundsCheckRequired(pOrigExpr, indices.size() - 1, 1) == true)
		{
			// Load the number of matrix elements
			llvm::Value* pNumElems = loadMemberValue(
				currentBuilder,
				pMatrixObj,
				MEMBER_OFFSET(BaseMatrixObj, m_numElements),
				getIntType(sizeof(size_t))
			);
			pNumElems = currentBuilder.CreateIntCast(pNumElems, llvm::Type::getInt64Ty(*s_Context), false); 
			
			// Test if the final data index is out of bounds
			llvm::Value* pCompVal = currentBuilder.CreateICmpUGE(pIndex, pNumElems);				
							
			// Create a basic block for the non out of bounds case
			llvm::BasicBlock* pPassBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);		
			
			// Branch based on the test condition
			currentBuilder.CreateCondBr(pCompVal, pOutBlock, pPassBlock);
					
			// Update the current basic block
			currentBuilder.SetInsertPoint(pPassBlock);
		}
		
		// For each index (excluding the last)
		for (size_t i = 0; i < zeroIndices.size() - 1; ++i)
		{
			// If no bounds checking is required for this index, skip it
			if (version.pBoundsCheckInfo->isBoundsCheckRequired(pOrigExpr, i, 0) == false &&
				version.pBoundsCheckInfo->isBoundsCheckRequired(pOrigExpr, i, 1) == false)
				continue;
			
			// Get the index along the current dimension
			llvm::Value* pDimIndex = zeroIndices[i];
			
			// Get the size of the current dimension
			llvm::Value* pDimSize = dimSizes[i];
			
			// Test if the index data is out of bounds
			llvm::Value* pCompVal = currentBuilder.CreateICmpUGE(pDimIndex, pDimSize);
			
			// Create a basic block for the sufficient size case
			llvm::BasicBlock* pPassBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
			
			// Branch based on the test condition
			currentBuilder.CreateCondBr(pCompVal, pOutBlock, pPassBlock);
			
			// Update the current basic block
			currentBuilder.SetInsertPoint(pPassBlock);
		}
		
		// Create a basic block for the negative index case
		llvm::BasicBlock* pFailBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
		llvm::IRBuilder<> failBuilder(pFailBlock);
		
		// In the fail case, throw an exception
		createThrowError(
			failBuilder,
			"negative index in matrix read",
			pOrigExpr
		);
		
		// Terminate the fail block
		failBuilder.CreateRetVoid();
		
		// For each index
		for (size_t i = 0; i < indices.size(); ++i)
		{
			// Get the index along the current dimension
			llvm::Value* pDimIndex = zeroIndices[i];
						
			// Test if the index data is out of bounds
			llvm::Value* pCompVal = outBuilder.CreateICmpSLE(pDimIndex, llvm::ConstantInt::get(llvm::Type::getInt64Ty(*s_Context), 0));
			
			// Create a basic block for the positive case
			llvm::BasicBlock* pPassBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
			
			// Branch based on the test condition
			outBuilder.CreateCondBr(pCompVal, pFailBlock, pPassBlock);
			
			// Update the current out of bounds basic block
			outBuilder.SetInsertPoint(pPassBlock);
		}
		
		// Allocate an array to store the indices
		llvm::Value* pIndexArray = outBuilder.CreateAlloca(
			getIntType(sizeof(size_t)),
			llvm::ConstantInt::get(
				llvm::Type::getInt32Ty(*s_Context), 
				indices.size()
			)
		);
		
		// For each index
		for (size_t i = 0; i < indices.size(); ++i)
		{
			// Get the index along the current dimension
			llvm::Value* pDimIndex = indices[i];
			
			// Convert the index to the size_t type
			pDimIndex = outBuilder.CreateIntCast(pDimIndex, getIntType(sizeof(size_t)), false);
			
			// Store the value in the array
			llvm::Value* pValAddr = outBuilder.CreateGEP(pIndexArray, llvm::ConstantInt::get(llvm::Type::getInt64Ty(*s_Context), i));
			outBuilder.CreateStore(pDimIndex, pValAddr);
		}
		
		// Create a call to expand the matrix
		LLVMValueVector expandArgs;
		expandArgs.push_back(pMatrixObj);
		expandArgs.push_back(pIndexArray);
		expandArgs.push_back(llvm::ConstantInt::get(getIntType(sizeof(size_t)), indices.size()));
		createNativeCall(
			outBuilder,
			(void*)BaseMatrixObj::expandMatrix,
			expandArgs
		);		
		
		// Clear the matrix dimensions
		dimSizes.clear();
		
		// For each index (except the last)
		for (size_t i = 0; i < indices.size() - 1; ++i)
		{
			// Extract the size of the current dimension
			llvm::Value* pDimElemPtr = outBuilder.CreateGEP(pSizeArrayPtr, llvm::ConstantInt::get(llvm::Type::getInt64Ty(*s_Context), i));
			llvm::Value* pDimSize = outBuilder.CreateLoad(pDimElemPtr);
			pDimSize = outBuilder.CreateIntCast(pDimSize, llvm::Type::getInt64Ty(*s_Context), false);		
			
			// Add the dimension size to the list
			dimSizes.push_back(pDimSize);
		}
		
		// Initialize the new index computation
		llvm::Value* pNewIndex = zeroIndices[0];
		
		// Initialize the offset product
		llvm::Value* pOffset = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*s_Context), 1);
		
		// For each index (excluding the first)
		for (size_t i = 1; i < zeroIndices.size(); ++i)
		{
			// Get the size of the previous dimension
			llvm::Value* pDimSize = dimSizes[i-1];
			
			// Update the offset product
			pOffset = outBuilder.CreateMul(pOffset, pDimSize);

			// Get the index along the current dimension
			llvm::Value* pDimIndex = zeroIndices[i];
					
			// Update the index sum
			llvm::Value* pDimOffset = outBuilder.CreateMul(pDimIndex, pOffset);
			pNewIndex = outBuilder.CreateAdd(pNewIndex, pDimOffset);		
		}
		
		// Create a basic block for the resolution
		llvm::BasicBlock* pResBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
		llvm::IRBuilder<> resBuilder(pResBlock);
		
		// Have the non-expansion case branch to the resolution block
		currentBuilder.CreateBr(pResBlock);
		
		// Have the matrix expansion case branch to the resolution block
		outBuilder.CreateBr(pResBlock);
		
		// Create a phi node for the computed indices
		llvm::PHINode* pIndexPhi = resBuilder.CreatePHI(llvm::Type::getInt64Ty(*s_Context), 2);
		
		// Add incoming values for the indices
		pIndexPhi->addIncoming(pIndex, currentBuilder.GetInsertBlock());
		pIndexPhi->addIncoming(pNewIndex, outBuilder.GetInsertBlock());
		
		// Replace the original index value by the phi node
		pIndex = pIndexPhi;
		
		// Make the resolution block the new current basic block
		currentBuilder.SetInsertPoint(pResBlock);
	}
	else
	{
		// Terminate the out of bounds block
		outBuilder.CreateRetVoid();
	}
	
	//std::cout << "Bounds checking complete" << std::endl;

	// Switch based on the matrix type
	switch (matrixType)
	{
		// Floating-point matrix
		case DataObject::MATRIX_F64:
		{
			// Load the element array pointer
			llvm::Value* pDataPtr = loadMemberValue(
				currentBuilder,
				pMatrixObj,
				MEMBER_OFFSET(MatrixF64Obj, m_pElements),
				llvm::PointerType::getUnqual(llvm::Type::getDoubleTy(*s_Context))
			);
			
			// Compute the address of the value
			llvm::Value* pValAddr = currentBuilder.CreateGEP(pDataPtr, pIndex);
			
			// Set the storage mode of the right-expression to the float64 type
			pValue = changeStorageMode(
				currentBuilder,
				pValue,
				valueType,
				llvm::Type::getDoubleTy(*s_Context)
			);
			
			// Store the value in the array
			currentBuilder.CreateStore(pValue, pValAddr);
		}
		break;

		// Character array
		case DataObject::CHARARRAY:
		{
			// Load the element array pointer
			llvm::Value* pDataPtr = loadMemberValue(
				currentBuilder,
				pMatrixObj,
				MEMBER_OFFSET(CharArrayObj, m_pElements),
				llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(*s_Context))
			);
			
			// Compute the address of the value
			llvm::Value* pValAddr = currentBuilder.CreateGEP(pDataPtr, pIndex);
			
			// Set the storage mode of the right-expression to the int64 type
			pValue = changeStorageMode(
				currentBuilder,
				pValue,
				valueType,
				llvm::Type::getInt64Ty(*s_Context)
			);
			
			// Convert the value to the int8 type (char)
			pValue = currentBuilder.CreateIntCast(pValue, llvm::Type::getInt8Ty(*s_Context), true);

			// Store the value in the array
			currentBuilder.CreateStore(pValue, pValAddr);
		}
		break;
		
		// Logical array array
		case DataObject::LOGICALARRAY:
		{
			// Load the element array pointer
			llvm::Value* pDataPtr = loadMemberValue(
				currentBuilder,
				pMatrixObj,
				MEMBER_OFFSET(LogicalArrayObj, m_pElements),
				llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(*s_Context))
			);
			
			// Compute the address of the value
			llvm::Value* pValAddr = currentBuilder.CreateGEP(pDataPtr, pIndex);
			
			// Set the storage mode of the right-expression to the int64 type
			pValue = changeStorageMode(
				currentBuilder,
				pValue,
				valueType,
				llvm::Type::getInt64Ty(*s_Context)
			);
			
			// Convert the value to the int8 type (bool)
			pValue = currentBuilder.CreateIntCast(pValue, llvm::Type::getInt8Ty(*s_Context), false);
			
			// Store the value in the array
			currentBuilder.CreateStore(pValue, pValAddr);
		}
		break;
	
		// For any other matrix type
		default:
		{
			// Throw a compilation error
			throw CompError("unsupported matrix type in scalar write \"" + DataObject::getTypeName(matrixType) + "\"");
		}
	}
	
	// Branch to the exit block
	currentBuilder.CreateBr(pExitBlock);
}

/***************************************************************
* Function: JITCompiler::arrayExprFallback()
* Purpose : Compile an array-returning evaluation fallback
* Initial : Maxime Chevalier-Boisvert on June 16, 2009
****************************************************************
Revisions and bug fixes:
*/
JITCompiler::ValueVector JITCompiler::arrayExprFallback(
	Expression* pExpression,
	void* pEvalFunc,
	size_t nargout,
	CompFunction& function,
	CompVersion& version,
	const Expression::SymbolSet& liveVars,
	const VarDefMap& reachDefs,
	const VarTypeMap& varTypes,
	VariableMap& varMap,
	llvm::BasicBlock* pEntryBlock,
	llvm::BasicBlock* pExitBlock
)
{
	// If we are in verbose mode, log the array expression fallback
	if (ConfigManager::s_verboseVar)
		std::cout << "Compiling array expr fallback for: " << pExpression->toString() << std::endl;
	
	// Create an IR builder for the entry block
	llvm::IRBuilder<> entryBuilder(pEntryBlock);
	
	// Write the symbols used by the expression to the environment
	writeVariables(entryBuilder, function, version, varMap, pExpression->getSymbolUses());
	
	// Create a native call to evaluate the expresssion
	LLVMValueVector evalArgs;
	evalArgs.push_back(createPtrConst(pExpression));
	evalArgs.push_back(getCallEnv(function, version));
	evalArgs.push_back(llvm::ConstantInt::get(getIntType(sizeof(size_t)), nargout));
	llvm::Value* pValue = createNativeCall(
		entryBuilder,
		(void*)pEvalFunc,
		evalArgs
	);
	
	// Load the type of the returned value
	llvm::Value* pObjType = loadObjectType(
		entryBuilder,
		pValue
	);
	
	// Test if whether object is an array object or not
	llvm::Value* pCompVal = entryBuilder.CreateICmpEQ(pObjType, getObjType(DataObject::ARRAY));
	
	// Create a basic block for the array case
	llvm::BasicBlock* pArrayBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
	llvm::IRBuilder<> arrayBuilder(pArrayBlock);

	// Create a basic block for the array read exit point
	llvm::BasicBlock* pArrayExitBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
	llvm::IRBuilder<> arrayExitBuilder(pArrayExitBlock);
	
	// Create a basic block for the single value case
	llvm::BasicBlock* pSingleBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
	llvm::IRBuilder<> singleBuilder(pSingleBlock);
	
	// Create a basic block for the resolution
	llvm::BasicBlock* pResBlock = llvm::BasicBlock::Create(*s_Context, "", version.pLLVMFunc);
	llvm::IRBuilder<> resBuilder(pResBlock);
				
	// If the object is an array, branch to the array block
	entryBuilder.CreateCondBr(pCompVal, pArrayBlock, pSingleBlock);
	
	// Find the type set for this expression
	ExprTypeMap::const_iterator typeItr = version.pTypeInferInfo->exprTypeMap.find(pExpression);
	assert (typeItr != version.pTypeInferInfo->exprTypeMap.end());
	const TypeSetString& exprTypes = typeItr->second;
		
	// Create vectors for the storage modes and object types of the values
	LLVMTypeVector storeModes(nargout, VOID_PTR_TYPE);
	std::vector<DataObject::Type> objTypes(nargout, DataObject::UNKNOWN);
	
	// If type information about the values is available
	if (exprTypes.size() >= nargout)
	{
		// For each value
		for (size_t i = 0; i < nargout; ++i)
		{
			// Get the optimal storage mode for the value
			storeModes[i] = getStorageMode(
				exprTypes[i],
				objTypes[i]
			);
		}
	}
	
	// Extract the array element values
	ValueVector arrayValues = getArrayValues(
		pValue,
		nargout,
		true,
		"insufficient number of return values",
		pExpression,
		storeModes,
		objTypes,
		version.pLLVMFunc,
		pArrayBlock,
		pArrayExitBlock
	);
	
	// Branch to the resolution block
	arrayExitBuilder.CreateBr(pResBlock);
	
	// If the number of output arguments is greater than 1
	if (nargout > 1)
	{
		// Throw an exception along the single path
		createThrowError(
			singleBuilder,
			"the expression only produces one output",
			pExpression
		);

		// Terminate this basic block
		singleBuilder.CreateRetVoid();
	} 
	else
	{
		// If the number of return arguments is 1
		if (nargout == 1)
		{
			// Change the value's storage mode
			pValue = changeStorageMode(
				singleBuilder,
				pValue,
				objTypes[0],
				storeModes[0]
			);
			
			// Create a phi node for the value
			llvm::PHINode* pPhiNode = resBuilder.CreatePHI(storeModes[0], 2);
			
			// Add the incoming values for both cases
			pPhiNode->addIncoming(arrayValues[0].pValue, pArrayExitBlock);
			pPhiNode->addIncoming(pValue, pSingleBlock);
			
			// Store the phi node as the first value
			arrayValues[0].pValue = pPhiNode;
		}
		
		// Branch to the resolution block
		singleBuilder.CreateBr(pResBlock);
	}
	
	// Branch to the exit block
	resBuilder.CreateBr(pExitBlock);
	
	//std::cout << "Completed array expr fallback code gen" << std::endl;
	
	// Return the array of values
	return arrayValues;
}

/***************************************************************
* Function: JITCompiler::exprFallback()
* Purpose : Generate fallback code for an expression evaluation
* Initial : Maxime Chevalier-Boisvert on March 25, 2009
****************************************************************
Revisions and bug fixes:
*/
JITCompiler::Value JITCompiler::exprFallback(
	Expression* pExpression,
	void* pInterpFunc,
	CompFunction& function,
	CompVersion& version,
	const Expression::SymbolSet& liveVars,
	const VarDefMap& reachDefs,
	const VarTypeMap& varTypes,
	VariableMap& varMap,
	llvm::BasicBlock* pEntryBlock,
	llvm::BasicBlock* pExitBlock
)
{
	// If we are in verbose mode, log the expression fallback
	if (ConfigManager::s_verboseVar)
		std::cout << "Generating expr fallback for: \"" << pExpression->toString() << "\"" << std::endl;
	
	// Create an IR builder for the entry block
	llvm::IRBuilder<> entryBuilder(pEntryBlock);
	
	// Write the symbols used by the expression to the environment
	writeVariables(entryBuilder, function, version, varMap, pExpression->getSymbolUses());
	
	// Generate the arguments for the native call
	LLVMValueVector evalArgs;
	evalArgs.push_back(createPtrConst(pExpression));
	evalArgs.push_back(getCallEnv(function, version));
	
	// Call the interpreter function
	llvm::Value* pValue = createNativeCall(
		entryBuilder,
		pInterpFunc,
		evalArgs
	);
	
	// Find the type set for this expression
	ExprTypeMap::const_iterator typeItr = version.pTypeInferInfo->exprTypeMap.find(pExpression);
	TypeSet exprTypes = (typeItr != version.pTypeInferInfo->exprTypeMap.end() && !typeItr->second.empty())? typeItr->second[0]:TypeSet();
	
	// Get the optimal storage mode for the value
	DataObject::Type objectType;
	llvm::Type* storageMode = getStorageMode(
		exprTypes,
		objectType
	);
	
	// If the storage mode of the value does not match
	if (pValue->getType() != storageMode)
	{
		// Change the read value's storage mode
		pValue = changeStorageMode(
			entryBuilder,
			pValue,
			objectType,
			storageMode
		);
	}
	
	// Link the entry block to the exit block
	entryBuilder.CreateBr(pExitBlock);		
	
	// Return the expression value
	return Value(pValue, objectType);
}

/***************************************************************
* Function: JITCompiler::getArrayValues()
* Purpose : Read values from an array object
* Initial : Maxime Chevalier-Boisvert on June 15, 2009
****************************************************************
Revisions and bug fixes:
*/
JITCompiler::ValueVector JITCompiler::getArrayValues(
	llvm::Value* pArrayObj,
	size_t numValues,
	bool testNumVals,
	const char* pErrorText,
	const IIRNode* pErrorNode,
	const LLVMTypeVector& objStoreModes,
	const std::vector<DataObject::Type>& objTypes,
	llvm::Function* pLLVMFunc,
	llvm::BasicBlock* pEntryBlock,
	llvm::BasicBlock* pExitBlock
)
{
	// If the number of values should be tested
	if (testNumVals && numValues > 0)
	{
		// Create an IR builder for the entry block
		llvm::IRBuilder<> entryBuilder(pEntryBlock);

		// Create a native call to get the array size
		llvm::Value* pSizeVal = createNativeCall(
			entryBuilder,
			(void*)ArrayObj::getArraySize,
			LLVMValueVector(1, pArrayObj)
		);
		
		// Create a basic block to replace the current entry block
		llvm::BasicBlock* pNewEntryBlock = llvm::BasicBlock::Create(*s_Context, "", pLLVMFunc);
		
		// Create a basic block for the insufficient size case
		llvm::BasicBlock* pFailBlock = llvm::BasicBlock::Create(*s_Context, "", pLLVMFunc);
		llvm::IRBuilder<> failBuilder(pFailBlock);
		
		// Test whether or not the size is less than the requested number of values
		llvm::Value* pCompVal = entryBuilder.CreateICmpSLT(
			pSizeVal, 
			llvm::ConstantInt::get(
				getIntType(sizeof(size_t)),
				numValues
			)
		);
		
		// If the value is true, branch to the fail block, otherwise to the new entry block
		entryBuilder.CreateCondBr(pCompVal, pFailBlock, pNewEntryBlock);
		
		// In the fail block, throw an exception
		createThrowError(
			failBuilder,
			pErrorText,
			pErrorNode
		);
		
		// Terminate the basic block to keep the CFG proper
		failBuilder.CreateRetVoid();
		
		// Update the entry block pointer
		pEntryBlock = pNewEntryBlock;		
	}
	
	// Create an IR builder for the entry block
	llvm::IRBuilder<> entryBuilder(pEntryBlock);
	
	// Create a vector to store the values
	ValueVector valueVector;

	//std::cout << "Number of values: " << numValues << std::endl;
	
	// For each value
	for (size_t i = 0; i < numValues; ++i)
	{
		// Create the call to get the ith object from the array
		LLVMValueVector getArgs;
		getArgs.push_back(pArrayObj);
		getArgs.push_back(llvm::ConstantInt::get(getIntType(sizeof(size_t)), i));
		llvm::Value* pObjPtr = createNativeCall(
			entryBuilder,
			(void*)ArrayObj::getArrayObj,
			getArgs
		);
		
		// Convert the storage mode to the preffered type
		llvm::Value* pValue = changeStorageMode(
			entryBuilder,
			pObjPtr,
			objTypes[i],
			objStoreModes[i]
		);
		
		// Add the value to the vector
		valueVector.push_back(Value(pValue, objTypes[i]));
	}
	
	// Branch to the exit block
	entryBuilder.CreateBr(pExitBlock);
	
	// Return the value vector
	return valueVector;	
}

/***************************************************************
* Function: JITCompiler::createThrowError()
* Purpose : Create a call throwing a runtime error
* Initial : Maxime Chevalier-Boisvert on June 15, 2009
****************************************************************
Revisions and bug fixes:
*/
void JITCompiler::createThrowError(
	llvm::IRBuilder<>& irBuilder,
	const char* pErrorText,
	const IIRNode* pErrorNode
)
{
	// Create a vector to store the call arguments
	LLVMValueVector arguments;
	
	// Add the text buffer pointer and the IIR node pointer to the arguments
	arguments.push_back(createPtrConst(pErrorText));
	arguments.push_back(createPtrConst(pErrorNode));
	
	// Create the native call to throw the exception
	createNativeCall(
		irBuilder,
		(void*)RunError::throwError,
		arguments
	);
}

/***************************************************************
* Function: JITCompiler::loadObjectType()
* Purpose : Load the type of an object from memory
* Initial : Maxime Chevalier-Boisvert on March 25, 2009
****************************************************************
Revisions and bug fixes:
*/
llvm::LoadInst* JITCompiler::loadObjectType(
	llvm::IRBuilder<>& irBuilder,
	llvm::Value* pObjPointer
)
{
	// Load and return the type value 
	return loadMemberValue(
		irBuilder,
		pObjPointer,
		MEMBER_OFFSET(DataObject, m_type),
		getIntType(sizeof(DataObject::Type))
	);
}

/***************************************************************
* Function: JITCompiler::loadMemberValue()
* Purpose : Load a member value from memory
* Initial : Maxime Chevalier-Boisvert on July 14, 2009
****************************************************************
Revisions and bug fixes:
*/
llvm::LoadInst* JITCompiler::loadMemberValue(
	llvm::IRBuilder<>& irBuilder,
	llvm::Value* pObjPointer,
	size_t valOffset,
	llvm::Type* valType
)
{	
	// Add the offset to the object base pointer
	llvm::Value* pTypeAddr = irBuilder.CreateGEP(
		pObjPointer, 
		llvm::ConstantInt::get(
			getIntType(PLATFORM_POINTER_SIZE),
			valOffset
		)
	);
	
	// Cast the type address pointer to the right value size for this platform
	llvm::Value* pTypePtr = irBuilder.CreateCast(
		llvm::Instruction::BitCast,
		pTypeAddr,
		llvm::PointerType::getUnqual(valType)
	);
	
	// Read the type value from memory
	llvm::LoadInst* pTypeVal = irBuilder.CreateLoad(pTypePtr);

	// Return the type value
	return pTypeVal;	
}

/***************************************************************
* Function: JITCompiler::createNativeCall()
* Purpose : Create a native function call
* Initial : Maxime Chevalier-Boisvert on March 9, 2009
****************************************************************
Revisions and bug fixes:
*/
llvm::CallInst* JITCompiler::createNativeCall(
	llvm::IRBuilder<>& irBuilder,
	void* pNativeFunc,
	const LLVMValueVector& arguments
)
{
	// Ensure the function was registered
	assert (s_nativeMap.find(pNativeFunc) != s_nativeMap.end());
	
	// Get a reference to the native function object
	const NativeFunc& nativeFunc = s_nativeMap[pNativeFunc];
	
	// If we are in verbose mode
	if (ConfigManager::s_verboseVar)
	{
		// Log the native call creation
		std::cout << "Creating native call to: " << nativeFunc.name << std::endl;
	
		// Log the argument types
		for (size_t i = 0; i < arguments.size(); ++i) {
          std::cout << "Arg #" << (i+1) << " ";
          arguments[i]->getType()->dump(); 
        }
	}
	
	// Create the call
	llvm::CallInst* pValue = 
      irBuilder.CreateCall(nativeFunc.pLLVMFunc, arguments);
	
	// Return the output of the call
	return pValue;
}

/***************************************************************
* Function: JITCompiler::createPtrConst()
* Purpose : Compile a pointer constant
* Initial : Maxime Chevalier-Boisvert on March 9, 2009
****************************************************************
Revisions and bug fixes:
*/
llvm::Constant* JITCompiler::createPtrConst(const void* pPointer, llvm::Type* valType)
{
	// Get the integer type matching the size of pointers on this platform
	llvm::Type* intType = getIntType(PLATFORM_POINTER_SIZE);
		
	// Create a constant integer from the pointer value
	llvm::Constant* constInt = llvm::ConstantInt::get(intType, (int64)pPointer);

	// Cast the integer into a pointer to the desired type
	llvm::Constant* constPtr = llvm::ConstantExpr::getIntToPtr(
		constInt,
		llvm::PointerType::getUnqual(valType)
	);
		
	// Return the pointer constant
	return constPtr;
}

/***************************************************************
* Function: JITCompiler::getIntType()
* Purpose : Get the integer type for a given size in bytes
* Initial : Maxime Chevalier-Boisvert on March 25, 2009
****************************************************************
Revisions and bug fixes:
*/
llvm::Type* JITCompiler::getIntType(size_t sizeInBytes)
{
	// Switch on the input size
	switch (sizeInBytes)
	{
		// Return the appropriate integer type
		case 1: return llvm::Type::getInt8Ty(*s_Context);
		case 2: return llvm::Type::getInt16Ty(*s_Context);
		case 4: return llvm::Type::getInt32Ty(*s_Context);
		case 8: return llvm::Type::getInt64Ty(*s_Context);
		
		// Invalid integer sizes
		default: assert (false);
	}
}

/***************************************************************
* Function: JITCompiler::getObjType()
* Purpose : Get the constant corresponding to an object type
* Initial : Maxime Chevalier-Boisvert on June 2, 2009
****************************************************************
Revisions and bug fixes:
*/
llvm::Constant* JITCompiler::getObjType(DataObject::Type objType)
{
	// Get the integer constant for the object type
	return llvm::ConstantInt::get(getIntType(sizeof(DataObject::Type)), objType);	
}
