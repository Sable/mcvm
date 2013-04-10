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
#include <iostream>
#include <cassert>
#include "analysis_reachdefs.h"
#include "functions.h"
#include "profiling.h"

/***************************************************************
* Function: computeReachDefs(ProgFunction, ...)
* Purpose : Get the reaching definitions for a function body
* Initial : Maxime Chevalier-Boisvert on April 1, 2009
****************************************************************
Revisions and bug fixes:
*/
AnalysisInfo* computeReachDefs(
	const ProgFunction* pFunction,
	const StmtSequence* pFuncBody,
	const TypeSetString& inArgTypes,
	bool returnBottom	
)
{
	// Create a reaching definition info object
	ReachDefInfo* pReachDefInfo = new ReachDefInfo();
	
	// If we should return bottom, return no information
	if (returnBottom)
		return pReachDefInfo;
	
	// Get the input parameters for this function
	const ProgFunction::ParamVector& inParams = pFunction->getInParams();
		
	// Declare an object for the initial variable map
	VarDefMap initialMap;
	
	// For each parent of the current function (direct or indirect)
	for (ProgFunction* pCurParent = pFunction->getParent(); pCurParent != NULL; pCurParent = pCurParent->getParent())
	{
		// Get the input parameters for this parent function
		const ProgFunction::ParamVector& inParams = pCurParent->getInParams();
		
		// Add the initial reaching definitions from the function parameters
		for (ProgFunction::ParamVector::const_iterator itr = inParams.begin(); itr != inParams.end(); ++itr)
			initialMap[*itr].insert(pCurParent);		
		
		// Get the function body for this parent function
		StmtSequence* pFuncBody = pCurParent->getCurrentBody();
		
		// Get the set of all defs in the function body
		Expression::SymbolSet bodyDefs = pFuncBody->getSymbolDefs();

		// Add the initial reaching definitions from the parent body
		for (Expression::SymbolSet::iterator itr = bodyDefs.begin(); itr != bodyDefs.end(); ++itr)
			initialMap[*itr].insert(pCurParent);		
	}
	
	// Add the initial reaching definitions from the function parameters
	for (ProgFunction::ParamVector::const_iterator itr = inParams.begin(); itr != inParams.end(); ++itr)
		initialMap[*itr].insert(pFunction);
		
	// Get the set of all uses and defs in the function body
	Expression::SymbolSet bodyUses = pFuncBody->getSymbolUses();
	Expression::SymbolSet bodyDefs = pFuncBody->getSymbolDefs();
	
	// Merge the two sets into the set of all variables present in the body
	Expression::SymbolSet varSet;
	varSet.insert(bodyUses.begin(), bodyUses.end());
	varSet.insert(bodyDefs.begin(), bodyDefs.end());

	// Add the initial reaching definitions from the environment for each variable
	for (Expression::SymbolSet::iterator itr = varSet.begin(); itr != varSet.end(); ++itr)
		initialMap[*itr].insert(NULL);
	
	// Declare maps for the continue and break reaching definitions
	VarDefMap breakMap;
	VarDefMap contMap;
	
	// double startTime = Profiler::getTimeSeconds();
	
	// Compute the reaching definitions for the function body
	getReachDefs(
		pFuncBody,
		initialMap,
		pReachDefInfo->exitDefMap,
		pReachDefInfo->exitDefMap,
		breakMap,
		contMap,
		pReachDefInfo->reachDefMap
	);

	// Ensure that the break and continue maps are empty
	assert (breakMap.empty() && contMap.empty());
	
	// std::cout << "*** Reach def time: " << (Profiler::getTimeSeconds() - startTime) << " s" << std::endl;
	
	// Return the reaching definition information
	return pReachDefInfo;
}

