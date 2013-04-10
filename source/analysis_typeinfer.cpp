// =========================================================================== //
//                                                                             //
// Copyright 2009 Maxime Chevalier-Boisvert and McGill University.             //
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
#include <cassert>
#include <iostream>
#include "analysis_typeinfer.h"
#include "functions.h"
#include "constexprs.h"
#include "interpreter.h"
#include "profiling.h"

/***************************************************************
* Function: computeTypeInfo(ProgFunction, ...)
* Purpose : Perform type inference on a function body
* Initial : Maxime Chevalier-Boisvert on April 17, 2009
****************************************************************
Revisions and bug fixes:
*/
AnalysisInfo* computeTypeInfo(
	const ProgFunction* pFunction,
	const StmtSequence* pFuncBody,
	const TypeSetString& inArgTypes,
	bool returnBottom
)
{	
	// Create a type inference info object
	TypeInferInfo* pTypeInferInfo = new TypeInferInfo();
	
	// Get the input and output parameters for the function
	const ProgFunction::ParamVector& inParams = pFunction->getInParams();
	const ProgFunction::ParamVector& outParams = pFunction->getOutParams();
	
	// Ensure that the length of the argument type string is valid
	assert (inArgTypes.size() <= inParams.size());
	
	// If we are in verbose mode
	if (ConfigManager::s_verboseVar)
	{
		// Log that we are performing the analysis
		std::cout << "Performing type inference analysis" << std::endl;
	
		// Log info about the input argument types
		for (size_t i = 0; i < inArgTypes.size(); ++i)
		{
			std::cout << "Arg \"" << inParams[i]->toString() << "\"" << std::endl;
			
			const TypeSet& typeSet = inArgTypes[i];
			
			for (TypeSet::const_iterator itr = typeSet.begin(); itr != typeSet.end(); ++itr)
				std::cout << itr->toString() << std::endl;		
		}
	}
	
	// If we should return bottom
	if (returnBottom)
	{
		// Set the possible output types to empty sets
		pTypeInferInfo->outArgTypes.resize(outParams.size());
		
		// Return the type inference info object
		return pTypeInferInfo;	
	}
	
	// std::cout << "Performing type inference on: " << pFunction->getFuncName() << std::endl;
	// double startTime = Profiler::getTimeSeconds();
	
	// Create a variable type map object for the initial map
	VarTypeMap initialMap;
	
	// Set the initial types of the input parameters in the initial map
	for (size_t i = 0; i < inArgTypes.size(); ++i)
		initialMap[inParams[i]] = inArgTypes[i];
	
	// Perform a reaching definition analysis on the function body
	const ReachDefInfo* pReachDefInfo = (ReachDefInfo*)AnalysisManager::requestInfo(
		&computeReachDefs,
		pFunction,
		pFuncBody,
		inArgTypes
	);
	
	// Perform a live variable analysis on the function body
	const LiveVarInfo* pLiveVarInfo = (LiveVarInfo*)AnalysisManager::requestInfo(
		&computeLiveVars,
		pFunction,
		pFuncBody,
		inArgTypes
	);
	
	// Declare variables for the exit, return, break and continue points
	VarTypeMap exitPoint;
	TypeMapVector retPoints;
	TypeMapVector breakPoints;
	TypeMapVector contPoints;
	
	// Perform type inference on the function body
	inferTypes(
		pFuncBody,
		pReachDefInfo->reachDefMap,
		pLiveVarInfo->liveVarMap,
		ProgFunction::getLocalEnv(pFunction),
		initialMap,
		exitPoint,
		retPoints,
		breakPoints, 
		contPoints,
		pTypeInferInfo->preTypeMap,
		pTypeInferInfo->postTypeMap,
		pTypeInferInfo->exprTypeMap
	);
		
	// Ensure that there are no unmatched break or continue points
	assert (breakPoints.empty() && contPoints.empty());
	
	// Add the exit point to the list of return points
	retPoints.push_back(exitPoint);
	
	// Compute the union of all return point type maps
	pTypeInferInfo->exitTypeMap = typeMapVectorUnion(retPoints);	
	
	// Resize the output type string to match the number of output arguments
	pTypeInferInfo->outArgTypes.resize(outParams.size());
	
	// For each output argument
	for (size_t i = 0; i < outParams.size(); ++i)
	{
		// Attempt to find this variable in the exit type map
		VarTypeMap::const_iterator varItr = pTypeInferInfo->exitTypeMap.find(outParams[i]);
		
		// If no type information was found
		if (varItr == pTypeInferInfo->exitTypeMap.end())
		{
			// If we are in verbose mode
			if (ConfigManager::s_verboseVar)
			{
				// Output a warning
				std::cout << "WARNING: type-inference analysis suggests output param \"";
				std::cout << outParams[i]->toString() << "\" may be unassigned" << std::endl;
			}
		}
		else
		{
			// Store the possible types for this output parameter
			pTypeInferInfo->outArgTypes[i] = varItr->second;
		}
	}
	
	// If we are in verbose mode
	if (ConfigManager::s_verboseVar)
	{	
		// Log that the analysis is complete
		std::cout << "Type inference analysis complete" << std::endl;
	}
	
	// std::cout << "Type inference time: " << (Profiler::getTimeSeconds() - startTime) << " s (" << pFunction->getFuncName() << ")" << std::endl;

	// Return the type inference info
	return pTypeInferInfo;
}

/***************************************************************
* Function: inferTypes(StmtSequence, ...)
* Purpose : Perform type inference on a statement sequence
* Initial : Maxime Chevalier-Boisvert on April 17, 2009
****************************************************************
Revisions and bug fixes:
*/
void inferTypes(
	const StmtSequence* pStmtSeq,
	const ReachDefMap& reachDefs,
	const LiveVarMap& liveVars,
	Environment* pLocalEnv,
	const VarTypeMap& startMap,
	VarTypeMap& exitMap,
	TypeMapVector& retPoints,
	TypeMapVector& breakPoints, 
	TypeMapVector& contPoints,
	TypeInfoMap& preTypeMap,
	TypeInfoMap& postTypeMap,
	ExprTypeMap& exprTypeMap
)
{
	// Get a reference to the statement vector
	const StmtSequence::StmtVector& stmts = pStmtSeq->getStatements();
	
	// Initialize the current variable type map with the start map
	VarTypeMap curMap = startMap;
	
	// Store the set before this statement sequence
	preTypeMap[pStmtSeq] = curMap;
	
	// For each statement, in reverse order
	for (StmtSequence::StmtVector::const_iterator stmtItr = stmts.begin(); stmtItr != stmts.end(); ++stmtItr)
	{
		// Get a pointer to the statement
		const Statement* pStmt = *stmtItr;
		
		// Reduce and store the variable type map before this statement
		preTypeMap[pStmt] = curMap;
		
		// Switch on the statement type
		switch (pStmt->getStmtType())
		{
			// Break statement
			case Statement::BREAK:
			{
				// Add the definitions of the current map to the break points
				breakPoints.push_back(curMap);
			}
			break;

			// Continue statement
			case Statement::CONTINUE:
			{
				// Add the definitions of the current map to the continue points
				contPoints.push_back(curMap);
			}
			break;

			// Return statement
			case Statement::RETURN:
			{
				// Add the definitions of the current map to the return points
				retPoints.push_back(curMap);
			}
			break;

			// Assignment statement
			case Statement::ASSIGN:
			{
				// Declare a map for the type info after the statement
				VarTypeMap exitMap;
				
				// Find the reaching definition information for this statement
				ReachDefMap::const_iterator defItr = reachDefs.find(pStmt);
				assert (defItr != reachDefs.end());
				
				// Infer the output types for this assignment statement
				inferTypes(
					(const AssignStmt*)pStmt,
					defItr->second,
					pLocalEnv,
					curMap,
					exprTypeMap
				);
			}
			break;
			
			// Expression statement
			case Statement::EXPR:
			{
				// Get a typed pointer to the statement
				ExprStmt* pExprStmt = (ExprStmt*)pStmt;
				
				// Find the reaching definition information for this statement
				ReachDefMap::const_iterator defItr = reachDefs.find(pStmt);
				assert (defItr != reachDefs.end());
				
				// Perform type inference for the expression
				inferTypes(
					pExprStmt->getExpression(),
					defItr->second,
					pLocalEnv,
					curMap,
					exprTypeMap
				);
			}
			break;
			
			// If-else statement
			case Statement::IF_ELSE:
			{
				// Declare a map for the type info after the statement
				VarTypeMap exitMap;
				
				// Get the type info map for the if-else statement
				inferTypes(
					(const IfElseStmt*)pStmt,
					reachDefs,
					liveVars,
					pLocalEnv,
					curMap,
					exitMap,
					retPoints,
					breakPoints,
					contPoints,
					preTypeMap,
					postTypeMap,
					exprTypeMap
				);		
				
				// Update the current variable type map
				curMap = exitMap;
				
				/*
				// Find the live variable information for this statement
				LiveVarMap::const_iterator liveItr = liveVars.find(pStmt);
				assert (liveItr != liveVars.end());				
				varTypeMapReduce(curMap);
				*/
			}
			break;

			// Loop statement
			case Statement::LOOP:
			{				
				// Declare a map for the type info after the statement
				VarTypeMap exitMap;
								
				// Get the type info map for the loop statement
				inferTypes(
					(const LoopStmt*)pStmt,
					reachDefs,
					liveVars,
					pLocalEnv,
					curMap,
					exitMap,
					retPoints,
					preTypeMap,
					postTypeMap,
					exprTypeMap
				);
											
				// Update the current variable type map
				curMap = exitMap;
				
				/*
				// Find the live variable information for this statement
				LiveVarMap::const_iterator liveItr = liveVars.find(pStmt);
				assert (liveItr != liveVars.end());				
				varTypeMapReduce(curMap);
				*/
			}
			break;
			
			// Other statement types
			default:
			{
				// Do nothing, no definitions created
			}
		}
		
		// Reduce and store the variable type map after this statement
		postTypeMap[pStmt] = curMap;
	}
	
	// Store the type map at the exit point
	exitMap = curMap;
	
	// Store the set after this statement sequence
	postTypeMap[pStmtSeq] = exitMap;
}

