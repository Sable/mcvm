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

// Include guards
#ifndef JITCOMPILER_H_
#define JITCOMPILER_H_

// Header files
#include <ext/hash_map>
#include <vector>
#include <map>
#include <llvm/LLVMContext.h>
#include <llvm/Function.h>
#include <llvm/PassManager.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/IRBuilder.h>
#include <llvm/Support/TargetSelect.h>
#include "configmanager.h"
#include "iir.h"
#include "platform.h"
#include "functions.h"
#include "exprstmt.h"
#include "assignstmt.h"
#include "ifelsestmt.h"
#include "loopstmts.h"
#include "constexprs.h"
#include "binaryopexpr.h"
#include "paramexpr.h"
#include "typeinfer.h"
#include "analysis_reachdefs.h"
#include "analysis_livevars.h"
#include "analysis_typeinfer.h"
#include "analysis_metrics.h"
#include "analysis_boundscheck.h"
#include "analysis_copyplacement.h"

/***************************************************************
* Class   : CompError
* Purpose : Exception class to represent a compilation error
* Initial : Maxime Chevalier-Boisvert on March 6, 2009
****************************************************************
Revisions and bug fixes:
*/
class CompError
{
public:
	
	// Constructor
	CompError(const std::string& errorText, IIRNode* pNode = NULL)
	: m_errorText(errorText), m_pErrorNode(pNode) {}
	
	// Method to obtain a string representation of the error
	std::string toString() const;
	
	// Accessor to get the error text
	const std::string& getErrorText() const { return m_errorText; }
	
	// Accessor to get the erroneous node
	IIRNode* getErrorNode() const { return m_pErrorNode; } 
	
private:
	
	// Error description text
	std::string m_errorText;
	
	// Erroneous node
	IIRNode* m_pErrorNode;
};

/***************************************************************
* Class   : JITCompiler
* Purpose : Compile IIR code into binary executable form
* Initial : Maxime Chevalier-Boisvert on March 4, 2009
****************************************************************
Revisions and bug fixes:
*/
class JITCompiler
{
public:
	
	// LLVM type vector type definition
	typedef std::vector<llvm::Type*> LLVMTypeVector;
	
	// LLVM value vector type definition
	typedef std::vector<llvm::Value*> LLVMValueVector;
	
	// LLVM value pair type definition
	typedef std::pair<llvm::Value*, llvm::Value*> LLVMValuePair;
	
	// Void pointer type constant
	static llvm::Type* VOID_PTR_TYPE;
	
	// Method to initialize the JIT compiler
	static void initialize();
	
	// Method to shut down the JIT compiler
	static void shutdown();

    // Method to initialize osr facility	
	static void initializeOSR();

	// Method to register a native function
	static void regNativeFunc(
		const std::string& name,
		void* pFuncPtr,
		llvm::Type* returnType,
		const LLVMTypeVector& argTypes,
		bool noMemWrites = false,
		bool noMemAccess = false,
		bool noThrows = false
	);
	
	// Method to register an optimized library function
	static void regLibraryFunc(
		const LibFunction* pLibFunc,
		void* pFuncPtr,
		const TypeSetString& inputTypes,
		const TypeSet& returnType,
		bool noMemWrites = false,
		bool noMemAccess = false,
		bool noThrows = false		
	);
	
	// Method to compile a program function given argument types
	static void compileFunction(ProgFunction* pFunction, const TypeSetString& argTypeStr);
	
	// Method to call a JIT-compiled version of a function
	static ArrayObj* callFunction(ProgFunction* pFunction, ArrayObj* pArguments, size_t outArgCount);
	
	// Config variable to enable/disable the JIT compiler
	static ConfigVar s_jitEnableVar;

	// Config variables to enable/disable specific JIT optimizations
	static ConfigVar s_jitUseArrayOpts;
	static ConfigVar s_jitUseBinOpOpts;
	static ConfigVar s_jitUseLibOpts;
	static ConfigVar s_jitUseDirectCalls;
	
	// Config variables to disable matrix read/write bounds checking
	static ConfigVar s_jitNoReadBoundChecks;
	static ConfigVar s_jitNoWriteBoundChecks;

