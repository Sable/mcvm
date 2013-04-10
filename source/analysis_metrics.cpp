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
#include "analysis_metrics.h"
#include "functions.h"
#include "paramexpr.h"
#include "interpreter.h"
#include "profiling.h"

/***************************************************************
* Function: computeMetrics()
* Purpose : Compute metrics info on a function body
* Initial : Maxime Chevalier-Boisvert on April July 22, 2009
****************************************************************
Revisions and bug fixes:
*/
AnalysisInfo* computeMetrics(
	const ProgFunction* pFunction,
	const StmtSequence* pFuncBody,
	const TypeSetString& inArgTypes,
	bool returnBottom
)
{
	// Create a new metrics info object
	MetricsInfo* pMetricsInfo = new MetricsInfo();
	
	// Initialize the metrics information	
	pMetricsInfo->numStmts = 0;
	pMetricsInfo->maxLoopDepth = 0;
	
	// If bottom should be returned, return no information
	if (returnBottom)
		return pMetricsInfo;

	// Perform a reaching definition analysis on the function body
	const ReachDefInfo* pReachDefInfo = (ReachDefInfo*)AnalysisManager::requestInfo(
		&computeReachDefs,
		pFunction,
		pFuncBody,
		inArgTypes
	);
	
	// Get the metrics for the function body
	getMetrics(
		pFunction->getCurrentBody(),
		pReachDefInfo->reachDefMap,
		ProgFunction::getLocalEnv(pFunction),
		0,
		pMetricsInfo->numStmts,
		pMetricsInfo->maxLoopDepth,
		pMetricsInfo->callees
	);
	
	// Update the statement count
	PROF_SET_COUNTER(
		Profiler::METRIC_NUM_STMTS,
		PROF_GET_COUNTER(Profiler::METRIC_NUM_STMTS) +
		pMetricsInfo->numStmts
	);
	
	// Update the max loop depth
	PROF_SET_COUNTER(
		Profiler::METRIC_MAX_LOOP_DEPTH,
		std::max(
			PROF_GET_COUNTER(Profiler::METRIC_MAX_LOOP_DEPTH),
			uint64(pMetricsInfo->maxLoopDepth)
		)
	);
	
	// Update the call site count
	PROF_SET_COUNTER(
		Profiler::METRIC_NUM_CALL_SITES,
		PROF_GET_COUNTER(Profiler::METRIC_NUM_CALL_SITES) +
		pMetricsInfo->callees.size()
	);
	
	// Return the metrics info object
	return pMetricsInfo;	
}