/***************************************************************
* Function: inferTypes(IfElseStmt)
* Purpose : Perform type inference for an if-else statement
* Initial : Maxime Chevalier-Boisvert on April 20, 2009
****************************************************************
Revisions and bug fixes:
*/
void inferTypes(
	const IfElseStmt* pIfStmt,
	const ReachDefMap& reachDefs,
	const LiveVarMap& liveVars,
	Environment* pLocalEnv,
	const VarTypeMap& startMap,
	VarTypeMap& exitMap,
	TypeMapVector& retPoints,
	TypeMapVector& breakPoints, 
	TypeMapVector& contPoints,
	TypeInfoMap& preTypeMap,
	TypeInfoMap& postTypeMap,
	ExprTypeMap& exprTypeMap
)
{
	// Get the condition expression
	Expression* pTestExpr = pIfStmt->getCondition();
	
	// Store the variable type map at the condition expression 
	preTypeMap[pTestExpr] = startMap;
	postTypeMap[pTestExpr] = startMap;

	// Create variable type maps for the exit points of the if and else blocks
	VarTypeMap ifVarTypes;
	VarTypeMap elseVarTypes;
	
	// Get the type information for the if block statements
	inferTypes(
		pIfStmt->getIfBlock(),
		reachDefs,
		liveVars,
		pLocalEnv,
		startMap,
		ifVarTypes,
		retPoints,
		breakPoints,
		contPoints,
		preTypeMap,
		postTypeMap,
		exprTypeMap
	);

	// Get the type information for the else block statements
	inferTypes(
		pIfStmt->getElseBlock(),
		reachDefs,
		liveVars,
		pLocalEnv,
		startMap,
		elseVarTypes,
		retPoints,
		breakPoints, 
		contPoints,
		preTypeMap,
		postTypeMap,
		exprTypeMap
	);
	
	// Compute the exit variable type map as the union of the maps of the if and else blocks
	exitMap = varTypeMapUnion(ifVarTypes, elseVarTypes);
}

/***************************************************************
* Function: inferTypes(LoopStmt)
* Purpose : Perform type inference for a loop statement
* Initial : Maxime Chevalier-Boisvert on April 20, 2009
****************************************************************
Revisions and bug fixes:
*/
void inferTypes(
	const LoopStmt* pLoopStmt,
	const ReachDefMap& reachDefs,
	const LiveVarMap& liveVars,
	Environment* pLocalEnv,
	const VarTypeMap& startMap,
	VarTypeMap& exitMap,
	TypeMapVector& retPoints,
	TypeInfoMap& preTypeMap,
	TypeInfoMap& postTypeMap,
	ExprTypeMap& exprTypeMap
)
{
	//std::cout << "Inferring loop: " << pLoopStmt->toString() << std::endl;
	
	// Declare type maps for the initialization exit variable type info
	VarTypeMap initExitMap;
	
	// Get the type info map for the initialization block
	TypeMapVector initRetPoints;
	TypeMapVector initBreakPoints;
	TypeMapVector initContPoints;
	inferTypes(
		pLoopStmt->getInitSeq(),
		reachDefs,
		liveVars,
		pLocalEnv,
		startMap,
		initExitMap,
		initRetPoints,
		initBreakPoints, 
		initContPoints,
		preTypeMap,
		postTypeMap,
		exprTypeMap
	);
	assert (initRetPoints.empty() && initBreakPoints.empty() && initContPoints.empty());
	
	// Initialize the current incrementation exit map to the value of the init exit map
	VarTypeMap curIncrExitMap = initExitMap;
	
	// Until we have reached a fixed point
	for (;;)
	{
		// Compute the test start map as the union of the init exit and incr exit maps
		VarTypeMap testStartMap = varTypeMapUnion(initExitMap, curIncrExitMap);		

		// Declare a map for the test exit reaching definitions
		VarTypeMap testExitMap;
				
		// Get the type info map for the test block
		TypeMapVector testRetPoints;
		TypeMapVector testBreakPoints;
		TypeMapVector testContPoints;
		inferTypes(
			pLoopStmt->getTestSeq(),
			reachDefs,
			liveVars,
			pLocalEnv,
			testStartMap,
			testExitMap,
			testRetPoints,
			testBreakPoints, 
			testContPoints,
			preTypeMap,
			postTypeMap,
			exprTypeMap
		);
		assert (testRetPoints.empty() && testBreakPoints.empty() && testContPoints.empty());

		// Declare variables for the body exit points
		VarTypeMap bodyExitMap;
		TypeMapVector breakPoints;
		TypeMapVector contPoints;
			
		// Get the live variable map for the body block
		inferTypes(
			pLoopStmt->getBodySeq(),
			reachDefs,
			liveVars,
			pLocalEnv,
			testExitMap,
			bodyExitMap,
			retPoints,
			breakPoints,
			contPoints,
			preTypeMap,
			postTypeMap,
			exprTypeMap
		);
	
		// Add the test exit map to the break point list
		breakPoints.push_back(testExitMap);
		
		// Add the body exit map to the continue point list
		contPoints.push_back(bodyExitMap);		
		
		// Compute the incr start map as the union of all the continue maps
		VarTypeMap incrStartMap = typeMapVectorUnion(contPoints);

		// Declare a map for the incrementation block exit types
		VarTypeMap incrExitMap;
		
		// Get the live variable map for the incrementation block
		TypeMapVector incrRetPoints;
		TypeMapVector incrBreakPoints;
		TypeMapVector incrContPoints;
		inferTypes(
			pLoopStmt->getIncrSeq(),
			reachDefs,
			liveVars,
			pLocalEnv,
			incrStartMap,
			incrExitMap,
			incrRetPoints,
			incrBreakPoints,
			incrContPoints,
			preTypeMap,
			postTypeMap,
			exprTypeMap
		);
		assert (incrRetPoints.empty() && incrBreakPoints.empty() && incrContPoints.empty());
				
		// Update the current exit map
		exitMap = typeMapVectorUnion(breakPoints);
		
		// If a fixed point has been reached, stop
		if (incrExitMap == curIncrExitMap)
			break;
		
		// Update the incrementation block exit types
		curIncrExitMap = incrExitMap;
	}
}

