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
#include "analysis_livevars.h"
#include "functions.h"

/***************************************************************
* Function: computeLiveVars(ProgFunction, ...)
* Purpose : Get the live variables for a function body
* Initial : Maxime Chevalier-Boisvert on March 30, 2009
****************************************************************
Revisions and bug fixes:
*/
AnalysisInfo* computeLiveVars(
	const ProgFunction* pFunction,
	const StmtSequence* pFuncBody,
	const TypeSetString& inArgTypes,
	bool returnBottom
)
{
	// Create a new live var info object
	LiveVarInfo* pLiveVarInfo = new LiveVarInfo();
	
	// If bottom should be returned, return no information
	if (returnBottom)
		return pLiveVarInfo;
	
	// Get the set of output parameters for the function
	const ProgFunction::ParamVector& outParams = pFunction->getOutParams();
	Expression::SymbolSet outParamSet(outParams.begin(), outParams.end());
	
	// Compute the live variables for the statement sequence
	getLiveVars(
		pFuncBody,
		pLiveVarInfo->entryLiveSet,
		outParamSet,
		outParamSet,
		NULL,
		NULL,
		pLiveVarInfo->liveVarMap
	);
	
	// Return the live var info object
	return pLiveVarInfo;
}
 
/***************************************************************
* Function: getLiveVars(StmtSequence, ...)
* Purpose : Get the live variable sets for a statement sequence
* Initial : Maxime Chevalier-Boisvert on March 30, 2009
****************************************************************
Revisions and bug fixes:
*/
void getLiveVars(
	const StmtSequence* pStmtSeq,
	Expression::SymbolSet& startSet,
	const Expression::SymbolSet& exitSet,
	const Expression::SymbolSet& retSet,
	const Expression::SymbolSet* pBreakSet, 
	const Expression::SymbolSet* pContSet,
	LiveVarMap& liveVarMap
)
{
	// Get a reference to the statement vector
	const StmtSequence::StmtVector& stmts = pStmtSeq->getStatements();
	
	// Initialize the current live set with the exit set
	Expression::SymbolSet curSet = exitSet;
	
	// Store the set associated with this statement sequence
	liveVarMap[pStmtSeq] = curSet;
	
	// For each statement, in reverse order
	for (StmtSequence::StmtVector::const_reverse_iterator stmtItr = stmts.rbegin(); stmtItr != stmts.rend(); ++stmtItr)
	{
		// Get a pointer to the statement
		const Statement* pStmt = *stmtItr;
		
		// Switch on the statement type
		switch (pStmt->getStmtType())
		{
			// Break statement
			case Statement::BREAK:
			{
				// Ensure that the break set was specified
				assert (pBreakSet != NULL);
				
				// Set the current set to the break set
				curSet = *pBreakSet;
				
				// Set the live variable set for this statement
				liveVarMap[pStmt] = curSet;
			}
			break;

			// Continue statement
			case Statement::CONTINUE:
			{
				// Ensure that the break set was specified
				assert (pContSet != NULL);
				
				// Set the current set to the continue set
				curSet = *pContSet;
				
				// Set the live variable set for this statement
				liveVarMap[pStmt] = curSet;
			}
			break;

			// Return statement
			case Statement::RETURN:
			{
				// Set the current set to the return set
				curSet = retSet;
				
				// Set the live variable set for this statement
				liveVarMap[pStmt] = curSet;
			}
			break;
			
			// If-else statement
			case Statement::IF_ELSE:
			{
				// Set the live variable set for this statement
				liveVarMap[pStmt] = curSet;
				
				// Declare a set for the live variables before if-else statement
				Expression::SymbolSet startSet;
			
				// Get the live variable map for the if and else block statements
				getLiveVars(
					(const IfElseStmt*)pStmt,
					startSet,
					curSet,
					retSet,
					pBreakSet, 
					pContSet,
					liveVarMap
				);

				// Update the current live variable set
				curSet = startSet;				
			}
			break;

			// Loop statement
			case Statement::LOOP:
			{
				// Set the live variable set for this statement
				liveVarMap[pStmt] = curSet;
				
				// Declare a set for the live variables before the loop
				Expression::SymbolSet startSet;
				
				// Get the live variable map for the loop statements
				getLiveVars(
					(const LoopStmt*)pStmt,
					startSet,
					curSet,
					retSet,
					liveVarMap
				);
				
				// Update the current live variable set
				curSet = startSet;				
			}
			break;
			
			// Other statement types
			default:
			{
				// Get the uses and defs of the statement
				Expression::SymbolSet uses = pStmt->getSymbolUses();
				Expression::SymbolSet defs = pStmt->getSymbolDefs();
	
				// Set the live variable set for this statement
				liveVarMap[pStmt] = curSet;
				
				// Remove the definitions from the current live set
				for (Expression::SymbolSet::iterator itr = defs.begin(); itr != defs.end(); ++itr)
					if (curSet.find(*itr) != curSet.end()) curSet.erase(*itr);
				
				// Add the uses to the current live set
				curSet.insert(uses.begin(), uses.end());
			}
		}
	}	
	
	// Merge the current live variable set into the final (sequence start) set
	startSet.insert(curSet.begin(), curSet.end());
}