	// Config variable for enabling or disabling copy optimizations	
	static ConfigVar s_jitCopyEnableVar;

    // Config variables to enable/disable on-stack replacement
    static ConfigVar s_jitOsrEnableVar;
    static ConfigVar s_jitOsrStrategyVar;

private:
	
	// Variable value class
	class Value
	{
	public:
		
		// Full constructor
		Value(llvm::Value* pVal, DataObject::Type type)
		: pValue(pVal), objType(type) {}
		
		// Default constructor
		Value()
		: pValue(NULL), objType(DataObject::UNKNOWN) {}
		
		// Variable value (null if env-stored)
		llvm::Value* pValue;
		
		// Actual object type
		DataObject::Type objType;
	};
	
	// Value vector type definition
	typedef std::vector<Value> ValueVector;
	
	// Variable map type definition
	typedef __gnu_cxx::hash_map<SymbolExpr*, Value, IntHashFunc<SymbolExpr*>, __gnu_cxx::equal_to<SymbolExpr*> > VariableMap;
	
	// Branching point type definition
	typedef std::pair<llvm::BasicBlock*, VariableMap> BranchPoint;
	
	// Branching point list type definition
	typedef std::vector<BranchPoint> BranchList;
	
	// Function set type definition
	typedef std::set<Function*, std::less<Function*>, gc_allocator<Function> > FunctionSet;
		
	// Native function structure
	struct NativeFunc
	{
		// Function name
		std::string name;
		
		// Return type
		llvm::Type* returnType;
		
		// Input argument types
		LLVMTypeVector argTypes;
		
		// LLVM function object
		llvm::Function* pLLVMFunc;
	};

	// Native function map type definition
	typedef std::map<void*, NativeFunc> NativeMap;
	
	// Optimized library function key class
	class LibFuncKey
	{
	public:
		
		// Constructor
		LibFuncKey(const LibFunction* pFunc, const TypeSetString& inTypes, const TypeSet& retType)
		: pLibFunc(pFunc), inputTypes(inTypes), returnType(retType) {}
		
		// Less-than comparison operator (for sorting)
		bool operator < (const LibFuncKey& other) const
		{
			// Compare the key elements
			if 		(pLibFunc < other.pLibFunc) 	return true;
			else if	(pLibFunc > other.pLibFunc)		return false;
			else if	(inputTypes < other.inputTypes)	return true;
			else if	(inputTypes > other.inputTypes)	return false;
			else if	(returnType < other.returnType)	return true;
			else	return false;
		}
		
		// Library function
		const LibFunction* pLibFunc;
				
		// Input types
		TypeSetString inputTypes;
		
		// Return type
		TypeSet returnType;
	};
	
	// Optimized library function map type definition
	typedef std::map<LibFuncKey, void*, std::less<LibFuncKey>, gc_allocator<std::pair<LibFuncKey, void*> > > LibFuncMap;
	
	// Compiled program function pointer type definition
	typedef void (*COMP_FUNC_PTR)(byte* pInStruct, byte* pOutStruct);
	
	// Compiled wrapper function pointer type definition
	typedef ArrayObj* (*WRAPPER_FUNC_PTR)(ArrayObj* pArgs, int64 outArgCount);
	
	// Compiled function version structure
	struct CompVersion
	{		
		CompVersion():pOutStructType(NULL), pEnvObject(NULL), pInStruct(NULL), pOutStruct(NULL),
		inStructSize(0), outStructSize(0), pFuncPtr(NULL), pWrapperPtr(NULL){}
		
		// Input argument types
		TypeSetString inArgTypes;
				
		// Reaching definition information
		const ReachDefInfo* pReachDefInfo;

		// Live variable information
		const LiveVarInfo* pLiveVarInfo;
		
		// Type inference information
		const TypeInferInfo* pTypeInferInfo;
		
		// Code metrics information
		const MetricsInfo* pMetricsInfo;
		
		// Bounds Checking elimination analysis information
		const BoundsCheckInfo* pBoundsCheckInfo;
		
		// Array copy analysis Information 
		const ArrayCopyAnalysisInfo* pArrayCopyInfo;
		
