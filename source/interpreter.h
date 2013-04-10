// =========================================================================== //
//                                                                             //
// Copyright 2008 Maxime Chevalier-Boisvert and McGill University.             //
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
//  limitations under the License.                                             //
//                                                                             //
// =========================================================================== //

// Include guards
#ifndef INTERPRETER_H_
#define INTERPRETER_H_

// Header files
#include <ext/hash_map>
#include <string>
#include <stack>
#include <set>
#include "iir.h"
#include "runtimebase.h"
#include "configmanager.h"
#include "parser.h"
#include "functions.h"
#include "environment.h"
#include "arrayobj.h"
#include "statements.h"
#include "stmtsequence.h"
#include "expressions.h"
#include "exprstmt.h"
#include "ifelsestmt.h"
#include "loopstmts.h"
#include "unaryopexpr.h"
#include "binaryopexpr.h"
#include "rangeexpr.h"
#include "endexpr.h"
#include "matrixexpr.h"
#include "cellarrayexpr.h"
#include "paramexpr.h"
#include "cellindexexpr.h"
#include "fnhandleexpr.h"
#include "lambdaexpr.h"
#include "typeinfer.h"
#include "analysis_typeinfer.h"

/***************************************************************
* Class   : Interpreter
* Purpose : Interpret the intermediate representation (IIR)
* Initial : Maxime Chevalier-Boisvert on November 8, 2008
****************************************************************
Revisions and bug fixes:
*/
class Interpreter
{
public:

	// Method to initialize the interpreter
	static void initialize();

	// Method to run an interactive-mode command
	static void runCommand(const std::string& commandString);

	// Method to call a function by name
	static ArrayObj* callByName(const std::string& funcName, ArrayObj* pArguments = new ArrayObj());

	// Method to perform a function call
	static ArrayObj* callFunction(Function* pFunction, ArrayObj* pArguments, size_t nargout = 0);

	// Method to evaluate a statement
	static void execStatement(const Statement* pStmt, Environment* pEnv);

	// Method to evaluate a sequence statement
	static void execSeqStmt(const StmtSequence* pSeqStmt, Environment* pEnv);

	// Method to evaluate an assignment statement
	static void evalAssignStmt(const AssignStmt* pStmt, Environment* pEnv);

	// Method to perform the assignment of an object to an expression
	static void assignObject(const Expression* pLeftExpr, DataObject* pRightObject, Environment* pEnv, bool output);

	// Method to evaluate indexing arguments
	static ArrayObj* evalIndexArgs(const Expression::ExprVector& argVector, Environment* pEnv);

	// Method to evaluate an expression statement
	static void evalExprStmt(const ExprStmt* pStmt, Environment* pEnv);

	// Method to evaluate an if-else statement
	static void evalIfStmt(const IfElseStmt* pStmt, Environment* pEnv);

	// Method to evaluate a loop statement
	static void evalLoopStmt(const LoopStmt* pLoopStmt, Environment* pEnv);

	// Method to evaluate an expression
	static DataObject* evalExpression(const Expression* pExpr, Environment* pEnv);

	// Method to evaluate a unary operator expression
	static DataObject* evalUnaryExpr(const UnaryOpExpr* pExpr, Environment* pEnv);

	// Method to evaluate a binary operator expression
	static DataObject* evalBinaryExpr(const BinaryOpExpr* pExpr, Environment* pEnv);

	// Method to evaluate a range expression
	static DataObject* evalRangeExpr(const RangeExpr* pExpr, Environment* pEnv, bool expand = true);

	// Method to evaluate a range end expression
	static DataObject* evalEndExpr(const EndExpr* pExpr, Environment* pEnv);

	// Method to evaluate a matrix expression
	static DataObject* evalMatrixExpr(const MatrixExpr* pExpr, Environment* pEnv);

	// Method to evaluate a cell array expression
	static DataObject* evalCellArrayExpr(const CellArrayExpr* pExpr, Environment* pEnv);

	// Method to evaluate a parameterized expression
	static DataObject* evalParamExpr(const ParamExpr* pExpr, Environment* pEnv, size_t nargout);

	// Method to evaluate a cell indexing expression
	static DataObject* evalCellIndexExpr(const CellIndexExpr* pExpr, Environment* pEnv);

	// Method to evaluate a function handle expression
	static DataObject* evalFnHandleExpr(const FnHandleExpr* pExpr, Environment* pEnv);

	// Method to evaluate a lambda expression
	static DataObject* evalLambdaExpr(const LambdaExpr* pExpr, Environment* pEnv);

	// Method to evaluate a symbol expression
	static DataObject* evalSymbolExpr(const SymbolExpr* pExpr, Environment* pEnv, size_t nargout);
	
	// Method to evaluate a symbol in a given environment
	static DataObject* evalSymbol(const SymbolExpr* pExpr, Environment* pEnv);

	// Method to load an m-file
	static CompUnits loadMFile(const std::string& fileName,  const bool bindScript = true);

	// Method to load compilation units for a command string
	static CompUnits loadSrcText(const std::string& srcText, const std::string& unitName, const bool bindScript = true);

	// Method to load compilation units
	static CompUnits loadCompUnits(CompUnits& nodes, const std::string& nameToken, const bool bindScript);
	
	// Method to build the local environment for a function
	static void buildLocalEnv(ProgFunction* pFunction, Environment* pLocalEnv);
	
	// Method to set a binding in the global environment
	static void setBinding(const std::string& name, DataObject* pObject);

	// Method to get the symbols bound in the global environment
	Environment::SymbolVec getGlobalSyms();
	
	// Method to evaluate a symbol in the global environment
	static DataObject* evalGlobalSym(SymbolExpr* pSymbol);
	
	// Method to clear the program functions from the global environment
	static void clearProgFuncs();

	// Accessors to get the "nargin" and "nargout" symbols
	static SymbolExpr* getNarginSym() { return s_pNarginSym; }
	static SymbolExpr* getNargoutSym() { return s_pNargoutSym; }

	// Config variable to enable/disable type inference validation
	static ConfigVar s_validateTypes;

	// Config variable to enable/disable type inference profiling
	static ConfigVar s_profTypeInfer;
	
private:

	// Global execution environment
	static Environment s_globalEnv;

	// Static "nargin" and "nargout" symbol objects
	static SymbolExpr* s_pNarginSym;
	static SymbolExpr* s_pNargoutSym;

	// Function type info structure definition
	struct FuncTypeInfo
	{
		// Currently running function
		ProgFunction* pCurrentFunction;

		// Current call arguments
		TypeSetString currentArgTypes;

		// Type inference information
		TypeInferInfo* pTypeInferInfo;

		// Validation count map
		typedef __gnu_cxx::hash_map<const IIRNode*, size_t, IntHashFunc<const IIRNode*>, __gnu_cxx::equal_to<const IIRNode*> > ValidCountMap;
		ValidCountMap validCountMap;
	};

	// Stack used for type inference validation
	typedef std::stack<FuncTypeInfo, std::deque<FuncTypeInfo, gc_allocator<FuncTypeInfo> > > TypeInfoStack;
	static TypeInfoStack s_typeInfoStack;

	// Set used for type inference output
	typedef std::set<const Statement*> ViewedStmtSet;
	static ViewedStmtSet s_viewedStmtSet;
};

#endif // #ifndef INTERPRETER_H_