/***************************************************************
* Function: inferTypes(AssignStmt)
* Purpose : Perform type inference for an assignment statement
* Initial : Maxime Chevalier-Boisvert on April 20, 2009
****************************************************************
Revisions and bug fixes:
*/
void inferTypes(
	const AssignStmt* pAssignStmt,
	const VarDefMap& reachDefs,
	Environment* pLocalEnv,
	VarTypeMap& varTypes,
	ExprTypeMap& exprTypeMap
)
{
	//std::cout << "Inferring types for assignment statement" << std::endl;
	
	// Get a pointer to the right expression
	const Expression* pRightExpr = pAssignStmt->getRightExpr();
	
	// Perform type inference for the right expression
	TypeSetString typeSetStr = inferTypes(
		pRightExpr,
		reachDefs,
		pLocalEnv,
		varTypes,
		exprTypeMap
	);

	// Get the vector of left-side expressions
	const AssignStmt::ExprVector& leftExprs = pAssignStmt->getLeftExprs();
	
	// If there are too many left-side expression
	if (leftExprs.size() > typeSetStr.size())
	{
		// Erase the type information
		typeSetStr.clear();
		typeSetStr.resize(leftExprs.size(), TypeSet());
	}
		
	// For each left-side expression
	for (size_t i = 0; i < leftExprs.size(); ++i)
	{
		// Get a pointer to the expression
		Expression* pLeftExpr = leftExprs[i];
		
		// Get the possible types for the corresponding rhs value
		TypeSet rhsValTypes = typeSetReduce(typeSetStr[i]);
		
		// Switch on the expression type
		switch (pLeftExpr->getExprType())
		{
			// Symbol expression
			case Expression::SYMBOL:
			{
				// Update the type information for this symbol
				varTypes[(SymbolExpr*)pLeftExpr] = rhsValTypes;				
			}
			break;

			// Parameterized expression
			case Expression::PARAM:
			{
				// Get a typed pointer to the expression
				ParamExpr* pParamExpr = (ParamExpr*)pLeftExpr;
				
				//std::cout << "Analyzing lhs param expr args" << std::endl;
				
				// Analyze the matrix indexing argument types
				size_t numIndexDims;
				bool isScalarIndexing;
				bool isMatrixIndexing;
				analyzeIndexTypes(
					pParamExpr->getArguments(),
					reachDefs,
					pLocalEnv,
					varTypes,
					exprTypeMap,
					numIndexDims,
					isScalarIndexing,
					isMatrixIndexing
				);
				
				//std::cout << "Done analyzing lhs param expr args" << std::endl;
				
				// Get the current type set for the symbol
				TypeSet& typeSet = varTypes[pParamExpr->getSymExpr()];
				
				// Declare a type set for the output
				TypeSet outSet;
				
				// For each type in the set
				for (TypeSet::iterator itr = typeSet.begin(); itr != typeSet.end(); ++itr)
				{
					// Get the type info object
					TypeInfo type = *itr;
					
					// If this is a matrix object
					if (type.getObjType() >= DataObject::MATRIX_I32 && type.getObjType() <= DataObject::CELLARRAY)
					{
						// Test whether or not the matrix can be guaranteed to be 2D
						type.set2D(type.is2D() && numIndexDims <= 2);
						
						// We can no longer assume the matrix is scalar
						type.setScalar(false);

						// Test whether or not the rhs value can be guaranteed to be integer
						bool rhsInteger = (rhsValTypes.size() > 0);
						for (TypeSet::const_iterator typeItr = rhsValTypes.begin(); typeItr != rhsValTypes.end(); ++typeItr)
							if (typeItr->isInteger() == false) rhsInteger = false;
						
						// Test whether or not the matrix can be guaranteed to be integer
						type.setInteger(type.isInteger() && rhsInteger);
						
						// The size of the matrix is no longer known
						type.setSizeKnown(false);
						
						// If this is a cell array
						if (type.getObjType() == DataObject::CELLARRAY)
						{
							// If we know nothing about the rhs types
							if (rhsValTypes.empty())
							{
								// Reset the cell types
								type.setCellTypes(TypeSet());
							}
							else
							{
								// Declare a variable to tell if this is the first cell array encountered
								bool firstCell = true;
								
								// Declare a set for the possible cell types
								TypeSet cellTypes;
								
								// For each possible rhs type
								for (TypeSet::const_iterator typeItr = rhsValTypes.begin(); typeItr != rhsValTypes.end(); ++typeItr)
								{
									// If this is a cell array
									if (typeItr->getObjType() == DataObject::CELLARRAY)
									{
										// Update the potential cell array types
										if (firstCell)
											cellTypes = typeItr->getCellTypes();
										else
											typeSetUnion(cellTypes, typeItr->getCellTypes());
												
										// Update the first cell flag
										firstCell = false;
									}
								}
								
								// If the lhs cell array is not empty, include its possible types in the union
								if (type.getSizeKnown() == false || type.getMatSize() != TypeInfo::DimVector(2, 0))
									cellTypes = typeSetUnion(cellTypes, type.getCellTypes());
								
								// Reduce the set of possible cell stored types
								cellTypes = typeSetReduce(cellTypes);
								
								// Update the possible cell array types
								type.setCellTypes(cellTypes);
							}
						}
						
						// Otherwise, if this is not a cell array
						else
						{							
							// Test whether or not the rhs value can be guaranteed to not be complex
							bool rhsNotComplex = (rhsValTypes.size() > 0);
							for (TypeSet::const_iterator typeItr = rhsValTypes.begin(); typeItr != rhsValTypes.end(); ++typeItr)
								if (typeItr->getObjType() == DataObject::MATRIX_C128) rhsNotComplex = false;
							
							// If the rhs value cannot be guaranteed not to be complex  
							if (rhsNotComplex == false)
							{
								// Add the equivalent complex type to the output set
								TypeInfo complexType = type;
								complexType.setObjType(DataObject::MATRIX_C128);
								outSet.insert(complexType);
							}
						}
					}
					
					// Add the updated type to the output type set
					outSet.insert(type);
				}
				
				//std::cout << "Done analyzing lhs param expr" << std::endl;
				
				// Update the type set for this symbol
				varTypes[pParamExpr->getSymExpr()] = outSet;
			}
			break;
			
			// Cell-indexing expression
			case Expression::CELL_INDEX:
			{
				//std::cout << "Inferring cell-indexing assignment lhs" << std::endl;
				
				// Get a typed pointer to the expression
				CellIndexExpr* pCellExpr = (CellIndexExpr*)pLeftExpr;
				
				// Analyze the matrix indexing argument types
				size_t numIndexDims;
				bool isScalarIndexing;
				bool isMatrixIndexing;
				analyzeIndexTypes(
					pCellExpr->getArguments(),
					reachDefs,
					pLocalEnv,
					varTypes,
					exprTypeMap,
					numIndexDims,
					isScalarIndexing,
					isMatrixIndexing
				);
				
				// Get the current type set for the symbol
				TypeSet& typeSet = varTypes[pCellExpr->getSymExpr()];
				
				// Declare a type set for the output
				TypeSet outSet;
				
				// For each type in the set
				for (TypeSet::iterator itr = typeSet.begin(); itr != typeSet.end(); ++itr)
				{
					// Get the type info object
					TypeInfo type = *itr;
				
					// If this is a cell array
					if (type.getObjType() == DataObject::CELLARRAY)
					{
						// Test whether or not the matrix can be guaranteed to be 2D
						type.set2D(type.is2D() && numIndexDims <= 2);
						
						// We can no longer assume the matrix is scalar
						type.setScalar(false);
				
						// The size of the matrix is no longer known
						type.setSizeKnown(false);
						
						// Declare a type set for the possible cell stored types
						TypeSet cellTypes;
							
						// If the type set string is empty
						if (rhsValTypes.empty())
						{
							// Let the cell type set be empty
						}						
												
						// Otherwise, if the lhs cell array is not empty
						else if (type.getSizeKnown() == false || type.getMatSize() != TypeInfo::DimVector(2, 0))
						{
							// Perform the union of the current cell types and the rhs types
							cellTypes = typeSetUnion(typeSetStr[0], type.getCellTypes());
						}
						
						// Otherwise, the lhs cell array is empty
						else			
						{
							// Store the rhs types as the potential cell types directly
							cellTypes = rhsValTypes;
						}
							
						// Reduce the set of possible cell stored types
						cellTypes = typeSetReduce(cellTypes);
						
						// Update the possible cell stored types
						type.setCellTypes(cellTypes);
					}	
				}
				
				// Update the type set for this symbol
				varTypes[pCellExpr->getSymExpr()] = outSet;
				
				//std::cout << "Done inferring cell-indexing assignment lhs" << std::endl;
			}
			break;
		
			// Other expression types
			default:
			{
				// Do nothing
			}
		}
	}
	
	//std::cout << "Done inferring types for assignment statement" << std::endl;
}

