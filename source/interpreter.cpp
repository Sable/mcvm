// =========================================================================== //
//                                                                             //
// Copyright 2008-2009 Maxime Chevalier-Boisvert and McGill University.        //
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

// Header files
#include <iostream>
#include <cstdio>
#include <cmath>
#include "interpreter.h"
#include "jitcompiler.h"
#include "parser.h"
#include "matrixobjs.h"
#include "matrixops.h"
#include "chararrayobj.h"
#include "cellarrayobj.h"
#include "rangeobj.h"
#include "rangeexpr.h"
#include "constexprs.h"
#include "cellindexexpr.h"

// Static  type inference validation config variable
ConfigVar Interpreter::s_validateTypes("validate_type_infer", ConfigVar::BOOL, "false");

// Config variable to enable/disable type inference profiling
ConfigVar Interpreter::s_profTypeInfer("profile_type_infer", ConfigVar::BOOL, "false");

// Static global environment variable
Environment Interpreter::s_globalEnv;

// Static "nargin" and "nargout" symbol object
SymbolExpr* Interpreter::s_pNarginSym = NULL;
SymbolExpr* Interpreter::s_pNargoutSym = NULL;

// Static stack used for type inference validation
Interpreter::TypeInfoStack Interpreter::s_typeInfoStack;

// Static set used for type inference output
Interpreter::ViewedStmtSet Interpreter::s_viewedStmtSet;

/***************************************************************
* Function: Interpreter::initialize()
* Purpose : Initialize the interpreter
* Initial : Maxime Chevalier-Boisvert on February 9, 2009
****************************************************************
Revisions and bug fixes:
*/
void Interpreter::initialize()
{
	// Register the local config variables
	ConfigManager::registerVar(&s_validateTypes);
	ConfigManager::registerVar(&s_profTypeInfer);

	// Get the static "nargin" and "nargout" symbol object
	s_pNarginSym = SymbolExpr::getSymbol("nargin");
	s_pNargoutSym = SymbolExpr::getSymbol("nargout");
}

/***************************************************************
* Function: Interpreter::runCommand()
* Purpose : Run an interactive-mode command
* Initial : Maxime Chevalier-Boisvert on February 5, 2009
****************************************************************
Revisions and bug fixes:
*/
void Interpreter::runCommand(const std::string& commandString)
{	
	if (!commandString.empty())
	{
		// Attempt to parse the commandString
		CompUnits nodes = loadSrcText(commandString, "input_command");
	
		// If the front node is not a function
		if (nodes.front()->getType() != IIRNode::FUNCTION)
		{
			// Throw an exception
			throw RunError("invalid IIR node produced");
		}
	
		// Get a typed pointer to the function
		ProgFunction* pFuncNode = (ProgFunction*)nodes.front();
	
		// If the node is a script
		if (pFuncNode->isScript())
		{
			// Call the function with no arguments
			callFunction(pFuncNode, new ArrayObj());
		}
	}
}

/***************************************************************
* Function: Interpreter::callByName()
* Purpose : Call a function by name
* Initial : Maxime Chevalier-Boisvert on January 30, 2009
****************************************************************
Revisions and bug fixes:
*/
ArrayObj* Interpreter::callByName(const std::string& funcName, ArrayObj* pArguments)
{
	// Evaluate the function name as a symbol in the global environment
	DataObject* pObject = evalSymbol(SymbolExpr::getSymbol(funcName), &s_globalEnv);

	// Ensure the object is a function
	if (pObject->getType() != DataObject::FUNCTION)
		throw RunError("symbol is not bound to a function");

	// Get a typed pointer to the function object
	Function* pFunction = (Function*)pObject;

	// Call the function with the given arguments
	return callFunction(pFunction, pArguments);
}