/***************************************************************
* Function: getReachDefs(StmtSequence, ...)
* Purpose : Get the reaching defs for a statement sequence
* Initial : Maxime Chevalier-Boisvert on April 1, 2009
****************************************************************
Revisions and bug fixes:
*/
void getReachDefs(
	const StmtSequence* pStmtSeq,
	const VarDefMap& startMap,
	VarDefMap& exitMap,
	VarDefMap& retMap,
	VarDefMap& breakMap, 
	VarDefMap& contMap,
	ReachDefMap& reachDefMap
)
{
	// Get a reference to the statement vector
	const StmtSequence::StmtVector& stmts = pStmtSeq->getStatements();
		
	// Initialize the current reaching definition map with the start map
	VarDefMap curMap = startMap;
	
	// Store the set associated with this statement sequence
	reachDefMap[pStmtSeq] = curMap;
	
	// For each statement, in reverse order
	for (StmtSequence::StmtVector::const_iterator stmtItr = stmts.begin(); stmtItr != stmts.end(); ++stmtItr)
	{
		// Get a pointer to the statement
		const Statement* pStmt = *stmtItr;
		
		// Set the reaching definition set for this statement
		reachDefMap[pStmt] = curMap;
		
		// Switch on the statement type
		switch (pStmt->getStmtType())
		{
			// Break statement
			case Statement::BREAK:
			{				
				// Add the definitions of the current map to the break map
				breakMap = varDefMapUnion(breakMap, curMap);
			}
			break;

			// Continue statement
			case Statement::CONTINUE:
			{		
				// Add the definitions of the current map to the continue map
				contMap = varDefMapUnion(contMap, curMap);
			}
			break;

			// Return statement
			case Statement::RETURN:
			{
				// Add the definitions of the current map to the return map
				retMap = varDefMapUnion(retMap, curMap);
			}
			break;
			
			// If-else statement
			case Statement::IF_ELSE:
			{
				// Declare a map for the reaching definitions after the statement
				VarDefMap exitMap;
			
				// Get the reaching definition map for the if-else statement
				getReachDefs(
					(const IfElseStmt*)pStmt,
					curMap,
					exitMap,
					retMap,
					breakMap, 
					contMap,
					reachDefMap
				);

				// Update the current reaching definition map
				curMap = exitMap;
			}
			break;

			// Loop statement
			case Statement::LOOP:
			{
				// Declare a map for the reaching definitions after the statement
				VarDefMap exitMap;
			
				// Get the reaching definition map for the loop statement
				getReachDefs(
					(const LoopStmt*)pStmt,
					curMap,
					exitMap,
					retMap,
					reachDefMap
				);
				
				// Update the current reaching definition map
				curMap = exitMap;		
			}
			break;
			
			// Other statement types
			default:
			{
				// Get the definitions of the statement
				Expression::SymbolSet defs = pStmt->getSymbolDefs();

				// Create a definition set for this statement
				VarDefSet defSet;
				defSet.insert(pStmt);
				
				// Replace all defined variable definition sets by the one for this statement
				for (Expression::SymbolSet::iterator itr = defs.begin(); itr != defs.end(); ++itr)
					curMap[*itr] = defSet;
			}
		}
	}
	
	// Merge the current reaching definition set into the final (sequence exit) set
	exitMap.insert(curMap.begin(), curMap.end());
}

/***************************************************************
* Function: getReachDefs(IfElseStmt)
* Purpose : Get the reaching defs for an if-else statement
* Initial : Maxime Chevalier-Boisvert on April 1, 2009
****************************************************************
Revisions and bug fixes:
*/
void getReachDefs(
	const IfElseStmt* pIfStmt,
	const VarDefMap& startMap,
	VarDefMap& exitMap,
	VarDefMap& retMap,
	VarDefMap& breakMap, 
	VarDefMap& contMap,
	ReachDefMap& reachDefMap
)
{
	// Get the condition expression
	Expression* pTestExpr = pIfStmt->getCondition();
	
	// Store the map of reaching definitions at the condition expression
	reachDefMap[pTestExpr] = startMap;
	
	// Create variable definition maps for the exit points of the if and else blocks
	VarDefMap ifVarDefs;
	VarDefMap elseVarDefs;	
	
	// Get the reaching definitions for the if block statements
	getReachDefs(
		pIfStmt->getIfBlock(),
		startMap,
		ifVarDefs,
		retMap,
		breakMap, 
		contMap,
		reachDefMap
	);

	// Get the reaching definitions for the else block statements
	getReachDefs(
		pIfStmt->getElseBlock(),
		startMap,
		elseVarDefs,
		retMap,
		breakMap,
		contMap,
		reachDefMap
	);
	
	// Compute the exit definition map as the union of the maps of the if and else blocks
	exitMap = varDefMapUnion(ifVarDefs, elseVarDefs);
}