/***************************************************************
* Function: inferTypes(Expression)
* Purpose : Perform type inference for an expression
* Initial : Maxime Chevalier-Boisvert on April 21, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeSetString inferTypes(
	const Expression* pExpression,
	const VarDefMap& reachDefs,
	Environment* pLocalEnv,
	const VarTypeMap& varTypes,
	ExprTypeMap& exprTypeMap
)
{
	// Create a type set string to store the output types
	TypeSetString outTypes;
	
	// Switch on the expression type
	switch (pExpression->getExprType())
	{
		// Parameterized expression
		case Expression::PARAM:
		{
			// Perform type inference for the parameterized expression
			outTypes = inferTypes(
				(const ParamExpr*)pExpression,
				reachDefs,
				pLocalEnv,
				varTypes,
				exprTypeMap
			);
		}
		break;
			
		// Cell indexing expression
		case Expression::CELL_INDEX:
		{
			// Perform type inference for the cell indexing expression
			outTypes = inferTypes(
				(const CellIndexExpr*)pExpression,
				reachDefs,
				pLocalEnv,
				varTypes,
				exprTypeMap
			);
		}
		break;
		
		// Binary expression
		case Expression::BINARY_OP:
		{
			// Perform type inference for the binary expression
			outTypes = inferTypes(
				(const BinaryOpExpr*)pExpression,
				reachDefs,
				pLocalEnv,
				varTypes,
				exprTypeMap
			);
		}
		break;
			
		// Unary expression
		case Expression::UNARY_OP:
		{
			// Perform type inference for the unary expression
			outTypes = inferTypes(
				(const UnaryOpExpr*)pExpression,
				reachDefs,
				pLocalEnv,
				varTypes,
				exprTypeMap
			);
		}
		break;
			
		// Symbol expression
		case Expression::SYMBOL:
		{
			// Perform type inference for the symbol expression
			outTypes = inferTypes(
				(const SymbolExpr*)pExpression,
				reachDefs,
				pLocalEnv,
				varTypes,
				exprTypeMap
			);
		}
		break;
			
		// Integer constant
		case Expression::INT_CONST:
		{
			// Return the type info for the integer constant
			outTypes = typeSetStrMake(TypeInfo(
				DataObject::MATRIX_F64,
				true,
				true,
				true,
				true,
				TypeInfo::DimVector(2, 1),
				NULL,
				TypeSet()
			));
		}
		break;
			
		// Floating-point constant
		case Expression::FP_CONST:
		{
			// Return the type info for the floating-point constant
			outTypes = typeSetStrMake(TypeInfo(
				DataObject::MATRIX_F64,
				true,
				true,
				false,
				true,
				TypeInfo::DimVector(2, 1),
				NULL,
				TypeSet()
			));
		}
		break;
			
		// String constant
		case Expression::STR_CONST:
		{
			// Get a typed pointer to the expression
			StrConstExpr* pStrConst = (StrConstExpr*)pExpression;
			
			// Store the resultign matrix size
			TypeInfo::DimVector matSize;
			matSize.push_back(1);
			matSize.push_back(pStrConst->getValue().length());
			
			// Return the type info for the floating-point constant
			outTypes = typeSetStrMake(TypeInfo(
				DataObject::CHARARRAY,
				true,
				(pStrConst->getValue().length() == 1),
				true,
				true,
				matSize,
				NULL,
				TypeSet()
			));			
		}
		break;
			
		// Range expression
		case Expression::RANGE:
		{
			// Perform type inference for the range expression
			outTypes = inferTypes(
				(const RangeExpr*)pExpression,
				reachDefs,
				pLocalEnv,
				varTypes,
				exprTypeMap
			);
		}
		break;
			
		// End expression
		case Expression::END:
		{
			// End acts as an integer constant
			outTypes = typeSetStrMake(TypeInfo(
				DataObject::MATRIX_F64,
				true,
				true,
				true,
				true,
				TypeInfo::DimVector(2, 1),
				NULL,
				TypeSet()
			));
		}
		break;

		// Matrix expression
		case Expression::MATRIX:
		{
			// Perform type inference for the matrix expression
			outTypes = inferTypes(
				(const MatrixExpr*)pExpression,
				reachDefs,
				pLocalEnv,
				varTypes,
				exprTypeMap
			);
		}
		break;
			
		// Cell array expression
		case Expression::CELLARRAY:
		{
			// Perform type inference for the cell array expression
			outTypes = inferTypes(
				(const CellArrayExpr*)pExpression,
				reachDefs,
				pLocalEnv,
				varTypes,
				exprTypeMap
			);
		}
		break;
			
		// Function handle expression
		case Expression::FN_HANDLE:
		{
			// Perform type inference for the function handle expression
			outTypes = inferTypes(
				(const FnHandleExpr*)pExpression,
				reachDefs,
				pLocalEnv,
				varTypes,
				exprTypeMap
			);
		}
		break;
		
		// Lambda form expression
		case Expression::LAMBDA:
		{
			// Return no type information
			outTypes = TypeSetString();
		}
		break;
		
		// Other expression types
		default:
		{
			// Return no type information
			outTypes = TypeSetString();
		}
	}
	
	//std::cout << "Done inferring expr: " << pExpression->toString() << std::endl;
	
	// If there is no entry for this expression in the expression type map
	if (exprTypeMap.find(pExpression) == exprTypeMap.end())
	{
		// Create an initial entry for this expression
		exprTypeMap[pExpression] = outTypes;
	}
	else
	{
		// Get the types currently associated with this expression
		TypeSetString& exprTypes = exprTypeMap[pExpression];
		
		// Resize the expression types to contain all output types, if necessary
		exprTypes.resize(std::max(exprTypes.size(), outTypes.size()));
		
		// Perform the union of the current types and the new output types
		for (size_t i = 0; i < outTypes.size(); ++i)
			exprTypes[i] = typeSetUnion(exprTypes[i], outTypes[i]);
	}
	
	// Return the output types
	return outTypes;
}

/***************************************************************
* Function: inferTypes(ParamExpr)
* Purpose : Perform type inference for a parameterized expr
* Initial : Maxime Chevalier-Boisvert on April 22, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeSetString inferTypes(
	const ParamExpr* pParamExpr,
	const VarDefMap& reachDefs,
	Environment* pLocalEnv,
	const VarTypeMap& varTypes,
	ExprTypeMap& exprTypeMap
)
{
	//std::cout << "Inferring parameterized expression" << std::endl;
	
	// Get the symbol expression
	SymbolExpr* pSymbol = pParamExpr->getSymExpr();
	
	// Lookup the symbol in the variable type map
	VarTypeMap::const_iterator varTypeItr = varTypes.find(pSymbol);
	
	// Get a reference to the argument vector 
	const ParamExpr::ExprVector& argVector = pParamExpr->getArguments();
		
	// Analyze the matrix indexing argument types
	size_t numIndexDims;
	bool isScalarIndexing;
	bool isMatrixIndexing;
	analyzeIndexTypes(
		argVector,
		reachDefs,
		pLocalEnv,
		varTypes,
		exprTypeMap,
		numIndexDims,
		isScalarIndexing,
		isMatrixIndexing
	);
	
	//std::cout << "Scalar indexing: " << isScalarIndexing << std::endl;
	
	// Declare a set for the potential callee functions
	std::set<Function*> calleeSet;
	
	// Declare a set for the possible output values
	std::set<TypeSetString> outputSet;
	
	// If there is type information for this variable
	if (varTypeItr != varTypes.end() && varTypeItr->second.empty() == false)
	{
		// Get a reference to the type set
		const TypeSet& typeSet = varTypeItr->second;		
		
		// Declare a type set for the output
		TypeSet outSet;
		
		// For each type in the set
		for (TypeSet::const_iterator itr = typeSet.begin(); itr != typeSet.end(); ++itr)
		{
			// Get the type info object
			TypeInfo type = *itr;
			
			// If this is a matrix object
			if (type.getObjType() >= DataObject::MATRIX_I32 && type.getObjType() <= DataObject::CHARARRAY)
			{
				// Add type information about the resulting matrix
				outSet.insert(TypeInfo(
					type.getObjType(),
					(numIndexDims <= 2) && !isMatrixIndexing,
					isScalarIndexing,
					type.isInteger(),
					isScalarIndexing,
					isScalarIndexing? TypeInfo::DimVector(2, 1):TypeInfo::DimVector(),
					NULL,
					TypeSet()
				));	
			}
			
			// Otherwise, if this is a cell array
			else if (type.getObjType() == DataObject::CELLARRAY)
			{
				// Add type information about the resulting cell array
				outSet.insert(TypeInfo(
					type.getObjType(),
					(numIndexDims <= 2) && !isMatrixIndexing,
					isScalarIndexing,
					false,
					isScalarIndexing,
					isScalarIndexing? TypeInfo::DimVector(2, 1):TypeInfo::DimVector(),
					NULL,
					type.getCellTypes()
				));
			}
			
			// If this is a function handle
			else if (type.getObjType() == DataObject::FN_HANDLE)
			{
				// Get the function the handle points to
				Function* pFunc = type.getFunction(); 
				
				// If the handle is to an unknown function, return no information
				if (pFunc == NULL)
					return TypeSetString();
				
				// Add the function to the potential callee set
				calleeSet.insert(pFunc);
			}
		}
		
		// Add the possible output values to the set
		outputSet.insert(TypeSetString(1, outSet));	
	}
	
	// Lookup the symbol in the reaching definitions map
	VarDefMap::const_iterator varDefItr = reachDefs.find(pSymbol);
	
	//std::cout << "Looked up symbol in reaching defs" << std::endl;
	
	// If we are in the case where the only reaching definition comes from the local env.
	if (varDefItr != reachDefs.end() && varDefItr->second.size() == 1 && varDefItr->second.find(NULL) != varDefItr->second.end())
	{
		//std::cout << "SYMBOL COMES FROM LOCAL ENV: " << pSymbol->toString() << std::endl;
		
		// Declare a pointer for the object
		const DataObject* pObject;
		
		// Setup a try block to catch lookup errors
		try
		{			
			// Lookup the symbol in the environment
			pObject = Interpreter::evalSymbol(pSymbol, pLocalEnv);
		}
		catch (RunError e)
		{
			// Lookup failed, set the object pointer to NULL
			pObject = NULL;
		}
	
		//std::cout << "Looked up symbol in environment" << std::endl;
		
		// If the object is not a function, return no information
		if (pObject == NULL || pObject->getType() != DataObject::FUNCTION)
			return TypeSetString();
	
		// Get a typed pointer to the function
		Function* pFunction = (Function*)pObject;
		
		//std::cout << "Adding potential callee" << std::endl;
		
		// Add the function to the potential callee set
		calleeSet.insert(pFunction);
	}
	
	// If there are potential callees
	if (calleeSet.size() > 0)
	{
		// Declare type set strings for the function call arguments
		TypeSetString callArgs;
		
		// For each argument expression
		for (ParamExpr::ExprVector::const_iterator argItr = argVector.begin(); argItr != argVector.end(); ++argItr)
		{
			// Get a pointer to the argument expression
			Expression* pArgExpr = *argItr;
					
			// If this is a cell array indexing espression
			if (pArgExpr->getExprType() == Expression::CELL_INDEX)
			{
				// Return no information
				return TypeSetString();
			}
			
			// Any other expression type
			else
			{
				// Perform type inference for the expression
				TypeSetString outTypes = inferTypes(
					pArgExpr,
					reachDefs,
					pLocalEnv,
					varTypes,
					exprTypeMap
				);
				
				// If one of the arguments has unknown type, return no information
				if (outTypes.size() == 0)
					return TypeSetString();
				
				// If one of the latter arguments is an empty set, return no information
				for (size_t i = 1; i < outTypes.size(); ++i)
					if (outTypes[i].empty()) return TypeSetString();
				
				// Add the arguments to the call arguments
				callArgs.insert(callArgs.end(), outTypes.begin(), outTypes.end());
			}
		}
				
		// For each potential callee
		for (std::set<Function*>::iterator funcItr = calleeSet.begin(); funcItr != calleeSet.end(); ++funcItr)
		{
			// Get a pointer to the function
			Function* pFunction = *funcItr;
			
			// If this is a program function
			if (pFunction->isProgFunction())
			{
				// Get a typed pointer to the program function
				ProgFunction* pProgFunc = (ProgFunction*)pFunction;
	
				// Perform a type inference analysis on the function body
				const TypeInferInfo* pTypeInferInfo = (TypeInferInfo*)AnalysisManager::requestInfo(
					&computeTypeInfo,
					pProgFunc,
					pProgFunc->getCurrentBody(),
					callArgs
				);
				
				// Add the output type string to the potential output set
				outputSet.insert(pTypeInferInfo->outArgTypes);
			}
			else
			{
				// TODO: handle the "feval" case
				
				// Get a typed pointer to the library function
				LibFunction* pLibFunc = (LibFunction*)pFunction;
				
				// Get the type mapping function for this library function
				TypeMapFunc pTypeMapping = pLibFunc->getTypeMapping();
				
				// Get the potential output arguments for the call arguments
				TypeSetString outTypes = pTypeMapping(callArgs);

				// Add the output type string to the potential output set
				outputSet.insert(outTypes);
			}
		}
	}
		
	// If there are no possible outputs, return no information
	if (outputSet.empty())
		return TypeSetString();
	
	// Declare a type set string for the output types
	TypeSetString outputTypes;
	
	// Get an iterator to the potential output set
	std::set<TypeSetString>::iterator outItr = outputSet.begin();
	
	// Initialize the output type string to the first possible output
	outputTypes = *(outItr++);

	// For each possible output type string
	for (; outItr != outputSet.end(); ++outItr)
	{
		// Get a reference to the current type string
		const TypeSetString& curTypes = *outItr; 
		
		// If the type string lengths do not match, return no information
		if (curTypes.size() != outputTypes.size())
			return TypeSetString();
		
		// Perform the union of the two type strings
		for (size_t i = 0; i < outputTypes.size(); ++i)
			outputTypes[i] = typeSetUnion(outputTypes[i], curTypes[i]);
	}	
	
	// Return the output type string
	return outputTypes;
}

/***************************************************************
* Function: inferTypes(CellIndexExpr)
* Purpose : Perform type inference for a cell indexing expr
* Initial : Maxime Chevalier-Boisvert on April 23, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeSetString inferTypes(
	const CellIndexExpr* pCellExpr,
	const VarDefMap& reachDefs,
	Environment* pLocalEnv,
	const VarTypeMap& varTypes,
	ExprTypeMap& exprTypeMap
)
{
	// Get the symbol expression
	SymbolExpr* pSymbol = pCellExpr->getSymExpr();
	
	// Lookup the symbol in the variable type map
	VarTypeMap::const_iterator varTypeItr = varTypes.find(pSymbol);

	// If we have no information about this variable, return no information
	if (varTypeItr == varTypes.end())
		return TypeSetString();
	
	// Get a reference to the argument vector 
	const CellIndexExpr::ExprVector& argVector = pCellExpr->getArguments();
		
	// Analyze the matrix indexing argument types
	size_t numIndexDims;
	bool isScalarIndexing;
	bool isMatrixIndexing;
	analyzeIndexTypes(
		argVector,
		reachDefs,
		pLocalEnv,
		varTypes,
		exprTypeMap,
		numIndexDims,
		isScalarIndexing,
		isMatrixIndexing
	);
	
	// If this is not scalar indexing, return no information
	if (isScalarIndexing == false)
		return TypeSetString();
	
	// Reduce the type set for this variable so there is at most one cell array type
	TypeSet typeSet = typeSetReduce(varTypeItr->second);
	
	// Declare a type set for the possible output types
	TypeSet outTypes;
	
	// For each possible variable type
	for (TypeSet::const_iterator typeItr = typeSet.begin(); typeItr != typeSet.end(); ++typeItr)
	{
		// If this type is the cell array type
		if (typeItr->getObjType() == DataObject::CELLARRAY)
		{
			// Store the possible output types
			outTypes = typeItr->getCellTypes();
		}
	}
	
	// Return the possible output types
	return TypeSetString(1, outTypes);
}

/***************************************************************
* Function: inferTypes(BinaryOpExpr)
* Purpose : Perform type inference for a binary expression
* Initial : Maxime Chevalier-Boisvert on April 23, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeSetString inferTypes(
	const BinaryOpExpr* pBinaryExpr,
	const VarDefMap& reachDefs,
	Environment* pLocalEnv,
	const VarTypeMap& varTypes,
	ExprTypeMap& exprTypeMap
)
{
	// Perform type inference for the left expression
	TypeSetString leftTypes = inferTypes(
		pBinaryExpr->getLeftExpr(),
		reachDefs,
		pLocalEnv,
		varTypes,
		exprTypeMap
	);

	// Perform type inference for the right expression
	TypeSetString rightTypes = inferTypes(
		pBinaryExpr->getRightExpr(),
		reachDefs,
		pLocalEnv,
		varTypes,
		exprTypeMap
	);
	
	// Get the first left and right type sets
	TypeSet leftSet = leftTypes.empty()? TypeSet():leftTypes.front();
	TypeSet rightSet = rightTypes.empty()? TypeSet():rightTypes.front();
	
	// Add the argument types to a type set string
	TypeSetString argTypes;
	argTypes.push_back(leftSet);
	argTypes.push_back(rightSet);
	
	// Switch on the binary operator
	switch (pBinaryExpr->getOperator())
	{
		// Array arithmetic operation (int preserving)
		case BinaryOpExpr::PLUS:
		case BinaryOpExpr::MINUS:
		case BinaryOpExpr::ARRAY_MULT:
		case BinaryOpExpr::ARRAY_POWER:
		{
			// Perform type inference for the array operation
			return arrayArithOpTypeMapping<true>(argTypes);
		};
		break;
		
		// Array arithmetic operation (non int preserving)
		case BinaryOpExpr::ARRAY_DIV:
		case BinaryOpExpr::ARRAY_LEFT_DIV:
		{
			// Perform type inference for the array operation
			return arrayArithOpTypeMapping<false>(argTypes);
		}
		break;
		
		// Multiplication operation
		case BinaryOpExpr::MULT:
		{
			// Perform type inference for the multiplication operation
			return multOpTypeMapping(argTypes);
		}
		break;
		
		// Division operation
		case BinaryOpExpr::DIV:
		{
			// Perform type inference for the division operation
			return divOpTypeMapping(argTypes);
		}
		break;			
			
		// Left division operation
		case BinaryOpExpr::LEFT_DIV:
		{
			// Perform type inference for the division operation
			return leftDivOpTypeMapping(argTypes);
		}
		break;	
			
		// Exponentiation operation	
		case BinaryOpExpr::POWER:
		{
			// Perform type inference for the power operation
			return powerOpTypeMapping(argTypes);
		}
		break;

		// Logical arithmetic operation
		case BinaryOpExpr::EQUAL:
		case BinaryOpExpr::NOT_EQUAL:
		case BinaryOpExpr::LESS_THAN:
		case BinaryOpExpr::LESS_THAN_EQ:
		case BinaryOpExpr::GREATER_THAN:
		case BinaryOpExpr::GREATER_THAN_EQ:
		case BinaryOpExpr::ARRAY_OR:
		case BinaryOpExpr::ARRAY_AND:
		{
			// Perform type inference for the array logical operation
			return arrayLogicOpTypeMapping(argTypes);
		}
		break;
			
		// Logical OR/AND operation
		case BinaryOpExpr::OR:
		case BinaryOpExpr::AND:
		{
			// Return the type info for the logical value
			return typeSetStrMake(TypeInfo(
				DataObject::LOGICALARRAY,
				true,
				true,
				true,
				true,
				TypeInfo::DimVector(2, 1),
				NULL,
				TypeSet()
			));
		}
		break;
		
		// Other expression types
		default:
		{
			// Return no type information
			return TypeSetString();
		}
	}
}

/***************************************************************
* Function: inferTypes(UnaryOpExpr)
* Purpose : Perform type inference for a unary expression
* Initial : Maxime Chevalier-Boisvert on April 23, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeSetString inferTypes(
	const UnaryOpExpr* pUnaryExpr,
	const VarDefMap& reachDefs,
	Environment* pLocalEnv,
	const VarTypeMap& varTypes,
	ExprTypeMap& exprTypeMap
)
{
	// Perform type inference for the operand expression
	TypeSetString argTypes = inferTypes(
		pUnaryExpr->getOperand(),
		reachDefs,
		pLocalEnv,
		varTypes,
		exprTypeMap
	);
	
	// Get the argument type set
	TypeSet typeSet = argTypes.empty()? TypeSet():argTypes.front();
	
	// Add the argument type set to a type set string
	TypeSetString typeSetStr;
	typeSetStr.push_back(typeSet);
	
	// Switch on the unary operator
	switch (pUnaryExpr->getOperator())
	{
		// Plus operator
		case UnaryOpExpr::PLUS:
		{
			// Return the identity type mapping
			return identTypeMapping(typeSetStr);
		}
		break;
			
		// Arithmetic negation operation
		case UnaryOpExpr::MINUS:
		{
			// Perform type inference for the negation operation
			return minusOpTypeMapping(typeSetStr);
		}
		break;
		
		// Logical negation operation
		case UnaryOpExpr::NOT:
		{
			// Perform type inference for the negation operation
			return notOpTypeMapping(typeSetStr);
		}
		break;
		
		// Transpose operations
		case UnaryOpExpr::TRANSP:
		case UnaryOpExpr::ARRAY_TRANSP:
		{
			// Perform the type inference for the transpose operation
			return transpOpTypeMapping(typeSetStr);
		}
		break;
	
		// Other expression types
		default:
		{
			// Return no type information
			return TypeSetString();
		}
	}
}

/***************************************************************
* Function: inferTypes(SymbolExpr)
* Purpose : Perform type inference for a symbol expression
* Initial : Maxime Chevalier-Boisvert on May 12, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeSetString inferTypes(
	const SymbolExpr* pSymbolExpr,
	const VarDefMap& reachDefs,
	Environment* pLocalEnv,
	const VarTypeMap& varTypes,
	ExprTypeMap& exprTypeMap
)
{
	// Attempt to find a type set for this symbol
	VarTypeMap::const_iterator varItr = varTypes.find(pSymbolExpr);
	
	// If type information for this variable was found
	if (varItr != varTypes.end())
	{
		// Return the type information associated with this variable
		return TypeSetString(1, varItr->second);
	}
	
	// Lookup the symbol in the reaching definitions map
	VarDefMap::const_iterator varDefItr = reachDefs.find(pSymbolExpr);
				
	// If we are in the case where the only reaching definition comes from the local env.
	if (varDefItr != reachDefs.end() && varDefItr->second.size() == 1 && varDefItr->second.find(NULL) != varDefItr->second.end())
	{
		// Declare a pointer for the object
		const DataObject* pObject;
		
		// Setup a try block to catch lookup errors
		try
		{				
			// Lookup the symbol in the environment
			pObject = Interpreter::evalSymbol(pSymbolExpr, pLocalEnv);
		}
		catch (RunError e)
		{
			// Lookup failed, set the object pointer to NULL
			pObject = NULL;
		}
			
		// If the object is not a function 
		if (pObject == NULL || pObject->getType() != DataObject::FUNCTION)
		{
			// Return no information
			return TypeSetString();
		}

		// Get a typed pointer to the function
		Function* pFunction = (Function*)pObject;
			
		// If this is a program function
		if (pFunction->isProgFunction())
		{
			// Get a typed pointer to the program function
			ProgFunction* pProgFunc = (ProgFunction*)pFunction;
			
			// Perform a type inference analysis on the function body
			const TypeInferInfo* pTypeInferInfo = (TypeInferInfo*)AnalysisManager::requestInfo(
				&computeTypeInfo,
				pProgFunc,
				pProgFunc->getCurrentBody(),
				TypeSetString()
			);
			
			// Return the potential output arguments for the call arguments
			return pTypeInferInfo->outArgTypes;
		}
		else
		{
			// Get a typed pointer to the library function
			LibFunction* pLibFunc = (LibFunction*)pFunction;
			
			// Get the type mapping function for this library function
			TypeMapFunc pTypeMapping = pLibFunc->getTypeMapping();
			
			// Return the potential output arguments for the call arguments
			return pTypeMapping(TypeSetString());
		}				
	}
		
	// Return no information
	return TypeSetString();
}

/***************************************************************
* Function: inferTypes(RangeExpr)
* Purpose : Perform type inference for a range expression
* Initial : Maxime Chevalier-Boisvert on May 10, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeSetString inferTypes(
	const RangeExpr* pRangeExpr,
	const VarDefMap& reachDefs,
	Environment* pLocalEnv,
	const VarTypeMap& varTypes,
	ExprTypeMap& exprTypeMap
)
{
	// Get the start and step expressions
	Expression* pStartExpr = pRangeExpr->getStartExpr();
	Expression* pStepExpr = pRangeExpr->getStepExpr();
	
	// Perform type inference for the start expression
	TypeSetString startTypeStr = pStartExpr? inferTypes(
		pStartExpr,
		reachDefs,
		pLocalEnv,
		varTypes,
		exprTypeMap
	):TypeSetString();
		
	// Perform type inference for the step expression
	TypeSetString stepTypeStr = pStepExpr? inferTypes(
		pStepExpr,
		reachDefs,
		pLocalEnv,
		varTypes,
		exprTypeMap
	):TypeSetString();

	// Get the type sets for the start, step and end expressions
	TypeSet startTypeSet = startTypeStr.empty()? TypeSet():startTypeStr[0];
	TypeSet stepTypeSet = stepTypeStr.empty()? TypeSet():stepTypeStr[0];
	
	// Declare a flag to indicate that the range comprises only integer values
	bool isInteger = true;
	
	// If the start or step type sets are empty, we cannot guarantee the range is integer
	if (startTypeSet.empty() || stepTypeSet.empty())
		isInteger = false;
	
	// If some of the possible start or step types are non-integer, the range is not integer
	for (TypeSet::iterator typeItr = startTypeSet.begin(); typeItr != startTypeSet.end(); ++typeItr)
		if (typeItr->isInteger() == false) isInteger = false;
	for (TypeSet::iterator typeItr = stepTypeSet.begin(); typeItr != stepTypeSet.end(); ++typeItr)
		if (typeItr->isInteger() == false) isInteger = false;
	
	// Return the type info for a 2D F64 matrix of unknown size
	return typeSetStrMake(TypeInfo(
		DataObject::MATRIX_F64,
		true,
		false,
		isInteger,
		false,
		TypeInfo::DimVector(),
		NULL,
		TypeSet()
	));
}

/***************************************************************
* Function: inferTypes(MatrixExpr)
* Purpose : Perform type inference for a matrix expression
* Initial : Maxime Chevalier-Boisvert on April 23, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeSetString inferTypes(
	const MatrixExpr* pMatrixExpr,
	const VarDefMap& reachDefs,
	Environment* pLocalEnv,
	const VarTypeMap& varTypes,
	ExprTypeMap& exprTypeMap
)
{
	//std::cout << "Inferring matrix expression" << std::endl;
		
	// Get the rows of the matrix expression
	const MatrixExpr::RowVector& rows = pMatrixExpr->getRows();
	
	// If the expression is empty
	if (rows.empty() || rows[0].empty())
	{
		// Return the type info for an empty matrix
		return typeSetStrMake(TypeInfo(
			DataObject::MATRIX_F64,
			true,
			false,
			true,
			true,
			TypeInfo::DimVector(2, 0),
			NULL,
			TypeSet()
		));
	}

	// Declare a variable to tell if the size is known
	bool sizeKnown = true;
	
	// Declare a variable to tell if all arguments are integer
	bool allInteger = true;
	
	// Declare a variable to tell if a complex value was encountered
	bool complexArg = false;
	
	// Declare a variable to tell if an unknown type argument was encountered
	bool unknownArg = false;
	
	// Create a dimension vector to store the output matrix size
	TypeInfo::DimVector outMatSize(2, 0);
	
	// Declare a set for the possible first argument types
	std::set<DataObject::Type> firstType;
	
	// For each row of the matrix expression
	for (MatrixExpr::RowVector::const_iterator rowItr = rows.begin(); rowItr != rows.end(); ++rowItr)
	{
		// Get a reference to this row
		const MatrixExpr::Row& row = *rowItr;
	
		// For each element in this row
		for (MatrixExpr::Row::const_iterator colItr = row.begin(); colItr != row.end(); ++colItr)
		{
			// Get a reference to this expression
			const Expression* pExpr = *colItr;
			
			//std::cout << "Inferring argument type" << std::endl;
			
			// Perform type inference for the expression
			TypeSetString exprTypeStr = inferTypes(
				pExpr,
				reachDefs,
				pLocalEnv,
				varTypes,
				exprTypeMap
			);
			
			//std::cout << "Done inferring argument type:" << std::endl;
			//std::cout << ((exprTypeStr.empty() || exprTypeStr[0].empty())? "unknown":exprTypeStr[0].begin()->toString()) << std::endl;
			
			// If the expression type is unknown
			if (exprTypeStr.empty() || exprTypeStr[0].empty())
			{
				// If this is the first argument, return no information
				if (rowItr == rows.begin() && colItr == row.begin())
					return TypeSetString();

				// Set the unknown argument flag
				unknownArg = true;
				
				// Set the size known flag to false
				sizeKnown = false;
				
				// Set the all integer flag to false
				allInteger = false;
			}
			else
			{
				// Get a reference to the expression type set
				const TypeSet& argTypes = exprTypeStr[0];
							
				// Declare a variable to store the previous matrix size
				TypeInfo::DimVector prevSize;
				
				// For each possible type
				for (TypeSet::const_iterator typeItr = argTypes.begin(); typeItr != argTypes.end(); ++typeItr)
				{
					// Get the matrix size vector
					const TypeInfo::DimVector& matSize = typeItr->getMatSize();
					
					// If this is the first argument
					if (rowItr == rows.begin() && colItr == row.begin())
					{
						// Add the object type to the potential type set
						firstType.insert(typeItr->getObjType());
						
						// If this is the first possible type
						if (typeItr == argTypes.begin())
						{
							// Initialize the output matrix size
							outMatSize = matSize;
							outMatSize[0] = 0;
							outMatSize[1] = 0;
						}
					}				
					else
					{
						// If the output matrix does not have the same number of dimensions as the current size matrix
						if (outMatSize.size() != matSize.size())
						{
							// Mark the output size as unknown
							sizeKnown = false;
						}
						else
						{
							// For each dimension of the output matrix size, except the first two
							for (size_t i = 2; i < outMatSize.size(); ++i)
							{
								// If the dimension sizes do not match, the output size is unknown
								if (outMatSize[i] != matSize[i])
									sizeKnown = false;
							}
						}
					}
						
					// If this is the first possible type and the size is known
					if (typeItr == argTypes.begin() && typeItr->getSizeKnown())
					{
						// Ensure that the matrix size is valid
						assert (matSize.size() >= 2);
						
						// If this is the first element of the row
						if (colItr == row.begin())
						{
							// Update the number of rows
							outMatSize[0] += matSize[0];
						}
						
						// If we are in the first row
						if (rowItr == rows.begin())
						{
							// Update the number of columns
							outMatSize[1] += matSize[1];
						}
					}
					
					// If the size is unknown
					if (typeItr->getSizeKnown() == false)
					{
						// Set the size known flag to false
						sizeKnown = false;
					}
					else
					{
						// If this is not the first possible type
						if (typeItr != argTypes.begin())
						{
							// If the size does not match the previous size
							if (matSize != prevSize)
							{
								// Mark the output size as unknown
								sizeKnown = false;
							}
						}
						
						// Update the previous matrix size
						prevSize = matSize; 
					}
					
					// If the argument is non-integer
					if (typeItr->isInteger() == false)
					{
						// Set the integer flag to false
						allInteger = false;
					}
					
					// If the type is complex
					if (typeItr->getObjType() == DataObject::MATRIX_C128)
					{
						// Set the complex arg flag to true
						complexArg = true;
					}			
				}
			}
		}
	}
	
	/*
	std::cout << "Done examining matrix expr arguments" << std::endl;
	
	std::cout << "Out mat size: " << std::endl;
	if (outMatSize.size() != 2)
		std::cout << "not 2D" << std::endl;
	else
	{
		std::cout << outMatSize[0] << "x" << outMatSize[1] << std::endl;
	}
	*/
	
	// Determine if the output will is a 2D matrix
	bool is2D = (sizeKnown && outMatSize.size() == 2);

	/*
	std::cout << "Size known: " << sizeKnown << std::endl;
	std::cout << "Num out dims: " << outMatSize.size() << std::endl;
	std::cout << "is 2D: " << is2D << std::endl;
	*/
	
	// Determine if the output will be scalar
	bool isScalar = (sizeKnown && outMatSize == TypeInfo::DimVector(2, 1)); 
	
	// If an unknown argument was found and we have some type info, add the complex type as a possible output type
	if (unknownArg && firstType.empty() == false) firstType.insert(DataObject::MATRIX_C128);

	// TODO: do we know for sure that the output *will* be complex?
	// If a complex argument was found and we have some type info, add the complex type as a possible output type
	if (complexArg && firstType.empty() == false) firstType.insert(DataObject::MATRIX_C128);
	
	// Declare a set for the possible output types
	TypeSet outTypes;
	
	// For each possible output type
	for (std::set<DataObject::Type>::iterator typeItr = firstType.begin(); typeItr != firstType.end(); ++typeItr)
	{
		// Add a type object to the set
		outTypes.insert(TypeInfo(
			*typeItr,
			is2D,
			isScalar,
			allInteger,
			sizeKnown,
			outMatSize,
			NULL,
			TypeSet()
		));
	}
	
	// Return the possible output types
	return TypeSetString(1, outTypes);
}