		// Input argument storage modes and object types
		LLVMTypeVector inArgStoreModes;
		std::vector<DataObject::Type> inArgObjTypes;

		// Type of the input data structure
		llvm::StructType* pInStructType;
		
		// Output argument storage modes and object types
		LLVMTypeVector outArgStoreModes;
		std::vector<DataObject::Type> outArgObjTypes;
		
		// Type of the return data structure
		llvm::StructType* pOutStructType;
		
		// Entry basic block for the function
		llvm::BasicBlock* pEntryBlock;
		
		// Environment object pointer (NULL if not used)
		llvm::Value* pEnvObject;
		
		// Call in/out data structs (NULL if not used)
		llvm::Value* pInStruct;
		llvm::Value* pOutStruct;
		
		// Maximum call in/out data struct sizes encountered
		size_t inStructSize;
		size_t outStructSize;
		
		// LLVM function object
		llvm::Function* pLLVMFunc;
		
		// LLVM wrapper function object
		llvm::Function* pLLVMWrapper;

		// Pointer to the compiled program function code
		COMP_FUNC_PTR pFuncPtr;
		
		// Pointer to the compiled wrapper function code
		WRAPPER_FUNC_PTR pWrapperPtr;
	};
	
	// Compiled version map type definition
	typedef std::map<TypeSetString, CompVersion, std::less<TypeSetString>, gc_allocator<std::pair<TypeSetString, CompVersion> > > VersionMap;
	
	// Compiled function structure
	struct CompFunction
	{
		// Program function object
		ProgFunction* pProgFunc;
		
		// IIR nodes for the function body
		StmtSequence* pFuncBody;
		
		// List of established callees
		FunctionSet callees;
		
		// Compiled function versions
		VersionMap versions;
	};
	
	// Function map type definition
	typedef std::map<ProgFunction*, CompFunction, std::less<ProgFunction*>, gc_allocator<std::pair<ProgFunction*, CompFunction> > > FunctionMap;
	
	// Binary operator factory function type definition
	typedef llvm::Value* (*BINOP_FACTORY_FUNC)(llvm::IRBuilder<>& builder, llvm::Value* pLVal, llvm::Value* pRVal);
	
	// Methods to create binary LLVM instructions
	static llvm::Value* createAddInstr(llvm::IRBuilder<>& builder, llvm::Value* pLVal, llvm::Value* pRVal);
	static llvm::Value* createSubInstr(llvm::IRBuilder<>& builder, llvm::Value* pLVal, llvm::Value* pRVal);
	static llvm::Value* createMulInstr(llvm::IRBuilder<>& builder, llvm::Value* pLVal, llvm::Value* pRVal);
	static llvm::Value* createOrInstr(llvm::IRBuilder<>& builder, llvm::Value* pLVal, llvm::Value* pRVal) { return builder.CreateOr(pLVal, pRVal); }
	static llvm::Value* createAndInstr(llvm::IRBuilder<>& builder, llvm::Value* pLVal, llvm::Value* pRVal) { return builder.CreateAnd(pLVal, pRVal); }
	static llvm::Value* createICmpEQInstr(llvm::IRBuilder<>& builder, llvm::Value* pLVal, llvm::Value* pRVal) { return builder.CreateICmpEQ(pLVal, pRVal); }
	static llvm::Value* createFCmpOEQInstr(llvm::IRBuilder<>& builder, llvm::Value* pLVal, llvm::Value* pRVal) { return builder.CreateFCmpOEQ(pLVal, pRVal); }
	static llvm::Value* createICmpSGTInstr(llvm::IRBuilder<>& builder, llvm::Value* pLVal, llvm::Value* pRVal) { return builder.CreateICmpSGT(pLVal, pRVal); }
	static llvm::Value* createFCmpOGTInstr(llvm::IRBuilder<>& builder, llvm::Value* pLVal, llvm::Value* pRVal) { return builder.CreateFCmpOGT(pLVal, pRVal); }
	static llvm::Value* createICmpSLTInstr(llvm::IRBuilder<>& builder, llvm::Value* pLVal, llvm::Value* pRVal) { return builder.CreateICmpSLT(pLVal, pRVal); }
	static llvm::Value* createFCmpOLTInstr(llvm::IRBuilder<>& builder, llvm::Value* pLVal, llvm::Value* pRVal) { return builder.CreateFCmpOLT(pLVal, pRVal); }
	static llvm::Value* createICmpSGEInstr(llvm::IRBuilder<>& builder, llvm::Value* pLVal, llvm::Value* pRVal) { return builder.CreateICmpSGE(pLVal, pRVal); }
	static llvm::Value* createFCmpOGEInstr(llvm::IRBuilder<>& builder, llvm::Value* pLVal, llvm::Value* pRVal) { return builder.CreateFCmpOGE(pLVal, pRVal); }
	static llvm::Value* createICmpSLEInstr(llvm::IRBuilder<>& builder, llvm::Value* pLVal, llvm::Value* pRVal) { return builder.CreateICmpSLE(pLVal, pRVal); }
	static llvm::Value* createFCmpOLEInstr(llvm::IRBuilder<>& builder, llvm::Value* pLVal, llvm::Value* pRVal) { return builder.CreateFCmpOLE(pLVal, pRVal); }
	