/***************************************************************
* Function: Interpreter::callFunction()
* Purpose : Perform a function call
* Initial : Maxime Chevalier-Boisvert on November 13, 2008
****************************************************************
Revisions and bug fixes:
*/
ArrayObj* Interpreter::callFunction(Function* pFunction, ArrayObj* pArguments, size_t nargout)
{
	// Increment the function call count
	PROF_INCR_COUNTER(Profiler::FUNC_CALL_COUNT);

	// Setup a try block to catch any errors
	try
	{
		// Declare an array object to store the output values
		ArrayObj* pOutput;

		// If this is a program function
		if (pFunction->isProgFunction())
		{
			// Get a typed pointer to the program function
			ProgFunction* pProgFunc = (ProgFunction*)pFunction;
			
			// If JIT compilation is enabled and this is not a script nor a closure
			if (JITCompiler::s_jitEnableVar.getBoolValue() == true &&
				pProgFunc->isScript() == false && 
				pProgFunc->isClosure() == false
			)
			{
				// Call a JIT-compiled version of the function
				pOutput = JITCompiler::callFunction(pProgFunc, pArguments, nargout);
			}

			// Otherwise, the function will be interpreted
			else
			{
				// Get a reference to the input parameter vector
				const ProgFunction::ParamVector& inParams = pProgFunc->getInParams();

				// Get a reference to the output parameter vector
				const ProgFunction::ParamVector& outParams = pProgFunc->getOutParams();

				// Get a pointer to the sequence statement
				StmtSequence* pSeqStmt = pProgFunc->getCurrentBody();

				// Get a pointer to the function's local environment
				Environment* pLocalEnv = ProgFunction::getLocalEnv(pProgFunc);

				// Declare a pointer for the calling environment
				Environment* pCallEnv;

				// If the function is a script
				if (pProgFunc->isScript())
				{
					// Make the calling environment the script's local environment
					pCallEnv = pLocalEnv;
				}
				else
				{
					// Extend the local environment for the call
					pCallEnv = Environment::extend(pLocalEnv);
				}

				// If there are too many input arguments, throw an exception
				if (pArguments->getSize() > inParams.size())
					throw RunError("too many input arguments");

				// If too many output arguments are requested, throw an exception
				if (nargout > outParams.size())
					throw RunError("too many output arguments");

				// For each input argument
				for (size_t i = 0; i < pArguments->getSize(); ++i)
				{
					// Get a reference to the value
					DataObject* pValue = pArguments->getObject(i);

					// Get a reference to the symbol
					SymbolExpr* pSymbol = inParams[i];

					// Create a binding in the calling environment
					Environment::bind(pCallEnv, pSymbol, pValue->copy());
				}

				// Bind the "nargin" symbol in the calling environment
				Environment::bind(pCallEnv, s_pNarginSym, new MatrixF64Obj(pArguments->getSize()));

				// Bind the "nargout" symbol in the calling environment
				Environment::bind(pCallEnv, s_pNargoutSym, new MatrixF64Obj(nargout));

				// Get the value of the type validation flag
				bool validateTypes = s_validateTypes.getBoolValue();

				// If type inference validation is enabled
				if (validateTypes == true)
				{
					// Create a function type info object
					FuncTypeInfo funcTypeInfo;

					// Store a pointer to the function being called
					funcTypeInfo.pCurrentFunction = pProgFunc;

					// Build a type set string for the argument types
					funcTypeInfo.currentArgTypes = typeSetStrMake(pArguments);

					// Perform a type inference analysis on the function body
					funcTypeInfo.pTypeInferInfo = (TypeInferInfo*)AnalysisManager::requestInfo(
						&computeTypeInfo,
						pProgFunc,
						pProgFunc->getCurrentBody(),
						funcTypeInfo.currentArgTypes
					);

					// Add the function type info to the stack
					s_typeInfoStack.push(funcTypeInfo);
				}

				// Setup a try block to catch return exceptions
				try
				{
					// Execute the sequence statement in the calling environment
					execSeqStmt(pSeqStmt, pCallEnv);
				}

				// Catch return exceptions
				catch (ReturnExcept e)
				{
				}

				// If type inference validation is enabled
				if (validateTypes == true)
				{
					// Pop the current function type info from the stack
					s_typeInfoStack.pop();
				}

				// Create an array to store the output
				pOutput = new ArrayObj(outParams.size());

				// Compute the effective number of output arguments
				size_t numOutArgs = std::min(std::max(size_t(1), nargout), outParams.size());
				
				// For each output argument
				for (size_t i = 0; i < numOutArgs; ++i)
				{
					// Get a reference to the symbol
					SymbolExpr* pSymbol = outParams[i];

					// Lookup the value in the calling environment
					DataObject* pValue = Environment::lookup(pCallEnv, pSymbol);

					// If the symbol was not found 
					if (pValue == NULL)
					{
						// If the required argument count is 0, break out of the loop
						if (nargout == 0)
							break;
						
						// Throw an exception
						throw RunError("return value unassigned: \"" + pSymbol->getSymName() + "\"");
					}

					// Add the value to the output
					ArrayObj::addObject(pOutput, pValue);
				}
			}
		}

		// Otherwise, this is a library function
		else
		{
			// Get a typed pointer to the library function
			LibFunction* pLibFunc = (LibFunction*)pFunction;

			// Get a function pointer to the host function
			LibFunction::FnPointer pHostFunc = pLibFunc->getHostFunc();

			// Call the host function with the specified arguments
			pOutput = pHostFunc(pArguments);
		}

		// Return the output values
		return pOutput;
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
* Function: Interpreter::execStatement()
* Purpose : Execute a statement
* Initial : Maxime Chevalier-Boisvert on November 13, 2008
****************************************************************
Revisions and bug fixes:
*/
void Interpreter::execStatement(const Statement* pStmt, Environment* pEnv)
{
	// If type inference validation is enabled
	if (s_validateTypes.getBoolValue() == true)
	{
		// Ensure that the type info stack is not empty
		assert (s_typeInfoStack.empty() == false);

		// Get a reference to the current function type info object
		FuncTypeInfo& funcTypeInfo = s_typeInfoStack.top();

		// Ensure that the current function pointer is valid
		assert (funcTypeInfo.pCurrentFunction != NULL);

		// Declare a variable to store the validation counter value
		size_t countVal;

		// If there is no count for this statement yet
		if (funcTypeInfo.validCountMap.find(pStmt) == funcTypeInfo.validCountMap.end())
		{
			// Initialize the count to 1
			funcTypeInfo.validCountMap.insert(FuncTypeInfo::ValidCountMap::value_type(pStmt, 1));

			// Store the counter value
			countVal = 1;
		}
		else
		{
			// Get a reference to the count
			size_t& count = funcTypeInfo.validCountMap[pStmt];

			// Store the counter value
			countVal = count;

			// Increment the count by 1
			count += 1;
		}

		// If the validation should be run
		if (countVal < 128 && s_profTypeInfer.getBoolValue() == false)
		{
			// Get the variable type map for this statement
			TypeInfoMap::const_iterator mapItr = funcTypeInfo.pTypeInferInfo->preTypeMap.find(pStmt);
			assert(mapItr != funcTypeInfo.pTypeInferInfo->preTypeMap.end());
			const VarTypeMap& varTypeMap = mapItr->second;

			// For each entry in the variable type map
			for (VarTypeMap::const_iterator typeItr = varTypeMap.begin(); typeItr != varTypeMap.end(); ++typeItr)
			{
				// Get a pointer to the variable symbol
				const SymbolExpr* pVarSymbol = typeItr->first;

				// Get a reference to the set of possible types
				const TypeSet& typeSet = typeItr->second;

				// Lookup the symbol in the environment
				DataObject* pObject = Environment::lookup(pEnv, pVarSymbol);

				// Test whether the type set is valid or not
				bool result = validateTypes(pObject, typeSet);

				// If the type validation failed
				if (result == false)
				{
					// Log that the type validation failed
					std::cout << "Type validation failed in function \"";
					std::cout << funcTypeInfo.pCurrentFunction->getFuncName() << "\"" << std::endl;

					// Log the statement where the validation failed
					std::cout << "Before statement: " << std::endl << pStmt->toString() << std::endl;

					// Log the concerned symbol
					std::cout << "Concerned symbol: \"" << pVarSymbol->toString() << "\"" << std::endl << std::endl;;

					// If there is a binding for this symbol
					if (pObject != NULL)
					{
						// Log the actual type of the object
						std::cout << "Actual type of object: " << std::endl;
						std::cout << TypeInfo(pObject, true, true).toString() << std::endl << std::endl;
					}
					else
					{
						// Log that there is no binding for this variable
						std::cout << "Actual type: no binding for this variable" << std::endl << std::endl;
					}

					// Log the possible types
					std::cout << "Possible types: " << std::endl;
					if (typeSet.empty())
						std::cout << "Empty set {}" << std::endl;
					for (TypeSet::const_iterator typeItr = typeSet.begin(); typeItr != typeSet.end(); ++typeItr)
						std::cout << typeItr->toString() << std::endl;
					std::cout << std::endl;

					// Log the current function body
					std::cout << "Function body: " << std::endl;
					std::cout << funcTypeInfo.pCurrentFunction->getCurrentBody()->toString() << std::endl;

					// Throw an exception to stop the execution
					throw RunError("type validation failed");
				}
			}

			// If this statement has not yet been viewed
			if (s_viewedStmtSet.find(pStmt) == s_viewedStmtSet.end())
			{
				// Insert the statement into the set
				s_viewedStmtSet.insert(pStmt);

				// Log the current statement
				std::cout << "*** Statement: " << pStmt->toString() << std::endl;

				// Get the uses of this statement
				Expression::SymbolSet stmtUses = pStmt->getSymbolUses();

				// For each use of this statement
				for (Expression::SymbolSet::iterator useItr = stmtUses.begin(); useItr != stmtUses.end(); ++useItr)
				{
					// Get a pointer to the variable symbol
					SymbolExpr* pVarSymbol = *useItr;

					// Log the name of the current symbol
					std::cout << "Symbol: \"" << pVarSymbol->toString() << "\"" << std::endl;

					// Attempt to find this symbol in the variable type map
					VarTypeMap::const_iterator typeItr = varTypeMap.find(pVarSymbol);

					// If the symbol was not found
					if (typeItr == varTypeMap.end())
					{
						// Log this and continue to the next symbol
						std::cout << "No entry in type map for this symbol" << std::endl;
						continue;
					}

					// Get a reference to the type set for this variable
					const TypeSet& typeSet = typeItr->second;

					// Log the possible types
					std::cout << "Possible types: " << std::endl;
					if (typeSet.empty())
						std::cout << "Empty set {}" << std::endl;
					for (TypeSet::const_iterator typeItr = typeSet.begin(); typeItr != typeSet.end(); ++typeItr)
						std::cout << typeItr->toString() << std::endl;
					std::cout << std::endl;
				}
			}
		}
		
		// If type information profiling should be performed
		if (/*s_viewedStmtSet.find(pStmt) == s_viewedStmtSet.end() &&*/
			s_profTypeInfer.getBoolValue() == true && 
			pStmt->getStmtType() != Statement::LOOP &&
			pStmt->getStmtType() != Statement::IF_ELSE)
		{
			// Insert the statement into the viewed set
			//s_viewedStmtSet.insert(pStmt);
			
			// Get the variable type map for this statement
			TypeInfoMap::const_iterator mapItr = funcTypeInfo.pTypeInferInfo->preTypeMap.find(pStmt);
			assert(mapItr != funcTypeInfo.pTypeInferInfo->preTypeMap.end());
			const VarTypeMap& varTypeMap = mapItr->second;
			
			// Get the variable uses for this statement
			Expression::SymbolSet stmtUses = pStmt->getSymbolUses();
			
			// For each use of the statement
			for (Expression::SymbolSet::const_iterator itr = stmtUses.begin(); itr != stmtUses.end(); ++itr)
			{
				// Get a pointer to the variable symbol
				const SymbolExpr* pVarSymbol = *itr;

				// Get a reference to the set of possible types
				VarTypeMap::const_iterator typeItr = varTypeMap.find(pVarSymbol);
				TypeSet typeSet = (typeItr != varTypeMap.end())? typeItr->second:TypeSet();

				// Lookup the symbol in the environment
				DataObject* pObject = Environment::lookup(pEnv, pVarSymbol);
				
				// If the symbol was not found, skip it
				if (pObject == NULL)
					continue;
				
				// Build a type info object from the object
				TypeInfo typeInfo(pObject, true, false);
				
				// Increment number of var uses
				PROF_INCR_COUNTER(Profiler::TYPE_NUM_TYPE_SETS);
				
				// If the object is a scalar
				if (typeInfo.isScalar())
				{
					// Increment object is actual scalar count
					PROF_INCR_COUNTER(Profiler::TYPE_NUM_SCALARS);
				}
				
				// If the object is a matrix (but not a cell array)
				if (typeInfo.getObjType() >= DataObject::MATRIX_I32 && typeInfo.getObjType() <= DataObject::CHARARRAY)
				{
					// Increment matrix count
					PROF_INCR_COUNTER(Profiler::TYPE_NUM_MATRICES);					
				}
				
				// If the type set is empty
				if (typeSet.empty())
				{
					// Increment empty set count
					PROF_INCR_COUNTER(Profiler::TYPE_NUM_EMPTY_SETS);
				}
				
				// If the type set contains one element
				else if (typeSet.size() == 1)
				{
					// Increment set with one element count
					PROF_INCR_COUNTER(Profiler::TYPE_NUM_UNARY_SETS);
					
					// Get the determined type of the object
					const TypeInfo& detType = *typeSet.begin();
					
					// If the determined type is scalar
					if (detType.isScalar())
					{
						// Increment determined type is scalar count
						PROF_INCR_COUNTER(Profiler::TYPE_NUM_KNOWN_SCALARS);	
					}
					
					// If the size is known
					if (detType.getSizeKnown())
					{
						// Increment matrix size known count
						PROF_INCR_COUNTER(Profiler::TYPE_NUM_KNOWN_SIZE);
					}
				}
			}			
		}
	}

	// Switch on the statement type
	switch (pStmt->getStmtType())
	{
		// If-else statement
		case Statement::IF_ELSE:
		{
			// Evaluate the if-else statement
			evalIfStmt((IfElseStmt*)pStmt, pEnv);
		}
		break;

		// Loop statement
		case Statement::LOOP:
		{
			// Evaluate the loop statement
			evalLoopStmt((LoopStmt*)pStmt, pEnv);
		}
		break;

		// Break statement
		case Statement::BREAK:
		{
			// Throw a break exception, to be caught
			// inside the loop
			throw BreakExcept();
		}
		break;

		// Continue statement
		case Statement::CONTINUE:
		{
			// Throw a continue exception, to be caught
			// inside the loop
			throw ContinueExcept();
		}
		break;

		// Return statement
		case Statement::RETURN:
		{
			// Throw a return exception, to be caught
			// at the function call point
			throw ReturnExcept();
		}
		break;

		// Assignment statement
		case Statement::ASSIGN:
		{
			// Evaluate the assignment statement
			evalAssignStmt((AssignStmt*)pStmt, pEnv);
		}
		break;

		// Expression statement
		case Statement::EXPR:
		{
			// Evaluate the expression statement
			evalExprStmt((ExprStmt*)pStmt, pEnv);
		}
		break;

		// Other statement types
		default:
		{
			// Throw a run-time error
			throw RunError("unexpected statement type", pStmt);
		}
	}
		
	// If type inference validation is enabled
	if (s_validateTypes.getBoolValue() == true && s_typeInfoStack.empty() == false)
	{
		// Get a reference to the current function type info object
		FuncTypeInfo& funcTypeInfo = s_typeInfoStack.top();

		// Ensure that the current function pointer is valid
		assert (funcTypeInfo.pCurrentFunction != NULL);

		// Declare a variable to store the validation counter value
		size_t countVal;

		// If there is no count for this statement yet
		if (funcTypeInfo.validCountMap.find(pStmt) == funcTypeInfo.validCountMap.end())
		{
			// Initialize the count to 1
			funcTypeInfo.validCountMap.insert(FuncTypeInfo::ValidCountMap::value_type(pStmt, 1));

			// Store the counter value
			countVal = 1;
		}
		else
		{
			// Get a reference to the count
			countVal = funcTypeInfo.validCountMap[pStmt];
		}
		
		// If the validation should be run
		if (countVal < 128)
		{
			// Get the variable type map for this statement
			TypeInfoMap::const_iterator mapItr = funcTypeInfo.pTypeInferInfo->postTypeMap.find(pStmt);
			assert(mapItr != funcTypeInfo.pTypeInferInfo->postTypeMap.end());
			const VarTypeMap& varTypeMap = mapItr->second;

			// For each entry in the variable type map
			for (VarTypeMap::const_iterator typeItr = varTypeMap.begin(); typeItr != varTypeMap.end(); ++typeItr)
			{
				// Get a pointer to the variable symbol
				const SymbolExpr* pVarSymbol = typeItr->first;

				// Get a reference to the set of possible types
				const TypeSet& typeSet = typeItr->second;

				// Lookup the symbol in the environment
				DataObject* pObject = Environment::lookup(pEnv, pVarSymbol);

				// Test whether the type set is valid or not
				bool result = validateTypes(pObject, typeSet);

				// If the type validation failed
				if (result == false)
				{
					// Log that the type validation failed
					std::cout << "Type validation failed in function \"";
					std::cout << funcTypeInfo.pCurrentFunction->getFuncName() << "\"" << std::endl;

					// Log the statement where the validation failed
					std::cout << "After statement: " << std::endl << pStmt->toString() << std::endl;

					// Log the concerned symbol
					std::cout << "Concerned symbol: \"" << pVarSymbol->toString() << "\"" << std::endl << std::endl;;

					// If there is a binding for this symbol
					if (pObject != NULL)
					{
						// Log the actual type of the object
						std::cout << "Actual type of object: " << std::endl;
						std::cout << TypeInfo(pObject, true, true).toString() << std::endl << std::endl;
					}
					else
					{
						// Log that there is no binding for this variable
						std::cout << "Actual type: no binding for this variable" << std::endl << std::endl;
					}

					// Log the possible types
					std::cout << "Possible types: " << std::endl;
					if (typeSet.empty())
						std::cout << "Empty set {}" << std::endl;
					for (TypeSet::const_iterator typeItr = typeSet.begin(); typeItr != typeSet.end(); ++typeItr)
						std::cout << typeItr->toString() << std::endl;
					std::cout << std::endl;

					// Log the current function body
					std::cout << "Function body: " << std::endl;
					std::cout << funcTypeInfo.pCurrentFunction->getCurrentBody()->toString() << std::endl;

					// Throw an exception to stop the execution
					throw RunError("type validation failed");
				}
			}
		}
	}	
}

/***************************************************************
* Function: Interpreter::execSeqStmt()
* Purpose : Execute a statement
* Initial : Maxime Chevalier-Boisvert on November 13, 2008
****************************************************************
Revisions and bug fixes:
*/
void Interpreter::execSeqStmt(const StmtSequence* pSeqStmt, Environment* pEnv)
{
	// Get a reference to the statement vector
	const StmtSequence::StmtVector& stmtVector = pSeqStmt->getStatements();

	// For each statement in the vector
	for (StmtSequence::StmtVector::const_iterator itr = stmtVector.begin(); itr != stmtVector.end(); ++itr)
	{
		// Execute this statement
		execStatement(*itr, pEnv);
	}
}

/***************************************************************
* Function: Interpreter::evalAssignStmt()
* Purpose : Evaluate an assignment statement
* Initial : Maxime Chevalier-Boisvert on November 13, 2008
****************************************************************
Revisions and bug fixes:
*/
void Interpreter::evalAssignStmt(const AssignStmt* pStmt, Environment* pEnv)
{
	// Get the list of left expressions
	const AssignStmt::ExprVector leftExprs = pStmt->getLeftExprs();

	// Get pointers to the right expression
	Expression* pRightExpr = pStmt->getRightExpr();

	// Declare a variable to store the evaluation result
	DataObject* pResult;

	// If the right expression is a parameterized expression
	if (pRightExpr->getExprType() == Expression::PARAM)
	{
		// Evaluate the parameterized expression directly to preserve possible multiple outputs
		pResult = evalParamExpr((ParamExpr*)pRightExpr, pEnv, leftExprs.size());
	}
	
	// Otherwise, if the right expression is a symbol expression
	else if (pRightExpr->getExprType() == Expression::SYMBOL)
	{
		// Evaluate the symbol expression directly to preserve possible multiple outputs
		// NOTE: this is because the symbol expression may be a function call
		pResult = evalSymbolExpr((SymbolExpr*)pRightExpr, pEnv, leftExprs.size());		
	}
	
	// Otherwise, for any other type of right expression
	else
	{
		// Evaluate the right side expression generically
		pResult = evalExpression(pRightExpr, pEnv);
	}

	// If the evaluation result is an array object
	if (pResult->getType() == DataObject::ARRAY)
	{
		// Get a pointer to the array object
		ArrayObj* pArrayObj = (ArrayObj*)pResult;

		// If there are not enough return arguments, throw an exception
		if (pArrayObj->getSize() < leftExprs.size())
			throw RunError("insuffucient number of return values in assignment", pStmt);

		// Compute the number of values to assign
		size_t numAssign = std::min(leftExprs.size(), pArrayObj->getSize());

		// For each value to assign
		for (size_t i = 0; i < numAssign; ++i)
		{
			// Assign the corresponding object to this left expression
			assignObject(leftExprs[i], pArrayObj->getObject(i), pEnv, !pStmt->getSuppressFlag());
		}
	}

	// There is only value to assign
	else
	{
		// Get the first left expression
		Expression* pLeftExpr = leftExprs.front();

		// Perform the assignment
		assignObject(pLeftExpr, pResult, pEnv, !pStmt->getSuppressFlag());
	}
}

/***************************************************************
* Function: Interpreter::assignObject()
* Purpose : Perform the assignment of an object to an expression
* Initial : Maxime Chevalier-Boisvert on February 1, 2009
****************************************************************
Revisions and bug fixes:
*/
void Interpreter::assignObject(const Expression* pLeftExpr, DataObject* pRightObject, Environment* pEnv, bool output)
{
	// Declare a pointer for the symbol expression
	SymbolExpr* pSymExpr;

	// Switch on the left expression type
	switch (pLeftExpr->getExprType())
	{
		// If the left side expression is a symbol
		case Expression::SYMBOL:
		{
			// Get a typed pointer to the symbol expression
			pSymExpr = (SymbolExpr*)pLeftExpr;

			// Bind the symbol to the result
			Environment::bind(pEnv, pSymExpr, pRightObject);
		}
		break;

		// If the left side is a parameterized expression
		case Expression::PARAM:
		{
			// Get a typed pointer to the parameterized expression
			ParamExpr* pParamExpr = (ParamExpr*)pLeftExpr;

			// Get a pointer to the symbol expression
			pSymExpr = pParamExpr->getSymExpr();

			// Lookup the symbol in the current environment
			DataObject* pLeftObject = Environment::lookup(pEnv, pSymExpr);

			// Get a reference to the argument vector
			const ParamExpr::ExprVector& argVector = pParamExpr->getArguments();

			// Evaluate the indexing arguments
			ArrayObj* pArguments = evalIndexArgs(argVector, pEnv);

			// Declare a variable to indicate if an object was created
			bool objCreated = false;

			// If the left object does not exist
			if (pLeftObject == NULL)
			{
				// Create an object of the same type as the right object
				pLeftObject = createBlankObj(pRightObject->getType());

				// Note that an object was created
				objCreated = true;
			}

			// If the left object is not a matrix
			if (pLeftObject->isMatrixObj() == false)
			{
				// Throw an exception
				throw RunError("unsupported left-expression type in parameterized assignment", pLeftExpr);
			}

			// If the right object is not a matrix
			if (pRightObject->isMatrixObj() == false)
			{
				// Throw an exception
				throw RunError("unsupported object type in parameterized assignment", pLeftExpr);
			}

			// Get a typed pointer to the left matrix
			BaseMatrixObj* pLeftMatrix = (BaseMatrixObj*)pLeftObject;

			// Get a typed pointer to the right matrix
			BaseMatrixObj* pRightMatrix = (BaseMatrixObj*)pRightObject;

			// If some of the indices are not valid
			if (pLeftMatrix->validIndices(pArguments) == false)
			{
				// Throw an exception
				throw RunError("invalid indices in matrix indexing");
			}

			// Get the maximum indices
			DimVector maxInds = pLeftMatrix->getMaxIndices(pArguments, pRightMatrix);

			// If bounds checking fails for these indices
			if (pLeftMatrix->boundsCheckND(maxInds) == false)
			{
				// Expand the matrix to match the new dimensions
				pLeftMatrix->expand(maxInds);
			}

			// If the right object is a complex matrix but the left matrix is not
			if (pRightObject->getType() == DataObject::MATRIX_C128 && pLeftMatrix->getType() != DataObject::MATRIX_C128)
			{
				// Convert the left matrix into a complex matrix
				pLeftMatrix = (BaseMatrixObj*)pLeftMatrix->convert(DataObject::MATRIX_C128);

				// Note that a new left matrix was created
				objCreated = true;
			}

			// Set the matrix slice
			pLeftMatrix->setSliceND(pArguments, pRightObject);

			// If an object was created
			if (objCreated)
			{
				// Create a binding for it in the environment
				Environment::bind(pEnv, pSymExpr, pLeftMatrix);
			}
		}
		break;

		// If the left side is a cell indexing expression
		case Expression::CELL_INDEX:
		{
			// Get a typed pointer to the cell index expression
			CellIndexExpr* pCellExpr = (CellIndexExpr*)pLeftExpr;

			// Get a pointer to the symbol expression
			pSymExpr = pCellExpr->getSymExpr();

			// Lookup the symbol in the current environment
			DataObject* pLeftObject = Environment::lookup(pEnv, pSymExpr);

			// Get a reference to the argument vector
			const ParamExpr::ExprVector& argVector = pCellExpr->getArguments();

			// Evaluate the indexing arguments
			ArrayObj* pArguments = evalIndexArgs(argVector, pEnv);

			// Declare a variable to indicate if an object was created
			bool objCreated = false;

			// If the left object does not exist
			if (pLeftObject == NULL)
			{
				// Create a cell array object
				pLeftObject = new CellArrayObj();

				// Note that an object was created
				objCreated = true;
			}

			// If the object is not a cell array
			if (pLeftObject->getType() != DataObject::CELLARRAY)
			{
				// Throw an exception
				throw RunError("cellarray indexing on non-cellarray object", pLeftExpr);
			}

			// Get a typed pointer to the matrix
			BaseMatrixObj* pMatrix = (BaseMatrixObj*)pLeftObject;

			// Wrap a copy of the right-hand object in a cell array
			BaseMatrixObj* pRightMatrix = new CellArrayObj(pRightObject->copy());

			// If some of the indices are not valid
			if (pMatrix->validIndices(pArguments) == false)
			{
				// Throw an exception
				throw RunError("invalid indices in matrix indexing");
			}

			// Get the maximum indices
			DimVector maxInds = pMatrix->getMaxIndices(pArguments, pRightMatrix);

			// If bounds checking fails for these indices
			if (pMatrix->boundsCheckND(maxInds) == false)
			{
				// Expand the matrix to match the new dimensions
				pMatrix->expand(maxInds);
			}

			// Set the matrix slice
			pMatrix->setSliceND(pArguments, pRightMatrix);

			// If an object was created
			if (objCreated)
			{
				// Create a binding for it in the environment
				Environment::bind(pEnv, pSymExpr, pLeftObject);
			}
		}
		break;

		// Other expression types
		default:
		{
			// Throw an exception
			throw RunError("unsupported left-expression in assignment", pLeftExpr);
		}
	}

	// If output should be presented
	if (output)
	{
		// Display the symbol name and an equal sign
		std::cout << pSymExpr->toString() << " = "  << std::endl;

		// Display the value bound to the symbol
		std::cout << Environment::lookup(pEnv, pSymExpr)->toString() << std::endl;
	}
}

/***************************************************************
* Function: Interpreter::evalIndexArgs()
* Purpose : Evaluate indexing arguments
* Initial : Maxime Chevalier-Boisvert on February 17, 2009
****************************************************************
Revisions and bug fixes:
*/
ArrayObj* Interpreter::evalIndexArgs(const Expression::ExprVector& argVector, Environment* pEnv)
{
	// Create an array object for the arguments
	ArrayObj* pArguments = new ArrayObj(argVector.size());
	
	// For each argument
	for (Expression::ExprVector::const_iterator itr = argVector.begin(); itr != argVector.end(); ++itr)
	{
		// Get a typed pointer to the expression
		Expression* pExpr = *itr;

		// Declare a variable for the expression value
		DataObject* pValue;

		// If this is a range expression
		if (pExpr->getExprType() == Expression::RANGE)
		{
			// Evaluate the range without expanding it
			pValue = evalRangeExpr((RangeExpr*)pExpr, pEnv, false);
		}
		else
		{
			// Evaluate the expression normally
			pValue = evalExpression(pExpr, pEnv);
		}

		// Add the value to the argument array
		ArrayObj::addObject(pArguments, pValue);
	}
	
	// Return the evaluated arguments
	return pArguments;
}

/***************************************************************
* Function: Interpreter::evalExprStmt()
* Purpose : Evaluate an expression statement
* Initial : Maxime Chevalier-Boisvert on February 19, 2009
****************************************************************
Revisions and bug fixes:
*/
void Interpreter::evalExprStmt(const ExprStmt* pStmt, Environment* pEnv)
{
	// Get a pointer to the expression
	Expression* pExpr = pStmt->getExpression();
	
	// Declare a variable to store the evaluation result
	DataObject* pResult;
	
	// If the expression is a parameterized expression
	if (pExpr->getExprType() == Expression::PARAM)
	{
		// Evaluate the parameterized expression directly with zero arguments required
		pResult = evalParamExpr((ParamExpr*)pExpr, pEnv, 0);
	}
	
	// Otherwise, if the expression is a symbol expression
	else if (pExpr->getExprType() == Expression::SYMBOL)
	{
		// Evaluate the symbol expression directly with zero arguments required
		// NOTE: this is because the symbol expression may be a function call
		pResult = evalSymbolExpr((SymbolExpr*)pExpr, pEnv, 0);		
	}
	
	// Otherwise, for any other type of expression
	else
	{
		// Evaluate the right side expression generically
		pResult = evalExpression(pExpr, pEnv);
	}
	
	// If output should be suppressed, stop
	if (pStmt->getSuppressFlag())
		return;

	// If the result is an array object
	if (pResult->getType() == DataObject::ARRAY)
	{
		// Get a typed pointer to the array object
		ArrayObj* pArrayObj = (ArrayObj*)pResult;

		// If the array object has nonzero size, use the first element as the result
		if (pArrayObj->getSize() > 0)
			pResult = pArrayObj->getObject(0);
		
		// Otherwise, if the array object has size zero, return early
		else if (pArrayObj->getSize() == 0)
			return;
	}

	// Display "ans =" to indicate that we will output the result
	std::cout << "ans = "  << std::endl;

	// Display the expression evaluation result
	std::cout << pResult->toString() << std::endl;
}

/***************************************************************
* Function: Interpreter::evalIfStmt()
* Purpose : Evaluate an if-else statement
* Initial : Maxime Chevalier-Boisvert on November 13, 2008
****************************************************************
Revisions and bug fixes:
*/
void Interpreter::evalIfStmt(const IfElseStmt* pStmt, Environment* pEnv)
{
	// Get a reference to the condition expression
	Expression* pCondExpr = pStmt->getCondition();

	// Evaluate the condition expression
	DataObject* pCondVal = evalExpression(pCondExpr, pEnv);

	// Evaluate the condition value as a boolean
	bool boolCondVal = getBoolValue(pCondVal);

	// If the condition evaluated to a true value
	if (boolCondVal == true)
	{
		// Execute the if block
		execSeqStmt(pStmt->getIfBlock(), pEnv);
	}

	// Otherwise, if the condition evaluated to false
	else
	{
		// Execute the else block
		execSeqStmt(pStmt->getElseBlock(), pEnv);
	}
}

/***************************************************************
* Function: Interpreter::evalLoopStmt()
* Purpose : Evaluate a loop statement
* Initial : Maxime Chevalier-Boisvert on January 23, 2009
****************************************************************
Revisions and bug fixes:
*/
void Interpreter::evalLoopStmt(const LoopStmt* pLoopStmt, Environment* pEnv)
{
	// Execute the loop initialization code
	execSeqStmt(pLoopStmt->getInitSeq(), pEnv);

	// Loop until the test condition is not met
	for (;;)
	{
		// Execute the loop test condition code
		execSeqStmt(pLoopStmt->getTestSeq(), pEnv);

		// Lookup the test variable
		DataObject* pTestResult = Environment::lookup(pEnv, pLoopStmt->getTestVar());

		// Ensure the lookup was successful
		assert (pTestResult != NULL);

		// Evaluate the boolean value of the test result
		bool boolResult = getBoolValue(pTestResult);

		// If the result is false, prevent the body from executing
		if (boolResult == false)
			break;

		// Setup a try block to catch break and continue exceptions
		try
		{
			// Execute the loop body code
			execSeqStmt(pLoopStmt->getBodySeq(), pEnv);
		}

		// If a break exception occurs
		catch (BreakExcept e)
		{
			// Break out of the loop
			break;
		}

		// If a continue exception occurs
		catch (ContinueExcept e)
		{
			// Do nothing, increment the loop normally
		}

		// Execute the index incrementation code
		execSeqStmt(pLoopStmt->getIncrSeq(), pEnv);
	}
}

/***************************************************************
* Function: Interpreter::evalExpression()
* Purpose : Evaluate an expression
* Initial : Maxime Chevalier-Boisvert on November 13, 2008
****************************************************************
Revisions and bug fixes:
*/
DataObject* Interpreter::evalExpression(const Expression* pExpr, Environment* pEnv)
{
	// Switch on the expression type
	switch (pExpr->getExprType())
	{
		// Parameterized expression
		case Expression::PARAM:
		{
			// Evaluate the parameterized expression
			DataObject* pResult = evalParamExpr((ParamExpr*)pExpr, pEnv, 1);

			// If the result is an array object
			if (pResult->getType() == DataObject::ARRAY)
			{
				// Get a typed pointer to the array object
				ArrayObj* pArrayObj = (ArrayObj*)pResult;

				// If the array object has nonzero size, return its first member
				if (pArrayObj->getSize() > 0)
					return pArrayObj->getObject(0);
			}

			// Return the result directly
			return pResult;
		}
		break;

		// Cell indexing expression
		case Expression::CELL_INDEX:
		{
			// Evaluate the cell indexing expression
			ArrayObj* pArrayObj = (ArrayObj*)evalCellIndexExpr((CellIndexExpr*)pExpr, pEnv);

			// If the array object has nonzero size, return its first member
			if (pArrayObj->getSize() > 0)
				return pArrayObj->getObject(0);

			// Return the result directly
			return pArrayObj;
		}
		break;

		// Binary operator expression
		case Expression::BINARY_OP:
		{
			// Evaluate the binary expression
			return evalBinaryExpr((BinaryOpExpr*)pExpr, pEnv);
		}
		break;

		// Unary operator expression
		case Expression::UNARY_OP:
		{
			// Evaluate the unary expression
			return evalUnaryExpr((UnaryOpExpr*)pExpr, pEnv);
		}
		break;

		// Symbol expression
		case Expression::SYMBOL:
		{
			// Evaluate the symbol expression
			DataObject* pResult = evalSymbolExpr((SymbolExpr*)pExpr, pEnv, 1);

			// If the result is an array object
			if (pResult->getType() == DataObject::ARRAY)
			{
				// Get a typed pointer to the array object
				ArrayObj* pArrayObj = (ArrayObj*)pResult;

				// If the array object has nonzero size, return its first member
				if (pArrayObj->getSize() > 0)
					return pArrayObj->getObject(0);
			}

			// Return the result directly
			return pResult;
		}
		break;

		// Integer constant expression
		case Expression::INT_CONST:
		{
			// Get a typed pointer to the constant expression
			IntConstExpr* pConstExpr = (IntConstExpr*)pExpr;

			// Return a new floating-point object with this value
			return new MatrixF64Obj(pConstExpr->getValue());
		}
		break;

		// Floating-point constant expression
		case Expression::FP_CONST:
		{
			// Get a typed pointer to the constant expression
			FPConstExpr* pConstExpr = (FPConstExpr*)pExpr;

			// Return a new floating-point object with this value
			return new MatrixF64Obj(pConstExpr->getValue());
		}
		break;

		// String constant expression
		case Expression::STR_CONST:
		{
			// Get a typed pointer to the constant expression
			StrConstExpr* pConstExpr = (StrConstExpr*)pExpr;

			// Return a new string object with this value
			return new CharArrayObj(pConstExpr->getValue());
		}
		break;

		// Range expression
		case Expression::RANGE:
		{
			// Evaluate the range expression
			return evalRangeExpr((RangeExpr*)pExpr, pEnv);
		}
		break;

		// Range end expression
		case Expression::END:
		{
			// Evaluate the range end expression
			return evalEndExpr((EndExpr*)pExpr, pEnv);
		}
		break;

		// Matrix expression
		case Expression::MATRIX:
		{
			// Evaluate the matrix expression
			return evalMatrixExpr((MatrixExpr*)pExpr, pEnv);
		}
		break;

		// Cell array expression
		case Expression::CELLARRAY:
		{
			// Evaluate the cell array expression
			return evalCellArrayExpr((CellArrayExpr*)pExpr, pEnv);
		}
		break;

		// Function handle expression
		case Expression::FN_HANDLE:
		{
			// Evaluate the function handle expression
			return evalFnHandleExpr((FnHandleExpr*)pExpr, pEnv);
		}
		break;

		// Lambda expression
		case Expression::LAMBDA:
		{
			// Evaluate the lambda expression
			return evalLambdaExpr((LambdaExpr*)pExpr, pEnv);
		}
		break;

		// All other expression types
		default:
		{
			// Throw an exception
			throw RunError("unsupported expression type", pExpr);
		}
	}
}

/***************************************************************
* Function: Interpreter::evalUnaryExpr()
* Purpose : Evaluate an expression
* Initial : Maxime Chevalier-Boisvert on January 23, 2008
****************************************************************
Revisions and bug fixes:
*/
DataObject* Interpreter::evalUnaryExpr(const UnaryOpExpr* pExpr, Environment* pEnv)
{
	// Evaluate the argument value
	DataObject* pArgVal = evalExpression(pExpr->getOperand(), pEnv);

	// Switch on the operator type
	switch (pExpr->getOperator())
	{
		// Unary plus
		case UnaryOpExpr::PLUS:
		{
			// Return the operand value unaltered
			return pArgVal;
		}
		break;

		// Arithmetic negation
		case UnaryOpExpr::MINUS:
		{
			// If the value is a 128-bit complex matrix
			if (pArgVal->getType() == DataObject::MATRIX_C128)
			{
				// Get a typed pointer to the value
				MatrixC128Obj* pMatrix = (MatrixC128Obj*)pArgVal;

				// Multiply the matrix by -1
				return MatrixC128Obj::scalarMult(pMatrix, -1);
			}

			// Convert the argument to a 64-bit matrix, if necessary
			if (pArgVal->getType() != DataObject::MATRIX_F64)
				pArgVal = pArgVal->convert(DataObject::MATRIX_F64);

			// Get a typed pointer to the value
			MatrixF64Obj* pMatrix = (MatrixF64Obj*)pArgVal;

			// Multiply the matrix by -1
			return MatrixF64Obj::scalarMult(pMatrix, -1);
		}
		break;

		// Logical negation
		case UnaryOpExpr::NOT:
		{
			// If the value is a 64-bit floating-point matrix
			if (pArgVal->getType() == DataObject::MATRIX_F64)
			{
				// Get a typed pointer to the value
				MatrixF64Obj* pMatrix = (MatrixF64Obj*)pArgVal;

				// Perform the negation operation
				return MatrixF64Obj::arrayOp<NotOp<float64>, bool>(pMatrix);
			}

			// Convert the argument to a logical array, if necessary
			if (pArgVal->getType() != DataObject::LOGICALARRAY)
				pArgVal = pArgVal->convert(DataObject::LOGICALARRAY);

			// Get a typed pointer to the value
			LogicalArrayObj* pMatrix = (LogicalArrayObj*)pArgVal;

			// Perform the negation operation
			return LogicalArrayObj::arrayOp<NotOp<bool>, bool>(pMatrix);
		}
		break;

		// Matrix transposition
		case UnaryOpExpr::TRANSP:
		{
			// If the value is a logical array
			if (pArgVal->getType() == DataObject::LOGICALARRAY)
			{
				// Get a typed pointer to the value
				LogicalArrayObj* pMatrix = (LogicalArrayObj*)pArgVal;

				// Get the conjugate transpose the matrix
				return LogicalArrayObj::transpose(pMatrix);
			}

			// If the value is a floating-point matrix
			else if (pArgVal->getType() == DataObject::MATRIX_F64)
			{
				// Get a typed pointer to the value
				MatrixF64Obj* pMatrix = (MatrixF64Obj*)pArgVal;

				// Transpose the matrix
				return MatrixF64Obj::transpose(pMatrix);
			}

			// If the value is a complex matrix
			else if (pArgVal->getType() == DataObject::MATRIX_C128)
			{
				// Get a typed pointer to the value
				MatrixC128Obj* pMatrix = (MatrixC128Obj*)pArgVal;

				// Get the conjugate transpose the matrix
				return MatrixC128Obj::conjTranspose(pMatrix);
			}
			
			// If the value is a cell array
			else if (pArgVal->getType() == DataObject::CELLARRAY)
			{
				// Get a typed pointer to the value
				CellArrayObj* pMatrix = (CellArrayObj*)pArgVal;

				// Get the conjugate transpose the matrix
				return CellArrayObj::transpose(pMatrix);
			}			
		}
		break;

		// Array transpositions
		case UnaryOpExpr::ARRAY_TRANSP:
		{
			// If the value is a logical array
			if (pArgVal->getType() == DataObject::LOGICALARRAY)
			{
				// Get a typed pointer to the value
				LogicalArrayObj* pMatrix = (LogicalArrayObj*)pArgVal;

				// Transpose the matrix
				return LogicalArrayObj::transpose(pMatrix);
			}

			// If the value is a 64-bit float matrix
			else if (pArgVal->getType() == DataObject::MATRIX_F64)
			{
				// Get a typed pointer to the value
				MatrixF64Obj* pMatrix = (MatrixF64Obj*)pArgVal;

				// Transpose the matrix
				return MatrixF64Obj::transpose(pMatrix);
			}

			// If the value is a 128-bit complex matrix
			else if (pArgVal->getType() == DataObject::MATRIX_C128)
			{
				// Get a typed pointer to the value
				MatrixC128Obj* pMatrix = (MatrixC128Obj*)pArgVal;

				// Transpose the matrix
				return MatrixC128Obj::transpose(pMatrix);
			}
		}
		break;

		// Any other operator type
		default:
		{
			// Throw an exception
			throw RunError("unhandled unary expression", pExpr);
		}
	}

	// If we get to this point, no value was produced
	// Throw a descriptive run-time exception
	throw RunError("unsupported operand type in unary expression", pExpr);
}

/***************************************************************
* Function: Interpreter::evalBinaryExpr()
* Purpose : Evaluate a binary expression
* Initial : Maxime Chevalier-Boisvert on November 13, 2008
****************************************************************
Revisions and bug fixes:
*/
DataObject* Interpreter::evalBinaryExpr(const BinaryOpExpr* pExpr, Environment* pEnv)
{
	// Get the left and right side expressions
	Expression* pLeftExpr = pExpr->getLeftExpr();
	Expression* pRightExpr = pExpr->getRightExpr();

	// Switch on the operator type
	switch (pExpr->getOperator())
	{
		// Binary addition
		case BinaryOpExpr::PLUS:
		{
			// Evaluate the left and right expressions
			DataObject* pLeftVal = evalExpression(pLeftExpr, pEnv);
			DataObject* pRightVal = evalExpression(pRightExpr, pEnv);

			// Perform the addition
			return arrayArithOp<AddOp>(pLeftVal, pRightVal);
		}
		break;

		// Binary subtraction
		case BinaryOpExpr::MINUS:
		{
			// Evaluate the left and right expressions
			DataObject* pLeftVal = evalExpression(pLeftExpr, pEnv);
			DataObject* pRightVal = evalExpression(pRightExpr, pEnv);

			// Perform the subtraction
			return arrayArithOp<SubOp>(pLeftVal, pRightVal);
		}
		break;

		// Binary multiplication
		case BinaryOpExpr::MULT:
		{
			// Evaluate the left and right expressions
			DataObject* pLeftVal = evalExpression(pLeftExpr, pEnv);
			DataObject* pRightVal = evalExpression(pRightExpr, pEnv);

			// Perform the multiplication
			return matrixMultOp(pLeftVal, pRightVal);
		}
		break;

		// Array multiplication
		case BinaryOpExpr::ARRAY_MULT:
		{
			// Evaluate the left and right expressions
			DataObject* pLeftVal = evalExpression(pLeftExpr, pEnv);
			DataObject* pRightVal = evalExpression(pRightExpr, pEnv);
			
			// Perform the array multiplication
			return arrayArithOp<MultOp>(pLeftVal, pRightVal);
		}
		break;

		// Right division
		case BinaryOpExpr::DIV:
		{
			// Evaluate the left and right expressions
			DataObject* pLeftVal = evalExpression(pLeftExpr, pEnv);
			DataObject* pRightVal = evalExpression(pRightExpr, pEnv);
			
			// Perform the right division operation
			return matrixRightDivOp(pLeftVal, pRightVal);
		}
		break;

		// Array division
		case BinaryOpExpr::ARRAY_DIV:
		{
			// Evaluate the left and right expressions
			DataObject* pLeftVal = evalExpression(pLeftExpr, pEnv);
			DataObject* pRightVal = evalExpression(pRightExpr, pEnv);

			// Perform the array division
			return arrayArithOp<DivOp>(pLeftVal, pRightVal);
		}
		break;

		// Left division
		case BinaryOpExpr::LEFT_DIV:
		{
			// Evaluate the left and right expressions
			DataObject* pLeftVal = evalExpression(pLeftExpr, pEnv);
			DataObject* pRightVal = evalExpression(pRightExpr, pEnv);

			// If both values are matrices
			if (pLeftVal->getType() == DataObject::MATRIX_F64 && pRightVal->getType() == DataObject::MATRIX_F64)
			{
				// Get typed pointers to the values
				MatrixF64Obj* pLMatrix = (MatrixF64Obj*)pLeftVal;
				MatrixF64Obj* pRMatrix = (MatrixF64Obj*)pRightVal;

				// If the left matrix is not a scalar
				if (pLMatrix->isScalar() == false)
				{
					// If the matrix dimensions are not compatible
					if (!MatrixF64Obj::leftDivCompatible(pLMatrix, pRMatrix))
					{
						// Throw an exception
						throw RunError("incompatible matrix dimensions in matrix left division");
					}

					// Perform matrix left division
					return MatrixF64Obj::matrixLeftDiv(pLMatrix, pRMatrix);
				}
			}
		}

		// Binary power
		case BinaryOpExpr::POWER:
		{
			// Evaluate the left and right expressions
			DataObject* pLeftVal = evalExpression(pLeftExpr, pEnv);
			DataObject* pRightVal = evalExpression(pRightExpr, pEnv);

			// If either of the values are 128-bit complex matrices
			if (pLeftVal->getType() == DataObject::MATRIX_C128 || pRightVal->getType() == DataObject::MATRIX_C128)
			{
				// Convert the objects to 128-bit complex matrices, if necessary
				if (pLeftVal->getType() != DataObject::MATRIX_C128)	 pLeftVal = pLeftVal->convert(DataObject::MATRIX_C128);
				if (pRightVal->getType() != DataObject::MATRIX_C128) pRightVal = pRightVal->convert(DataObject::MATRIX_C128);

				// Get typed pointers to the values
				MatrixC128Obj* pLMatrix = (MatrixC128Obj*)pLeftVal;
				MatrixC128Obj* pRMatrix = (MatrixC128Obj*)pRightVal;

				// If both matrices are scalars
				if (pLMatrix->isScalar() && pRMatrix->isScalar())
				{
					// Get the scalar values
					Complex128 lScalar = pLMatrix->getScalar();
					Complex128 rScalar = pRMatrix->getScalar();

					// Perform the operation
					return new MatrixC128Obj(pow(lScalar, rScalar));
				}
			}

			// Convert the objects to 64-bit float matrices, if necessary
			if (pLeftVal->getType() != DataObject::MATRIX_F64) 	pLeftVal = pLeftVal->convert(DataObject::MATRIX_F64);
			if (pRightVal->getType() != DataObject::MATRIX_F64) pRightVal = pRightVal->convert(DataObject::MATRIX_F64);

			// Get typed pointers to the values
			MatrixF64Obj* pLMatrix = (MatrixF64Obj*)pLeftVal;
			MatrixF64Obj* pRMatrix = (MatrixF64Obj*)pRightVal;

			// If both matrices are scalars
			if (pLMatrix->isScalar() && pRMatrix->isScalar())
			{
				// Get the scalar values
				float64 lFloat = pLMatrix->getScalar();
				float64 rFloat = pRMatrix->getScalar();

				// Perform the operation
				return new MatrixF64Obj(pow(lFloat, rFloat));
			}
		}
		break;

		// Array power
		case BinaryOpExpr::ARRAY_POWER:
		{
			// Evaluate the left and right expressions
			DataObject* pLeftVal = evalExpression(pLeftExpr, pEnv);
			DataObject* pRightVal = evalExpression(pRightExpr, pEnv);

			// Perform the array power operation
			return arrayArithOp<PowOp>(pLeftVal, pRightVal);
		}
		break;

		// Equality comparison
		case BinaryOpExpr::EQUAL:
		{
			// Evaluate the left and right expressions
			DataObject* pLeftVal = evalExpression(pLeftExpr, pEnv);
			DataObject* pRightVal = evalExpression(pRightExpr, pEnv);

			// Perform an equality comparison between the objects
			return matrixLogicOp<EqualOp>(pLeftVal, pRightVal);
		}
		break;

		// Inequality comparison
		case BinaryOpExpr::NOT_EQUAL:
		{
			// Evaluate the left and right expressions
			DataObject* pLeftVal = evalExpression(pLeftExpr, pEnv);
			DataObject* pRightVal = evalExpression(pRightExpr, pEnv);

			// Perform an inequality comparison between the objects
			return matrixLogicOp<NotEqualOp>(pLeftVal, pRightVal);
		}
		break;

		// Less-than comparison
		case BinaryOpExpr::LESS_THAN:
		{
			// Evaluate the left and right expressions
			DataObject* pLeftVal = evalExpression(pLeftExpr, pEnv);
			DataObject* pRightVal = evalExpression(pRightExpr, pEnv);

			// Perform a less-than comparison between the objects
			return matrixLogicOp<LessThanOp>(pLeftVal, pRightVal);
		}
		break;

		// Less-than or equal comparison
		case BinaryOpExpr::LESS_THAN_EQ:
		{
			// Evaluate the left and right expressions
			DataObject* pLeftVal = evalExpression(pLeftExpr, pEnv);
			DataObject* pRightVal = evalExpression(pRightExpr, pEnv);

			// Perform a less-than or equal comparison between the objects
			return matrixLogicOp<LessThanEqOp>(pLeftVal, pRightVal);
		}
		break;

		// Greater-than comparison
		case BinaryOpExpr::GREATER_THAN:
		{
			// Evaluate the left and right expressions
			DataObject* pLeftVal = evalExpression(pLeftExpr, pEnv);
			DataObject* pRightVal = evalExpression(pRightExpr, pEnv);

			// Perform a greater-than comparison between the objects
			return matrixLogicOp<GreaterThanOp>(pLeftVal, pRightVal);
		}
		break;

		// Greater-than or equal comparison
		case BinaryOpExpr::GREATER_THAN_EQ:
		{
			// Evaluate the left and right expressions
			DataObject* pLeftVal = evalExpression(pLeftExpr, pEnv);
			DataObject* pRightVal = evalExpression(pRightExpr, pEnv);

			// Perform a greater-than or equal comparison between the objects
			return matrixLogicOp<GreaterThanEqOp>(pLeftVal, pRightVal);
		}
		break;

		// Logical OR
		case BinaryOpExpr::OR:
		{
			// Evaluate the left expression
			DataObject* pLeftVal = evalExpression(pLeftExpr, pEnv);

			// If the left value evaluates to true
			if (getBoolValue(pLeftVal) == true)
			{
				// Return a true value
				return new LogicalArrayObj(1);
			}

			// Evaluate the right expression
			DataObject* pRightVal = evalExpression(pRightExpr, pEnv);

			// If the right value evaluates to true
			if (getBoolValue(pRightVal) == true)
			{
				// Return a true value
				return new LogicalArrayObj(1);
			}

			// Return a false value
			return new LogicalArrayObj(0);
		}
		break;

		// Array OR
		case BinaryOpExpr::ARRAY_OR:
		{
			// Evaluate the left and right expressions
			DataObject* pLeftVal = evalExpression(pLeftExpr, pEnv);
			DataObject* pRightVal = evalExpression(pRightExpr, pEnv);
			
			// Perform the array OR operation
			return matrixLogicOp<OrOp>(pLeftVal, pRightVal);
		}
		break;

		// Logical AND
		case BinaryOpExpr::AND:
		{
			// Evaluate the left expression
			DataObject* pLeftVal = evalExpression(pLeftExpr, pEnv);

			// If the left value evaluates to false
			if (getBoolValue(pLeftVal) == false)
			{
				// Return a false value
				return new LogicalArrayObj(0);
			}

			// Evaluate the right expression
			DataObject* pRightVal = evalExpression(pRightExpr, pEnv);

			// If the right value evaluates to true
			if (getBoolValue(pRightVal) == false)
			{
				// Return a true value
				return new LogicalArrayObj(0);
			}

			// Return a true value
			return new LogicalArrayObj(1);
		}
		break;

		// Array AND
		case BinaryOpExpr::ARRAY_AND:
		{
			// Evaluate the left and right expressions
			DataObject* pLeftVal = evalExpression(pLeftExpr, pEnv);
			DataObject* pRightVal = evalExpression(pRightExpr, pEnv);

			// Perform the array AND operation
			return matrixLogicOp<AndOp>(pLeftVal, pRightVal);
		}
		break;

		// Any other operator type
		default:
		{
			// Throw an exception
			throw RunError("unhandled binary expression", pExpr);
		}
	}

	// If we get to this point, no value was produced
	// Throw a descriptive run-time exception
	throw RunError("unsupported operand types in binary expression", pExpr);
}

/***************************************************************
* Function: Interpreter::evalRangeExpr()
* Purpose : Evaluate a range expression
* Initial : Maxime Chevalier-Boisvert on February 16, 2009
****************************************************************
Revisions and bug fixes:
*/
DataObject* Interpreter::evalRangeExpr(const RangeExpr* pExpr, Environment* pEnv, bool expand)
{
	// If this is the full range
	if (pExpr->isFullRange())
	{
		// If we should expand this range
		if (expand)
		{
			// Throw an exception
			throw RunError("cannot expand full range");
		}

		// Return the full range
		return new RangeObj(RangeObj::FULL_RANGE);
	}

	// Evaluate the start, step and end expressions
	DataObject* pStartVal = evalExpression(pExpr->getStartExpr(), pEnv);
	DataObject* pStepVal  = evalExpression(pExpr->getStepExpr(), pEnv);
	DataObject* pEndVal   = evalExpression(pExpr->getEndExpr(), pEnv);

	// Attempt to get floating-point values for the range parameters
	double startVal = getFloat64Value(pStartVal);
	double stepVal  = getFloat64Value(pStepVal);
	double endVal   = getFloat64Value(pEndVal);

	// Create a range object to store the range
	RangeObj* pRangeObj = new RangeObj(startVal, stepVal, endVal);

	// If the range should be expanded
	if (expand)
	{
		// Return the expanded range
		return pRangeObj->expand();
	}
	else
	{
		// Return the range object
		return pRangeObj;
	}
}

/***************************************************************
* Function: Interpreter::evalEndExpr()
* Purpose : Evaluate a range end expression
* Initial : Maxime Chevalier-Boisvert on March 2, 2009
****************************************************************
Revisions and bug fixes:
*/
DataObject* Interpreter::evalEndExpr(const EndExpr* pExpr, Environment* pEnv)
{
	// Get a reference to the association vector
	const EndExpr::AssocVector& assocs = pExpr->getAssocs();

	// For each association
	for (EndExpr::AssocVector::const_iterator itr = assocs.begin(); itr != assocs.end(); ++itr)
	{
		// Get a reference to this association
		const EndExpr::Assoc& assoc = *itr;

		// Lookup the symbol for this association
		DataObject* pObject = evalSymbol(assoc.pSymbol, pEnv);

		// If the object is a matrix
		if (pObject->isMatrixObj())
		{
			// Get a pointer to the matrix
			BaseMatrixObj* pMatrix = (BaseMatrixObj*)pObject;

			// Get a reference to the size vector of the matrix
			const DimVector& matSize = pMatrix->getSize();

			// If the dimension index is invalid, throw an exception
			if (assoc.dimIndex >= matSize.size())
				throw RunError("invalid indexing dimension");

			// Get the size of the matrix along this dimension
			size_t dimSize = matSize[assoc.dimIndex];

			// If we are assuming this is the last matrix dimension
			if (assoc.lastDim)
			{
				// Compute the size of the extended last dimension
				for (size_t i = assoc.dimIndex + 1; i < matSize.size(); ++i)
					dimSize *= matSize[i];
			}

			// Return the dimension size
			return new MatrixF64Obj(dimSize);
		}
	}

	// No association was found, throw a run-time error
	throw RunError("Range end expression does not associate with any matrix");
}

/***************************************************************
* Function: Interpreter::evalMatrixExpr()
* Purpose : Evaluate a parameterized expression
* Initial : Maxime Chevalier-Boisvert on January 24, 2009
****************************************************************
Revisions and bug fixes:
*/
DataObject* Interpreter::evalMatrixExpr(const MatrixExpr* pExpr, Environment* pEnv)
{
	// Get the rows of the matrix expression
	const MatrixExpr::RowVector& rows = pExpr->getRows();

	// Declare a pointer for the result (vertical) matrix
	BaseMatrixObj* pVertMatrix = NULL;

	// For each row of the matrix expression
	for (MatrixExpr::RowVector::const_iterator rowItr = rows.begin(); rowItr != rows.end(); ++rowItr)
	{
		// Get a reference to this row
		const MatrixExpr::Row& row = *rowItr;

		// Declare a pointer for the matrix associated with this row (horizontal)
		BaseMatrixObj* pHorzMatrix = NULL;

		// For each element in this row
		for (MatrixExpr::Row::const_iterator colItr = row.begin(); colItr != row.end(); ++colItr)
		{
			// Get a reference to this expression
			const Expression* pExpr = *colItr;

			// Evaluate this expression
			DataObject* pObject = evalExpression(pExpr, pEnv);

			// Declare a matrix to store this object
			BaseMatrixObj* pObjMatrix;

			// If this is a matrix object
			if (pObject->isMatrixObj())
			{
				// Store the matrix directly
				pObjMatrix = (BaseMatrixObj*)pObject;
			}

			// Otherwise
			else
			{
				// Throw an exception
				throw RunError("unsupported data type in matrix expression");
			}

			// If this is the first object in this row
			if (pHorzMatrix == NULL)
			{
				// Use the object matrix as the initial object
				pHorzMatrix = pObjMatrix;
			}
			else
			{
				// Concatenate the object matrix into the horizontal matrix
				pHorzMatrix = pHorzMatrix->concat(pObjMatrix, 1);
			}
		}

		// If this is the first row
		if (pVertMatrix == NULL)
		{
			// Use the row matrix as the initial result matrix
			pVertMatrix = pHorzMatrix;
		}
		else
		{
			// Concatenate the row matrix into the vertical matrix
			pVertMatrix = pVertMatrix->concat(pHorzMatrix, 0);
		}
	}

	// If there is no result matrix
	if (pVertMatrix == NULL)
	{
		// Return an empty matrix
		return new MatrixF64Obj();
	}

	// Return the result matrix
	return pVertMatrix;
}

/***************************************************************
* Function: Interpreter::evalCellArrayExpr()
* Purpose : Evaluate a cell array expression
* Initial : Maxime Chevalier-Boisvert on February 23, 2009
****************************************************************
Revisions and bug fixes:
*/
DataObject* Interpreter::evalCellArrayExpr(const CellArrayExpr* pExpr, Environment* pEnv)
{
	// Get the rows of the cell array expression
	const CellArrayExpr::RowVector& rows = pExpr->getRows();

	// Declare a pointer for the result (vertical) cell array
	CellArrayObj* pVertArray = NULL;

	// For each row of the cell array expression
	for (CellArrayExpr::RowVector::const_iterator rowItr = rows.begin(); rowItr != rows.end(); ++rowItr)
	{
		// Get a reference to this row
		const CellArrayExpr::Row& row = *rowItr;

		// Declare a pointer for the cell array associated with this row (horizontal)
		CellArrayObj* pHorzArray = NULL;

		// For each element in this row
		for (CellArrayExpr::Row::const_iterator colItr = row.begin(); colItr != row.end(); ++colItr)
		{
			// Get a reference to this expression
			const Expression* pExpr = *colItr;

			// Evaluate this expression
			DataObject* pObject = evalExpression(pExpr, pEnv);

			// Wrap the object inside a cell array
			CellArrayObj* pWrapperArray = new CellArrayObj(pObject->copy());

			// If this is the first object in this row
			if (pHorzArray == NULL)
			{
				// Use the wrapper object as the initial object
				pHorzArray = pWrapperArray;
			}
			else
			{
				// Concatenate the wrapper array into the horizontal array
				pHorzArray = CellArrayObj::concat(pHorzArray, pWrapperArray, 1);
			}
		}

		// If this is the first row
		if (pVertArray == NULL)
		{
			// Use the row array as the initial result array
			pVertArray = pHorzArray;
		}
		else
		{
			// Concatenate the row array into the vertical array
			pVertArray = CellArrayObj::concat(pVertArray, pHorzArray, 0);
		}
	}

	// If there is no result array
	if (pVertArray == NULL)
	{
		// Return an empty cell array
		return new CellArrayObj();
	}

	// Return the resulting array
	return pVertArray;
}

/***************************************************************
* Function: Interpreter::evalParamExpr()
* Purpose : Evaluate a parameterized expression
* Initial : Maxime Chevalier-Boisvert on November 13, 2008
****************************************************************
Revisions and bug fixes:
*/
DataObject* Interpreter::evalParamExpr(const ParamExpr* pExpr, Environment* pEnv, size_t nargout)
{
	// Get a pointer to the symbol expression
	SymbolExpr* pSymExpr = pExpr->getSymExpr();

	// Get a reference to the argument vector
	const ParamExpr::ExprVector& argVector = pExpr->getArguments();

	// Evaluate the symbol expression
	DataObject* pObject = evalSymbol(pSymExpr, pEnv);

	// If the object is a function handle
	if (pObject->getType() == DataObject::FN_HANDLE)
	{
		// Get a typed pointer to the function handle
		FnHandleObj* pHandle = (FnHandleObj*)pObject;

		// Extract the function pointer
		pObject = (DataObject*)pHandle->getFunction();
	}

	// If the object is a function
	if (pObject->getType() == DataObject::FUNCTION)
	{
		// Create an array object for the arguments
		ArrayObj* pArguments = new ArrayObj(argVector.size());

		// For each argument
		for (ParamExpr::ExprVector::const_iterator itr = argVector.begin(); itr != argVector.end(); ++itr)
		{
			// Get a typed pointer to the expression
			Expression* pExpr = *itr;

			// If this is a cell indexing expression
			if (pExpr->getExprType() == Expression::CELL_INDEX)
			{
				// Evaluate the cell indexing expression directly
				ArrayObj* pArrayObj = (ArrayObj*)evalCellIndexExpr((CellIndexExpr*)pExpr, pEnv);

				// Append the resulting elements to the arguments
				ArrayObj::append(pArguments, pArrayObj);
			}
			else
			{
				// Evaluate the expression
				DataObject* pValue = evalExpression(pExpr, pEnv);

				// Add the value to the argument array
				ArrayObj::addObject(pArguments, pValue);
			}
		}

		// Get a typed pointer to the function object
		Function* pFunction = (Function*)pObject;

		// If the function is a program function
		if (pFunction->isProgFunction())
		{
			// Get a typed pointer to the program function
			ProgFunction* pProgFunc = (ProgFunction*)pFunction;

			// If the function is nested
			if (pProgFunc->getParent() != NULL)
			{
				// Set the function's local env. to the current evaluation environment
				ProgFunction::setLocalEnv(pProgFunc, pEnv);
			}
		}
		
		// Call the function
		ArrayObj* pResult = callFunction(pFunction, pArguments, nargout);

		// Return the result
		return pResult;
	}

	// Otherwise, if the object is a matrix
	else if (pObject->isMatrixObj())
	{
		// Get a typed pointer to the matrix
		BaseMatrixObj* pMatrix = (BaseMatrixObj*)pObject;

		// Evaluate the indexing arguments
		ArrayObj* pArguments = evalIndexArgs(argVector, pEnv);

		// If some of the indices are not valid
		if (pMatrix->validIndices(pArguments) == false)
		{
			// Throw an exception
			throw RunError("invalid indices in matrix indexing");
		}

		// Get the maximum indices
		DimVector maxInds = pMatrix->getMaxIndices(pArguments);

		// If bounds checking fails for these indices
		if (pMatrix->boundsCheckND(maxInds) == false)
		{
			// Throw an exception
			throw RunError("index out of bounds in matrix rhs indexing", pExpr);
		}

		// Attempt to get a sub-matrix slice using the indexing arguments
		BaseMatrixObj* pSubMatrix = pMatrix->getSliceND(pArguments);

		// Return the sub-matrix
		return pSubMatrix;
	}

	// Otherwise
	else
	{
		// Throw an exception
		throw RunError("invalid operator in parameterized expression");
	}
}

/***************************************************************
* Function: Interpreter::evalCellIndexExpr()
* Purpose : Evaluate a cell indexing expression
* Initial : Maxime Chevalier-Boisvert on February 18, 2009
****************************************************************
Revisions and bug fixes:
*/
DataObject* Interpreter::evalCellIndexExpr(const CellIndexExpr* pExpr, Environment* pEnv)
{
	// Get a pointer to the symbol expression
	SymbolExpr* pSymExpr = pExpr->getSymExpr();

	// Get a reference to the argument vector
	const ParamExpr::ExprVector& argVector = pExpr->getArguments();

	// Evaluate the symbol expression
	DataObject* pObject = evalSymbol(pSymExpr, pEnv);

	// If the object is not a cell array
	if (pObject->getType() != DataObject::CELLARRAY)
	{
		// Throw an exception
		throw RunError("non-cellarray object in cell array indexing");
	}

	// Get a typed pointer to the cell array
	CellArrayObj* pCellArray = (CellArrayObj*)pObject;

	// Evaluate the indexing arguments
	ArrayObj* pArguments = evalIndexArgs(argVector, pEnv);

	// Get the maximum indices
	DimVector maxInds = pCellArray->getMaxIndices(pArguments);

	// If bounds checking fails for these indices
	if (pCellArray->boundsCheckND(maxInds) == false)
	{
		// Throw an exception
		throw RunError("index out of bounds in cell array indexing");
	}

	// Attempt to get a sub-matrix slice using the indexing arguments
	CellArrayObj* pSubArray = pCellArray->getSliceND(pArguments);

	// Create an array object to store the values extracted
	ArrayObj* pValues = new ArrayObj(pSubArray->getNumElems());

	// Add copies of each object in the sub-array to the array object
	for (size_t i = 1; i <= pSubArray->getNumElems(); ++i)
		ArrayObj::addObject(pValues, pSubArray->getElem1D(i)->copy());

	// Return the array of values
	return pValues;
}

/***************************************************************
* Function: Interpreter::evalFnHandleExpr()
* Purpose : Evaluate a function handle expression
* Initial : Maxime Chevalier-Boisvert on February 25, 2009
****************************************************************
Revisions and bug fixes:
*/
DataObject* Interpreter::evalFnHandleExpr(const FnHandleExpr* pExpr, Environment* pEnv)
{
	// Get the symbol expression
	SymbolExpr* pSymbol = pExpr->getSymbolExpr();

	// Evaluate the symbol expression
	DataObject* pObject = evalSymbol(pSymbol, pEnv);

	// If the object is not a function
	if (pObject->getType() != DataObject::FUNCTION)
	{
		// Throw an exception
		throw RunError("cannot create handle to non-function object");
	}

	// Get a typed pointer to the function
	Function* pFunction = (Function*)pObject;

	// If the function is a program function
	if (pFunction->isProgFunction())
	{
		// Get a typed pointer to the program function
		ProgFunction* pProgFunc = (ProgFunction*)pFunction;

		// If the function is nested
		if (pProgFunc->getParent())
		{
			// Create a copy of the function
			pProgFunc = pProgFunc->copy();

			// Set the function's local env. to the current evaluation environment
			ProgFunction::setLocalEnv(pProgFunc, pEnv);

			// Set the parent pointer to NULL, the closure is no longer nested
			pProgFunc->setParent(NULL);

			// Set the closure flag
			pProgFunc->setClosure(true);
			
			// Update the function pointer
			pFunction = pProgFunc;
		}
	}

	// Create and return a function handle object
	return new FnHandleObj(pFunction);
}

/***************************************************************
* Function: Interpreter::evalLambdaExpr()
* Purpose : Evaluate a lambda expression
* Initial : Maxime Chevalier-Boisvert on February 25, 2009
****************************************************************
Revisions and bug fixes:
*/
DataObject* Interpreter::evalLambdaExpr(const LambdaExpr* pExpr, Environment* pEnv)
{
	// Create a copy of the body expression
	Expression* pBodyExpr = pExpr->getBodyExpr()->copy();

	// Create a symbol for the output
	SymbolExpr* pOutSym = SymbolExpr::getSymbol("out");

	// Create a body where the expression is assigned to an output symbol
	StmtSequence* pStmtSeq = new StmtSequence(
		new AssignStmt(
			pOutSym,
			pBodyExpr
		)
	);

	// Get the input variables for the lambda expression
	const LambdaExpr::ParamVector& inVars = pExpr->getInParams();

	// Create vectors for the input and output parameters
	ProgFunction::ParamVector inParams;
	ProgFunction::ParamVector outParams;

	// Copy the input variables into the input parameter vector
	for (LambdaExpr::ParamVector::const_iterator itr = inVars.begin(); itr != inVars.end(); ++itr)
		inParams.push_back((*itr)->copy());

	// Add the output symbol to the output parameters
	outParams.push_back(pOutSym);

	// Create a program function
	ProgFunction* pFunction = new ProgFunction(
		"",
		inParams,
		outParams,
		ProgFunction::FuncVector(),
		pStmtSeq,
		false,
		true
	);

	// Set the function's local environment to a copy of the current environment
	ProgFunction::setLocalEnv(pFunction, pEnv->copy());

	// Return a handle to the new function object
	return new FnHandleObj(pFunction);
}

/***************************************************************
* Function: Interpreter::evalSymbolExpr()
* Purpose : Evaluate a symbol expression
* Initial : Maxime Chevalier-Boisvert on June 15, 2009
****************************************************************
Revisions and bug fixes:
*/
DataObject* Interpreter::evalSymbolExpr(const SymbolExpr* pExpr, Environment* pEnv, size_t nargout)
{
	// Evaluate the symbol
	DataObject* pResult = evalSymbol((SymbolExpr*)pExpr, pEnv);

	// If the result is a function
	if (pResult->getType() == DataObject::FUNCTION)
	{
		// Get a typed reference to the function
		Function* pFunction = (Function*)pResult;

		// Call the function with no arguments to obtain the desired result
		// Note: this is done to handle calls without explicit parenthesizing
		pResult = callFunction(pFunction, new ArrayObj(), nargout);
	}
	
	// Return the evaluation result
	return pResult;
}

/***************************************************************
* Function: Interpreter::evalSymbol()
* Purpose : Evaluate a symbol in a given environment
* Initial : Maxime Chevalier-Boisvert on January 30, 2009
****************************************************************
Revisions and bug fixes:
*/
DataObject* Interpreter::evalSymbol(const SymbolExpr* pExpr, Environment* pEnv)
{
	// Lookup the symbol
	DataObject* pObject = Environment::lookup(pEnv, pExpr);
	
	// If the symbol is not found
	if (pObject == NULL)
	{
		// If verbose mode is enabled
		if (ConfigManager::s_verboseVar.getBoolValue())
		{
			// Log that the symbol was not found
			std::cout << "Symbol not found: \"" + pExpr->toString() + "\"" << std::endl;
		}

		// Attempt to load the m-file with this name, if any
		loadMFile(pExpr->getSymName() + ".m");

		// Lookup the symbol a second time
		pObject = Environment::lookup(pEnv, pExpr);

		// If the symbol is still not found
		if (pObject == NULL)
		{
			// Throw an exception
			throw RunError("symbol lookup failed: \"" + pExpr->getSymName() + "\"");
		}
	}

	// Return the object bound to this symbol
	return pObject;
}

/***************************************************************
* Function: Interpreter::loadMFile()
* Purpose : Load an m-file by name
* Initial : Maxime Chevalier-Boisvert on November 16, 2008
****************************************************************
Revisions and bug fixes:
*/
CompUnits Interpreter::loadMFile(const std::string& fileName, const bool bindScript)
{
	// If verbose mode is enabled
	if (ConfigManager::s_verboseVar.getBoolValue())
	{
		// Log that we are loading an m-file
		std::cout << "Loading m-file: \"" << fileName << "\"" << std::endl;
	}

	// Tokenize the file name string
	std::vector<std::string> tokens = tokenize(fileName, "\\/.", false, true);

	// If there are less than two tokens, or no extension is present
	if (tokens.size() < 2 || tokens.back() != "m")
	{
		// Throw an exception
		throw RunError("invalid m-file name: \"" + fileName + "\"");
	}

	// Extract the script/function name token
	std::string nameToken = tokens[tokens.size() - 2];

	// Attempt to parse a file with this name in the current working directory
	CompUnits nodes = CodeParser::parseSrcFile(fileName);

	// Load the compilation units
	return loadCompUnits(nodes, nameToken, bindScript);
}

/***************************************************************
* Function: Interpreter::loadSrcText()
* Purpose : Load source code from a command string
* Initial : Maxime Chevalier-Boisvert on November 16, 2008
****************************************************************
Revisions and bug fixes:Nurudeen Lameed on May 18, 2009
*/
CompUnits Interpreter::loadSrcText(const std::string& srcText, const std::string& unitName, const bool bindScript)
{
	// Attempt to parse a file with this name in the current working directory
	CompUnits nodes = CodeParser::parseSrcText(srcText);

	// Load the compilation units
	return loadCompUnits(nodes, unitName, bindScript);
}

/***************************************************************
* Function: Interpreter::loadCompUnits()
* Purpose : Loading compilation units
* Initial : Maxime Chevalier-Boisvert on November 16, 2008
****************************************************************
Revisions and bug fixes:Nurudeen Lameed on May 18, 2009
*/
CompUnits Interpreter::loadCompUnits(CompUnits& nodes, const std::string& nameToken, const bool bindScript)
{
	// If there is no output
	if (nodes.size() == 0)
	{
		// Throw an exception
		throw RunError("no IIR nodes produced as a result of parsing");
	}

	// Create a vector to store the parsed functions
	std::vector<ProgFunction*> funcVec;

	// For each IIR node produced
	for (CompUnits::iterator nodeItr = nodes.begin(); nodeItr != nodes.end(); ++nodeItr)
	{
		// Get a pointer to this IIR node
		IIRNode* pNode = *nodeItr;

		// If this node is not a function
		if (pNode->getType() != IIRNode::FUNCTION || ((Function*)pNode)->isProgFunction() == false)
		{
			// Throw an exception
			throw RunError("parsing did not result in function");
		}

		// Get a typed pointer to the function
		ProgFunction* pFunction = (ProgFunction*)pNode;

		// If this is the first function
		if (nodeItr == nodes.begin())
		{
			// If this function is a script
			if (pFunction->isScript())
			{
				// Set the function name to the m-file name
				pFunction->setFuncName(nameToken);
			}

			// If this is not a script, or the bind script flag is true
			if (pFunction->isScript() == false || bindScript == true)
			{
				// Add a new binding for this function in the global environment
				Environment::bind(&s_globalEnv, SymbolExpr::getSymbol(pFunction->getFuncName()), (DataObject*)pFunction);
			}
		}

		// Add the function to the list
		funcVec.push_back(pFunction);
	}

	// Create a local environment for the functions
	Environment* pLocalEnv = Environment::extend(&s_globalEnv);

	// For each parsed function
	for (std::vector<ProgFunction*>::iterator funcItr = funcVec.begin(); funcItr != funcVec.end(); ++funcItr)
	{
		// Get a pointer to this function
		ProgFunction* pFunction = *funcItr;

		// Bind this function in the local environment
		Environment::bind(pLocalEnv, SymbolExpr::getSymbol(pFunction->getFuncName()), (DataObject*)pFunction);
	}

	// For each parsed function
	for (std::vector<ProgFunction*>::iterator funcItr = funcVec.begin(); funcItr != funcVec.end(); ++funcItr)
	{
		// Get a pointer to this function
		ProgFunction* pFunction = *funcItr;

		// If this function is a script
		if (pFunction->isScript())
		{
			// Make the global environment the function's local environment
			ProgFunction::setLocalEnv(pFunction, &s_globalEnv);
		}
		else
		{			
			// Build the local environment for this function
			buildLocalEnv(pFunction, pLocalEnv);
		}
	}

	// Return the list of compilation units
	return nodes;
}

/***************************************************************
* Function: Interpreter::buildLocalEnv()
* Purpose : Build the local environment for a function
* Initial : Maxime Chevalier-Boisvert on June 29, 2009
****************************************************************
Revisions and bug fixes:
*/
void Interpreter::buildLocalEnv(ProgFunction* pFunction, Environment* pLocalEnv)
{
	// Increment the function loaded count
	PROF_INCR_COUNTER(Profiler::FUNC_LOAD_COUNT);
	
	// Create a copy of the local environment for this function
	Environment* pFuncEnv = pLocalEnv->copy();

	// Get the list of nested function
	const ProgFunction::FuncVector& nestedFuncs = pFunction->getNestedFuncs();

	// For each nested function
	for (ProgFunction::FuncVector::const_iterator nestItr = nestedFuncs.begin(); nestItr != nestedFuncs.end(); ++nestItr)
	{
		// Get a pointer to the nested function
		ProgFunction* pFunction = *nestItr;

		// Bind this function in the local environment
		Environment::bind(pFuncEnv, SymbolExpr::getSymbol(pFunction->getFuncName()), (DataObject*)pFunction);
	}

	// Set the local environment for this function
	ProgFunction::setLocalEnv(pFunction, pFuncEnv);
	
	// For each nested function
	for (ProgFunction::FuncVector::const_iterator nestItr = nestedFuncs.begin(); nestItr != nestedFuncs.end(); ++nestItr)
	{
		// Get a pointer to the nested function
		ProgFunction* pFunction = *nestItr;
		
		// Create an extension of the local environment for this function
		Environment* pNestedEnv = Environment::extend(pLocalEnv);

		// Build the local environment for this function
		buildLocalEnv(pFunction, pNestedEnv);
	}	
}

/***************************************************************
* Function: Interpreter::setBinding()
* Purpose : Set a binding in the global environment
* Initial : Maxime Chevalier-Boisvert on January 28, 2009
****************************************************************
Revisions and bug fixes:
*/
void Interpreter::setBinding(const std::string& name, DataObject* pObject)
{
	// Ensure the name is valid
	assert (name != "");

	// Ensure the object pointer is valid
	assert (pObject != NULL);

	// Perform the binding operation
	Environment::bind(&s_globalEnv, SymbolExpr::getSymbol(name), pObject);
}

/***************************************************************
* Function: Interpreter::getGlobalSyms()
* Purpose : Get the symbols bound in the global environment
* Initial : Maxime Chevalier-Boisvert on July 11, 2009
****************************************************************
Revisions and bug fixes:
*/
Environment::SymbolVec Interpreter::getGlobalSyms()
{
	// Return the symbols bound in the global environment
	return s_globalEnv.getSymbols();
}

/***************************************************************
* Function: Interpreter::evalGlobalSym()
* Purpose : Evaluate a symbol in the global environment
* Initial : Maxime Chevalier-Boisvert on July 11, 2009
****************************************************************
Revisions and bug fixes:
*/
DataObject* Interpreter::evalGlobalSym(SymbolExpr* pSymbol)
{
	// Evaluate the symbol in the global environment
	return evalSymbol(pSymbol, &s_globalEnv);
}

/***************************************************************
* Function: Interpreter::clearProgFuncs()
* Purpose : Clear the program functions from the global environment
* Initial : Maxime Chevalier-Boisvert on February 22, 2009
****************************************************************
Revisions and bug fixes:
*/
void Interpreter::clearProgFuncs()
{
	// Get the symbols bound in the global environment
	Environment::SymbolVec symbols = s_globalEnv.getSymbols();

	// For each symbol
	for (Environment::SymbolVec::iterator itr = symbols.begin(); itr != symbols.end(); ++itr)
	{
		// Get a pointer to the symbol
		SymbolExpr* pSymbol = *itr;

		// Lookup the symbol in the global environment
		DataObject* pObject = Environment::lookup(&s_globalEnv, pSymbol);

		// If the object is a function
		if (pObject->getType() == DataObject::FUNCTION && ((Function*)pObject)->isProgFunction())
		{
			// Remove this binding from the global environment
			Environment::unbind(&s_globalEnv, pSymbol);
		}
	}
}