/***************************************************************
* Function: inferTypes(CellArrayExpr)
* Purpose : Perform type inference for a cell array expression
* Initial : Maxime Chevalier-Boisvert on April 23, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeSetString inferTypes(
	const CellArrayExpr* pCellExpr,
	const VarDefMap& reachDefs,
	Environment* pLocalEnv,
	const VarTypeMap& varTypes,
	ExprTypeMap& exprTypeMap
)
{
	// Get the rows of the matrix expression
	const CellArrayExpr::RowVector& rows = pCellExpr->getRows();
	
	// If the expression is empty
	if (rows.empty() || rows[0].empty())
	{
		// Return the type info for an empty cell array
		return typeSetStrMake(TypeInfo(
			DataObject::CELLARRAY,
			true,
			false,
			false,
			true,
			TypeInfo::DimVector(2, 0),
			NULL,
			TypeSet()
		));
	}
	
	// Declare a variable to tell if an unknown type argument was encountered
	bool unknownArg = false;
	
	// Create a dimension vector to store the output matrix size
	TypeInfo::DimVector outMatSize;
	
	// Store the output matrix size
	outMatSize.push_back(rows.size());
	outMatSize.push_back(rows[0].size());
	
	// Declare a set for the possible cell stored types
	TypeSet cellTypes;
	
	// For each row of the matrix expression
	for (CellArrayExpr::RowVector::const_iterator rowItr = rows.begin(); rowItr != rows.end(); ++rowItr)
	{
		// Get a reference to this row
		const CellArrayExpr::Row& row = *rowItr;
	
		// For each element in this row
		for (CellArrayExpr::Row::const_iterator colItr = row.begin(); colItr != row.end(); ++colItr)
		{
			// Get a reference to this expression
			const Expression* pExpr = *colItr;
			
			// Perform type inference for the expression
			TypeSetString exprTypeStr = inferTypes(
				pExpr,
				reachDefs,
				pLocalEnv,
				varTypes,
				exprTypeMap
			);

			// If the expression type is unknown
			if (exprTypeStr.empty() || exprTypeStr[0].empty())
			{
				// Set the unknown argument flag
				unknownArg = true;
			}
			else
			{
				// Get a reference to the expression type set
				const TypeSet& argTypes = exprTypeStr[0];
							
				// Add the possible types to the cell array stored type set
				cellTypes.insert(argTypes.begin(), argTypes.end());
			}
		}
	}
	
	// Determine if the output will is a 2D matrix
	bool is2D = (outMatSize.size() == 2);
	
	// Determine if the output will be scalar
	bool isScalar = (outMatSize == TypeInfo::DimVector(2, 1)); 
	
	// If an unknown argument was found, clear the possible cell types
	if (unknownArg)	cellTypes.clear();
	
	// Reduce the set of possible cell types
	cellTypes = typeSetReduce(cellTypes);
	
	// Return the type info for the output cell array
	return typeSetStrMake(TypeInfo(
		DataObject::CELLARRAY,
		is2D,
		isScalar,
		false,
		true,
		outMatSize,
		NULL,
		cellTypes
	));
}

/***************************************************************
* Function: inferTypes(FnHandleExpr)
* Purpose : Perform type inference for a function handle expr.
* Initial : Maxime Chevalier-Boisvert on May 12, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeSetString inferTypes(
	const FnHandleExpr* pHandleExpr,
	const VarDefMap& reachDefs,
	Environment* pLocalEnv,
	const VarTypeMap& varTypes,
	ExprTypeMap& exprTypeMap
)
{
	// Get the handle symbol
	SymbolExpr* pSymbol = pHandleExpr->getSymbolExpr();
	
	// Lookup the handle symbol in the reaching definitions map
	VarDefMap::const_iterator varDefItr = reachDefs.find(pSymbol);
			
	// If we are not in the case where the only reaching definition comes from the local env., return no information
	if (varDefItr == reachDefs.end() || varDefItr->second.size() != 1 || varDefItr->second.find(NULL) == varDefItr->second.end())
		return TypeSetString();

	// Lookup the symbol in the environment
	const DataObject* pObject = Environment::lookup(pLocalEnv, pSymbol);

	// If the object is not a function, return no information
	if (pObject->getType() != DataObject::FUNCTION)
		return TypeSetString();

	// Get a typed pointer to the function
	Function* pFunction = (Function*)pObject;
		
	// If the function is a nested function
	if (pFunction->isProgFunction() && ((ProgFunction*)pFunction)->getParent() != NULL)
		return TypeSetString();

	// Return type information for the function handle
	return typeSetStrMake(TypeInfo(
		DataObject::FN_HANDLE,
		false,
		false,
		false,
		false,
		TypeInfo::DimVector(),
		pFunction,
		TypeSet()
	));
}

/***************************************************************
* Function: analyzeIndexTypes()
* Purpose : Analyze matrix indexing argument types
* Initial : Maxime Chevalier-Boisvert on April 27, 2009
****************************************************************
Revisions and bug fixes:
*/
void analyzeIndexTypes(
	const ParamExpr::ExprVector& argVector,
	const VarDefMap& reachDefs,
	Environment* pLocalEnv,
	const VarTypeMap& varTypes,
	ExprTypeMap& exprTypeMap,
	size_t& numIndexDims,
	bool& isScalarIndexing,
	bool& isMatrixIndexing
)
{
	//std::cout << "Analyzing index types" << std::endl;
	
	// Get the number of indexing dimensions
	numIndexDims = argVector.size();
	
	// Initially, set all flags to optimistic values
	isScalarIndexing = true;
	isMatrixIndexing = false;
	
	// TODO: full range exprs can allow us to determine size, in some cases
	
	// For each argument expression
	for (ParamExpr::ExprVector::const_iterator argItr = argVector.begin(); argItr != argVector.end(); ++argItr)
	{
		// Get a pointer to the argument expression
		Expression* pArgExpr = *argItr;

		// Perform type inference for the expression
		TypeSetString outTypes = inferTypes(
			pArgExpr,
			reachDefs,
			pLocalEnv,
			varTypes,
			exprTypeMap
		);
		
		// If one of the arguments has unknown type
		if (outTypes.size() == 0 || outTypes[0].size() == 0)
		{
			// If this is the only argument, this could be matrix indexing
			if (argVector.size() == 1)
				isMatrixIndexing = true;
			
			// Set flags to pessimistic values
			isScalarIndexing = false;
			
			// Continue to the next argument
			continue;
		}
		
		// Get a reference to the possible argument types
		const TypeSet& argTypes = outTypes.front();
		
		// For each possible argument type
		for (TypeSet::const_iterator typeItr = argTypes.begin(); typeItr != argTypes.end(); ++typeItr)
		{
			// If this is a matrix type
			if (typeItr->getObjType() >= DataObject::MATRIX_I32 && typeItr->getObjType() <= DataObject::CHARARRAY)
			{
				// If we cannot guarantee that this argument is a scalar
				if (typeItr->isScalar() == false)
				{
					// If this is the only argument, this could be matrix indexing
					if (argVector.size() == 1)
						isMatrixIndexing = true;
					
					// Set the scalar indexing flag to false
					isScalarIndexing = false;
				}
			}			
		}		
	}
	
	//std::cout << "Done analyzing index types" << std::endl;
}