	// Method to handle exceptions during function calls
	static void callExceptHandler(
		ProgFunction* pFunction,
		COMP_FUNC_PTR pFuncPtr,
		byte* pInStruct,
		byte* pOutStruct
	);
	
	// Method to create/get the calling environment for a function
	static llvm::Value* getCallEnv(
		CompFunction& function,
		CompVersion& version
	);
	
	// Method to create/get the call data structures for a function
	static LLVMValuePair getCallStructs(
		CompFunction& callerFunction,
		CompVersion& callerVersion,
		CompFunction& calleeFunction,
		CompVersion& calleeVersion
	);
	
	// Method to match branching point variable mappings
	static VariableMap matchBranchPoints(
		CompFunction& function,
		CompVersion& version,
		const Expression::SymbolSet& liveVars,
		const VarTypeMap& varTypes,
		const BranchList& branchPoints,
		llvm::BasicBlock* pDestBlock
	);
	
	// Method to write variables to the environment
	static void writeVariables(
		llvm::IRBuilder<>& irBuilder,
		CompFunction& function,
		CompVersion& version,
		VariableMap& varMap,
		const Expression::SymbolSet& variables
	);

	// Method to write a single variable to the environment
	static void writeVariable(
		llvm::IRBuilder<>& irBuilder,
		CompFunction& function,
		CompVersion& version,
		SymbolExpr* pSymbol,
		const Value& value
	);
	
	// Method to read variables to the environment
	static void readVariables(
		llvm::IRBuilder<>& irBuilder,
		CompFunction& function,
		CompVersion& version,
		const VarTypeMap& varTypes,
		VariableMap& varMap,
		const Expression::SymbolSet& variables
	);

	// Method to read a single variable from the environment
	static Value readVariable(
		llvm::IRBuilder<>& irBuilder,
		CompFunction& function,
		CompVersion& version,
		const TypeSet& typeSet,
		SymbolExpr* pSymbol,
		bool safeRead
	);
	
	// Method to mark variables as written in the environment
	static void markVarsWritten(
		VariableMap& varMap,
		const Expression::SymbolSet& variables
	);
	
	// Method to print the contents of a variable map
	static void printVarMap(
		const VariableMap& varMap
	);
	
	// Method to get the storage mode for a given type set
	static llvm::Type* getStorageMode(
		const TypeSet& typeSet,
		DataObject::Type& objType
	);
	
	// Method to get the widest storage mode among two options
	static llvm::Type* widestStorageMode(
		llvm::Type* modeA,
		llvm::Type* modeB
	);
	
	// Method to change the storage mode of a value
	static llvm::Value* changeStorageMode(
		llvm::IRBuilder<>& irBuilder,
		llvm::Value* pCurVal,
		DataObject::Type objType,
		llvm::Type* newMode
	);
		
	// Method to compile a call wrapper function
	static void compWrapperFunc(
		CompFunction& function,
		CompVersion& version			
	);
	