/***************************************************************
* Function: getLiveVars(IfElseStmt)
* Purpose : Get the live variable sets for an if-else statement
* Initial : Maxime Chevalier-Boisvert on March 30, 2009
****************************************************************
Revisions and bug fixes:
*/
void getLiveVars(
	const IfElseStmt* pIfElseStmt,
	Expression::SymbolSet& startSet,
	const Expression::SymbolSet& exitSet,
	const Expression::SymbolSet& retSet,
	const Expression::SymbolSet* pBreakSet, 
	const Expression::SymbolSet* pContSet,
	LiveVarMap& liveVarMap
)
{
	// Get the live variable map for the if block statements
	getLiveVars(
		pIfElseStmt->getIfBlock(),
		startSet,
		exitSet,
		retSet,
		pBreakSet, 
		pContSet,
		liveVarMap
	);

	// Get the live variable map for the else block statements
	getLiveVars(
		pIfElseStmt->getElseBlock(),
		startSet,
		exitSet,
		retSet,
		pBreakSet, 
		pContSet,
		liveVarMap
	);
	
	// Get the test expression
	Expression* pTestExpr = pIfElseStmt->getCondition();
	
	// Note the set of variables live after the test expression
	liveVarMap[pTestExpr] = startSet;
	
	// Get the uses of the test expression
	Expression::SymbolSet testUses = pTestExpr->getSymbolUses();
		
	// Add the uses of the test expression to the start set 
	startSet.insert(testUses.begin(), testUses.end());	
}

/***************************************************************
* Function: getLiveVars(LoopStmt)
* Purpose : Get the live variable sets for a loop statement
* Initial : Maxime Chevalier-Boisvert on March 30, 2009
****************************************************************
Revisions and bug fixes:
*/
void getLiveVars(
	const LoopStmt* pLoopStmt,
	Expression::SymbolSet& startSet,
	const Expression::SymbolSet& exitSet,
	const Expression::SymbolSet& retSet,
	LiveVarMap& liveVarMap
)
{
	// Declare a variable for the current loop live variable map
	//LiveVarMap loopVarMap;
	
	// Initialize the current test start set
	Expression::SymbolSet curTestStartSet;	
	
	// Initialize the current loop continue set
	Expression::SymbolSet curContSet;
	
	// Until we have reached a fixed point
	for (;;)
	{
		// Clear the loop live variable map
		//loopVarMap.clear();
		
		// Declare a variable for the incrementation block start set
		Expression::SymbolSet incrStartSet;
		
		// Get the live variable map for the incrementation block
		getLiveVars(
			pLoopStmt->getIncrSeq(),
			incrStartSet,
			curTestStartSet,
			retSet,
			NULL, 
			NULL,
			liveVarMap
		);
		
		// Declare a variable for the body block start set
		Expression::SymbolSet bodyStartSet;
				
		// Get the live variable map for the body block
		getLiveVars(
			pLoopStmt->getBodySeq(),
			bodyStartSet,
			incrStartSet,
			retSet,
			&exitSet, 
			&curContSet,
			liveVarMap
		);

		// Add the loop test variable to the body start set
		bodyStartSet.insert(pLoopStmt->getTestVar());
		
		// Merge the exit point set into the body start set
		bodyStartSet.insert(exitSet.begin(), exitSet.end());
		
		// Declare a variable for the test block start set
		Expression::SymbolSet testStartSet;
		
		// Get the live variable map for the test block
		getLiveVars(
			pLoopStmt->getTestSeq(),
			testStartSet,
			bodyStartSet,
			retSet,
			NULL, 
			NULL,
			liveVarMap
		);
			
		// Update the current continue set using the current incrementation block start set
		curContSet = incrStartSet;
		
		// If a fixed point has been reached, stop
		if (curTestStartSet == testStartSet)
			break;
		
		// Update the current test start set
		curTestStartSet = testStartSet;
	}

	// Get the live variable map for the initialization block
	getLiveVars(
		pLoopStmt->getInitSeq(),
		startSet,
		curTestStartSet,
		retSet,
		NULL, 
		NULL,
		liveVarMap
	);
		
	// Merge the current loop map into the final live map
	//liveVarMap.insert(loopVarMap.begin(), loopVarMap.end());
}