/***************************************************************
* Function: varTypeMapReduce()
* Purpose : Reduce type sets in a variable map
* Initial : Maxime Chevalier-Boisvert on April 20, 2009
****************************************************************
Revisions and bug fixes:
*/
void varTypeMapReduce(
	VarTypeMap& varTypes
)
{
	/*
	// Create an object for the output map
	VarTypeMap outMap;
	
	// For each element of the input map
	for (VarTypeMap::const_iterator itr = varTypes.begin(); itr != varTypes.end(); ++itr)
	{
		// Reduce the type set for this variable
		outMap[itr->first] = typeSetReduce(itr->second);
	}
	
	// Return the output map
	return outMap;
 	*/

	// For each element of the input map
	for (VarTypeMap::const_iterator itr = varTypes.begin(); itr != varTypes.end(); ++itr)
	{
		/*
		// If this variable is not live anymore, skip it
		if (liveVars.find(itr->first) == liveVars.end() && weedMap)
			continue;
		*/
		
		// Reduce the type set for this variable
		varTypes[itr->first] = typeSetReduce(itr->second);
	}
}

/***************************************************************
* Function: varTypeMapUnion()
* Purpose : Obtain the union of two variable type maps
* Initial : Maxime Chevalier-Boisvert on April 20, 2009
****************************************************************
Revisions and bug fixes:
*/
VarTypeMap varTypeMapUnion(
	const VarTypeMap& mapA,
	const VarTypeMap& mapB
)
{
	// Create an object for the output map
	VarTypeMap outMap;
	
	// For each element of the first map
	for (VarTypeMap::const_iterator itrA = mapA.begin(); itrA != mapA.end(); ++itrA)
	{
		// Attempt to find this symbol in the other map
		VarTypeMap::const_iterator itrB = mapB.find(itrA->first);
			
		// If the symbol was not found, skip it
		if (itrB == mapB.end())
			continue;	
		
		// Store the union of the type sets from both maps in the output map
		outMap[itrA->first] = typeSetUnion(itrA->second, itrB->second);
	}
	
	// Return the output map
	return outMap;
}

/***************************************************************
* Function: exitPointUnion()
* Purpose : Perform the union of multiple type maps
* Initial : Maxime Chevalier-Boisvert on May 7, 2009
****************************************************************
Revisions and bug fixes:
*/
VarTypeMap typeMapVectorUnion(
	const TypeMapVector& typeMaps
)
{
	// If the vector is empty, return an empty type map
	if (typeMaps.empty())
		return VarTypeMap();
	
	// Create a map to store the output and initialize it to the first type map
	VarTypeMap outMap = typeMaps[0];
	
	// For each type map, except the first
	for (size_t i = 1; i < typeMaps.size(); ++i)
	{
		// Perform the union of this map and the output map
		outMap = varTypeMapUnion(outMap, typeMaps[i]);
	}	
	
	// Return the output map
	return outMap;
}