	// Method to compile a statement sequence
	static llvm::BasicBlock* compStmtSeq(
		StmtSequence* pSequence,
		CompFunction& function,
		CompVersion& version,
		VariableMap varMap,
		BranchPoint& exitPoint,
		BranchList& contPoints,
		BranchList& breakPoints,
		BranchList& returnPoints
	);

	// Method to compile a statement
	static llvm::BasicBlock* compStatement(
		Statement* pStatement,
		CompFunction& function,
		CompVersion& version,
		VariableMap varMap,
		BranchPoint& exitPoint,
		BranchList& contPoints,
		BranchList& breakPoints,
		BranchList& returnPoints
	);

	// Method to compile an expression statement
	static llvm::BasicBlock* compExprStmt(
		ExprStmt* pExprStmt,
		CompFunction& function,
		CompVersion& version,
		VariableMap varMap,
		BranchPoint& exitPoint
	);

	// Method to compile an assignment statement
	static llvm::BasicBlock* compAssignStmt(
		AssignStmt* pAssignStmt,
		CompFunction& function,
		CompVersion& version,
		VariableMap varMap,
		BranchPoint& exitPoint
	);
	
	// Method to compile an if-else statement
	static llvm::BasicBlock* compIfElseStmt(
		IfElseStmt* pIfStmt,
		CompFunction& function,
		CompVersion& version,
		VariableMap varMap,
		BranchPoint& exitPoint,
		BranchList& contPoints,
		BranchList& breakPoints,
		BranchList& returnPoints
	);
	
	// Method to compile a loop statement
	static llvm::BasicBlock* compLoopStmt(
		LoopStmt* pLoopStmt,
		CompFunction& function,
		CompVersion& version,
		VariableMap varMap,
		BranchPoint& exitPoint,
		BranchList& returnPoints
	);
	
	// Method to compile an expression
	static Value compExpression(
		Expression* pExpression,
		CompFunction& function,
		CompVersion& version,
		const Expression::SymbolSet& liveVars,
		const VarDefMap& reachDefs,
		const VarTypeMap& varTypes,
		VariableMap& varMap,
		llvm::BasicBlock* pEntryBlock,
		llvm::BasicBlock* pExitBlock
	);

	// Method to compile a unary expression
	static Value compUnaryExpr(
		UnaryOpExpr* pUnaryExpr,
		CompFunction& function,
		CompVersion& version,
		const Expression::SymbolSet& liveVars,
		const VarDefMap& reachDefs,
		const VarTypeMap& varTypes,
		VariableMap& varMap,
		llvm::BasicBlock* pEntryBlock,
		llvm::BasicBlock* pExitBlock
	);
	
	// Method to compile a binary expression
	static Value compBinaryExpr(
		BinaryOpExpr* pBinaryExpr,
		CompFunction& function,
		CompVersion& version,
		const Expression::SymbolSet& liveVars,
		const VarDefMap& reachDefs,
		const VarTypeMap& varTypes,
		VariableMap& varMap,
		llvm::BasicBlock* pEntryBlock,
		llvm::BasicBlock* pExitBlock
	);
	
	// Method to generate code for binary operations
	static Value compBinaryOp(
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
	);
	
	// Method to compile a parameterized expression
	static ValueVector compParamExpr(
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
	);

	// Method to compile a function call
	static ValueVector compFunctionCall(
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
		llvm::BasicBlock* pExitBlock
	);	
	
	static void compFuncCallJIT(
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
		ValueVector& valueVector);
		
	// Method to compile a symbol expression
	static ValueVector compSymbolExpr(
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
	);

	// Method to compile a symbol evaluation
	static Value compSymbolEval(
		SymbolExpr* pSymbolExpr,
		CompFunction& function,
		CompVersion& version,
		const Expression::SymbolSet& liveVars,
		const VarDefMap& reachDefs,
		const VarTypeMap& varTypes,
		VariableMap& varMap,
		llvm::BasicBlock* pEntryBlock,
		llvm::BasicBlock* pExitBlock
	);
	
	// Method to generate code for a scalar array read operation
	static Value compArrayRead(
		llvm::Value* pMatrixObj,
		DataObject::Type matrixType,
		const LLVMValueVector& indices,
		ParamExpr* pOrigExpr,
		CompFunction& function,
		CompVersion& version,
		llvm::BasicBlock* pEntryBlock,
		llvm::BasicBlock* pExitBlock
	);