/***************************************************************
* Function: getReachDefs(LoopStmt)
* Purpose : Get the reaching defs for a loop statement
* Initial : Maxime Chevalier-Boisvert on April 1, 2009
****************************************************************
Revisions and bug fixes:
*/
void getReachDefs(
	const LoopStmt* pLoopStmt,
	const VarDefMap& startMap,
	VarDefMap& exitMap,
	VarDefMap& retMap,
	ReachDefMap& reachDefMap
)
{
	// Declare a map for the initialization exit reaching definitions
	VarDefMap initExitMap;
	
	// Get the reaching definitions map for the initialization block
	VarDefMap initRetMap;
	VarDefMap initBreakMap;
	VarDefMap initContMap;
	getReachDefs(
		pLoopStmt->getInitSeq(),
		startMap,
		initExitMap,
		initRetMap,
		initBreakMap, 
		initContMap,
		reachDefMap
	);
	assert (initRetMap.empty() && initBreakMap.empty() && initContMap.empty());
		
	// Initialize the current incrementation exit map
	VarDefMap curIncrExitMap;
	
	// Declare a reaching definition map for the loop statements
	//ReachDefMap loopDefMap;
	
	// Until we have reached a fixed point
	for (;;)
	{
		// Clear the loop definition map
		//loopDefMap.clear();
		
		// Compute the test start map as the union of the init exit and incr exit maps
		VarDefMap testStartMap = varDefMapUnion(initExitMap, curIncrExitMap);
		
		// Declare a map for the test exit reaching definitions
		VarDefMap testExitMap;
		
		// Get the live variable map for the test block
		VarDefMap testRetMap;
		VarDefMap testBreakMap;
		VarDefMap testContMap;
		getReachDefs(
			pLoopStmt->getTestSeq(),
			testStartMap,
			testExitMap,
			testRetMap,
			testBreakMap, 
			testContMap,
			reachDefMap
		);
		assert (testRetMap.empty() && testBreakMap.empty() && testContMap.empty());
		
		// Declare maps for the body exit points
		VarDefMap bodyExitMap;
		VarDefMap breakMap;
		VarDefMap contMap;
		
		// Get the live variable map for the body block
		getReachDefs(
			pLoopStmt->getBodySeq(),
			testExitMap,
			bodyExitMap,
			retMap,
			breakMap,
			contMap,
			reachDefMap
		);

		// Merge the test exit map and the break map
		breakMap = varDefMapUnion(breakMap, testExitMap);
		
		// Compute the incr start map as the union of the body exit and continue maps
		VarDefMap incrStartMap = varDefMapUnion(bodyExitMap, contMap);
		
		// Get the live variable map for the incrementation block
		VarDefMap incrExitMap;
		VarDefMap incrRetMap;
		VarDefMap incrBreakMap;
		VarDefMap incrContMap;
		getReachDefs(
			pLoopStmt->getIncrSeq(),
			incrStartMap,
			incrExitMap,
			incrRetMap,
			incrBreakMap,
			incrContMap,
			reachDefMap
		);
		assert (incrRetMap.empty() && incrBreakMap.empty() && incrContMap.empty());
					
		// Update the current exit map
		exitMap = breakMap;
		
		// If a fixed point has been reached, stop
		if (incrExitMap == curIncrExitMap)
			break;
		
		// Update the current incrementation exit map
		curIncrExitMap = incrExitMap;
	}
	
	// Merge the the current loop map into the final definition map
	//reachDefMap.insert(loopDefMap.begin(), loopDefMap.end());
}

/***************************************************************
* Function: varDefMapUnion()
* Purpose : Obtain the union of two variable definition maps
* Initial : Maxime Chevalier-Boisvert on April 1, 2009
****************************************************************
Revisions and bug fixes:
*/
VarDefMap varDefMapUnion(
	const VarDefMap& mapA,
	const VarDefMap& mapB
)
{
	// Create an object for the output map
	VarDefMap outMap;
	
	// For each element of the first map
	for (VarDefMap::const_iterator itr = mapA.begin(); itr != mapA.end(); ++itr)
	{
		// Insert the definitions for this symbol into the output map
		outMap[itr->first].insert(itr->second.begin(), itr->second.end());
	}
	
	// For each element of the second map
	for (VarDefMap::const_iterator itr = mapB.begin(); itr != mapB.end(); ++itr)
	{
		// Insert the definitions for this symbol into the output map
		outMap[itr->first].insert(itr->second.begin(), itr->second.end());
	}
	
	// Return the output map
	return outMap;
}