/***************************************************************
* Function: getMetrics()
* Purpose : Get the metrics info for a statement sequence
* Initial : Maxime Chevalier-Boisvert on April July 22, 2009
****************************************************************
Revisions and bug fixes:
*/
void getMetrics(
	const StmtSequence* pStmtSeq,
	const ReachDefMap& reachDefs,
	Environment* pLocalEnv,
	size_t curLoopDepth,
	size_t& numStmts,
	size_t& maxLoopDepth,
	FunctionSet& callees
)
{
	// Get a reference to the statement vector
	const StmtSequence::StmtVector& stmts = pStmtSeq->getStatements();
	
	// For each statement
	for (size_t i = 0; i < stmts.size(); ++i)
	{
		// Get a pointer to the statement
		const Statement* pStmt = stmts[i];
		
		// Increment the statement count
		numStmts += 1;
		
		// Find the reaching definition information for this statement
		ReachDefMap::const_iterator defItr = reachDefs.find(pStmt);
		assert (defItr != reachDefs.end());
		
		// Switch on the statement type
		switch (pStmt->getStmtType())
		{
			// If-else statement
			case Statement::IF_ELSE:
			{
				// Get a typed pointer to the statement
				const IfElseStmt* pIfStmt = (const IfElseStmt*)pStmt;
				
				// Analyse the condition expression
				getMetrics(
					pIfStmt->getCondition(),
					defItr->second,
					pLocalEnv,
					callees
				);
				
				// Analyse the true branch
				getMetrics(
					pIfStmt->getIfBlock(),
					reachDefs,
					pLocalEnv,
					curLoopDepth,
					numStmts,
					maxLoopDepth,
					callees
				);
				
				// Analyse the false branch
				getMetrics(
					pIfStmt->getElseBlock(),
					reachDefs,
					pLocalEnv,
					curLoopDepth,
					numStmts,
					maxLoopDepth,
					callees
				);				
			}
			break;
		
			// Loop statement
			case Statement::LOOP:
			{
				// Update the maximum loop nesting depth
				maxLoopDepth = std::max(maxLoopDepth, curLoopDepth + 1);
				
				// Get a typed pointer to the statement
				const LoopStmt* pLoopStmt = (const LoopStmt*)pStmt;
				
				// Analyse the init sequence
				getMetrics(
					pLoopStmt->getInitSeq(),
					reachDefs,
					pLocalEnv,
					curLoopDepth,
					numStmts,
					maxLoopDepth,
					callees
				);
				
				// Analyse the test sequence
				getMetrics(
					pLoopStmt->getTestSeq(),
					reachDefs,
					pLocalEnv,
					curLoopDepth + 1,
					numStmts,
					maxLoopDepth,
					callees
				);
				
				// Analyse the body sequence
				getMetrics(
					pLoopStmt->getBodySeq(),
					reachDefs,
					pLocalEnv,
					curLoopDepth + 1,
					numStmts,
					maxLoopDepth,
					callees
				);
				
				// Analyse the incr sequence
				getMetrics(
					pLoopStmt->getIncrSeq(),
					reachDefs,
					pLocalEnv,
					curLoopDepth + 1,
					numStmts,
					maxLoopDepth,
					callees
				);				
			}
			break;
		
			// Assignment statement
			case Statement::ASSIGN:
			{
				// Analyse the right expression
				getMetrics(
					((const AssignStmt*)pStmt)->getRightExpr(),
					defItr->second,
					pLocalEnv,
					callees
				);
			}
			break;
			
			// Expression statement
			case Statement::EXPR:
			{
				// Analyse the expression
				getMetrics(
					((const ExprStmt*)pStmt)->getExpression(),
					defItr->second,
					pLocalEnv,
					callees
				);
			}
			break;
			
			// Other statement types
			default:
			{
				// Do nothing
			}
		}
	}	
}

/***************************************************************
* Function: getMetrics()
* Purpose : Get the metrics info for an expression
* Initial : Maxime Chevalier-Boisvert on April July 22, 2009
****************************************************************
Revisions and bug fixes:
*/
void getMetrics(
	const Expression* pExpr,
	const VarDefMap& reachDefs,
	Environment* pLocalEnv,
	FunctionSet& callees
)
{
	// If this is a parameterized expression
	if (pExpr->getExprType() == Expression::PARAM)
	{
		// Get a typed pointer to the expression
		const ParamExpr* pParamExpr = (const ParamExpr*)pExpr;
		
		// Get the symbol expression
		SymbolExpr* pSymbol = pParamExpr->getSymExpr();
		
		// Lookup the symbol in the reaching definitions map
		VarDefMap::const_iterator varDefItr = reachDefs.find(pSymbol);
		
		// If we are in the case where the only reaching definition comes from the local env.
		if (varDefItr != reachDefs.end() && varDefItr->second.size() == 1 && varDefItr->second.find(NULL) != varDefItr->second.end())
		{
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
					
			// If the object is a program function
			if (pObject != NULL && pObject->getType() == DataObject::FUNCTION)
			{
				// Add the function to the list of callees
				callees.insert((Function*)pObject);
			}
		}
	}
	
	// Get the list of sub-expressions for this expression
	const Expression::ExprVector& subExprs = pExpr->getSubExprs();
	
	// For each sub-expression
	for (size_t i = 0; i < subExprs.size(); ++i)
	{
		// Get a pointer to the sub-expression
		Expression* pSubExpr = subExprs[i];
		
		// If the sub-expression is null, skip it
		if (pSubExpr == NULL)
			continue;
	
		// Analyse the sub-expression
		getMetrics(
			pSubExpr,
			reachDefs,
			pLocalEnv,
			callees
		);
	}
}