	// Method to generate code for a scalar array write operation
	static void compArrayWrite(
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
	);
	
	// Method to generate fallback code for an array-returning expression evaluation
	static ValueVector arrayExprFallback(
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
	);
	
	// Method to generate fallback code for an expression evaluation
	static Value exprFallback(
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
	);
	
	// Method to read values from an array object
	static ValueVector getArrayValues(
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
	);
	
	// Method to create a call throwing a runtime error
	static void createThrowError(
		llvm::IRBuilder<>& irBuilder,
		const char* pErrorText,
		const IIRNode* pErrorNode = NULL
	);
	
	// Method to load the type of an object from memory
	static llvm::LoadInst* loadObjectType(
		llvm::IRBuilder<>& irBuilder,
		llvm::Value* pObjPointer
	);
	
	// Method to load a member value from memory
	static llvm::LoadInst* loadMemberValue(
		llvm::IRBuilder<>& irBuilder,
		llvm::Value* pObjPointer,
		size_t valOffset,
		llvm::Type* valType
	);
	
	// Method to create a native function call
	static llvm::CallInst* createNativeCall(
		llvm::IRBuilder<>& irBuilder,
		void* pNativeFunc,
		const LLVMValueVector& arguments
	);	
	
	// Method to create a pointer constant
	static llvm::Constant* createPtrConst(
		const void* pPointer,
		llvm::Type* valType = llvm::Type::getInt8Ty(*s_Context)
	);
	
	static void setUpParameters(ProgFunction* pFunction, CompFunction& compFunction,  CompVersion& compVersion,
	const TypeSetString& argTypeStr);
	
	static void compFuncReturn(llvm::IRBuilder<>& exitBuilder, CompFunction& compFunction, 
			CompVersion& compVersion, VariableMap& exitVarMap, const ProgFunction::ParamVector& outParams);
	
	// Method to generate copies for a block of stmts
	static void genCopyCode(
		llvm::IRBuilder<>& irBuilder,
		CompFunction& function,
		CompVersion& version,
		const VarTypeMap& typeMap,
		VariableMap& varMap,
		const Statement* pStmt
	);

	// Method to generate copy code for an assignment stmt
	static void genCopyCode(
		llvm::IRBuilder<>& irBuilder,
		CompFunction& function,
		CompVersion& version,
		const VarTypeMap& typeMap,
		VariableMap& varMap,
		const CopyInfo& cpInfo
	);
	
	// Method to generate copy code at a loop's header
	static llvm::BasicBlock* genLoopHeaderCopy(
		llvm::BasicBlock* curBlock,
		StmtSequence* pTestSeq,
		CompFunction& function,
		CompVersion& version,
		LoopStmt* pLoopStmt,
		SymbolExpr* pTestVar,
		VariableMap& varMap
	);
	
	// Method to get the integer type for a given size in bytes
	static llvm::Type* getIntType(size_t sizeInBytes);
	
	// Method to get the constant corresponding to an object type
	static llvm::Constant* getObjType(DataObject::Type objType);

        // LLVM Context 
        static llvm::LLVMContext* s_Context;
	
	// LLVM module to store functions
	static llvm::Module* s_pModule;
	
	// LLVM execution engine
	static llvm::ExecutionEngine* s_pExecEngine;
	
	// LLVM function pass manager (for optimization passes)
	static llvm::FunctionPassManager* s_pFunctionPasses;

	// LLVM function pass manager (for the printing pass)
	static llvm::FunctionPassManager* s_pPrintPass;
	
	// Map of function pointers to native function objects
	static NativeMap s_nativeMap;
	
	// Map of signatures to optimized library functions
	static LibFuncMap s_libFuncMap;
	
	// Map of program functions to function objects
	static FunctionMap s_functionMap;

        static llvm::DataLayout* s_data_layout ;

    // LLVM function pass manager (for osr pass)
    static llvm::FunctionPassManager* s_pOsrInfoPass;
};

#endif // #ifndef JITCOMPILER_H_ 
