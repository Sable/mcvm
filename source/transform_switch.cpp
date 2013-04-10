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
#include "transform_switch.h"
#include "utility.h"
#include "ifelsestmt.h"
#include "switchstmt.h"
#include "loopstmts.h"
#include "rangeexpr.h"
#include "binaryopexpr.h"
#include "constexprs.h"
#include "paramexpr.h"
#include "cellindexexpr.h"

/***************************************************************
* Function: transformSwitch()
* Purpose : Eliminate switch statements in a statement sequence
* Initial : Maxime Chevalier-Boisvert on March 17, 2009
****************************************************************
Revisions and bug fixes:
*/
StmtSequence* transformSwitch(StmtSequence* pSeq, ProgFunction* pFunction)
{
	// Get a reference to the statement vector
	const StmtSequence::StmtVector& seqStmts = pSeq->getStatements();
	
	// Create a statement vector to store the transformed output
	StmtSequence::StmtVector output;
	
	// For each statement in the sequence
	for (StmtSequence::StmtVector::const_iterator stmtItr = seqStmts.begin(); stmtItr != seqStmts.end(); ++stmtItr)
	{
		// Get a pointer to the statement
		Statement* pStmt = *stmtItr;
		
		// Switch on the statement type
		switch (pStmt->getStmtType())
		{
			// If-else statement
			case Statement::IF_ELSE:
			{
				// Get a typed pointer to the statement
				IfElseStmt* pIfStmt = (IfElseStmt*)pStmt;
								
				// Copy the test condition
				Expression* pNewTest = pIfStmt->getCondition()->copy();
				
				// Transform the if and else blocks
				StmtSequence* pNewIfBlock = transformSwitch(pIfStmt->getIfBlock(), pFunction);				
				StmtSequence* pNewElseBlock = transformSwitch(pIfStmt->getElseBlock(), pFunction);
				
				// Create a new if-else statement and add it to the vector
				output.push_back(new IfElseStmt(pNewTest, pNewIfBlock, pNewElseBlock));
			}
			break;
			
			// Loop statement
			case Statement::LOOP:
			{
				// Get a typed pointer to the statement
				LoopStmt* pLoopStmt = (LoopStmt*)pStmt;
				
				// Transform the statement sequences of the loop
				StmtSequence* pNewInitSeq = transformSwitch(pLoopStmt->getInitSeq(), pFunction);
				StmtSequence* pNewTestSeq = transformSwitch(pLoopStmt->getTestSeq(), pFunction);
				StmtSequence* pNewBodySeq = transformSwitch(pLoopStmt->getBodySeq(), pFunction);
				StmtSequence* pNewIncrSeq = transformSwitch(pLoopStmt->getIncrSeq(), pFunction);
				
				// Create a new loop statement
				output.push_back(
					new LoopStmt(
						pLoopStmt->getIndexVar(),
						pLoopStmt->getTestVar(),
						pNewInitSeq,
						pNewTestSeq,
						pNewBodySeq,
						pNewIncrSeq,
                        pLoopStmt->getAnnotations()			
					)
				);
			}
			break;
			
			// Switch statement
			case Statement::SWITCH:
			{
				// Get a typed pointer to the statement 
				SwitchStmt* pSwitchStmt = (SwitchStmt*)pStmt;
				
				// Transform the switch statement
				StmtSequence* pSequence = transformSwitch(pSwitchStmt, pFunction); 
				
				// Insert the statements into the current sequence
				output.insert(
					output.end(),
					pSequence->getStatements().begin(),
					pSequence->getStatements().end()
				);
			}
			break;
			
			// For all other statement types
			default:
			{
				// Copy the statement and add it to the output
				output.push_back(pStmt->copy());
			}
		}
	}
	
	// Return the transformed output
	return new StmtSequence(output);
}

/***************************************************************
* Function: transformSwitch()
* Purpose : Transform a switch statement into if-elses/loops
* Initial : Maxime Chevalier-Boisvert on March 17, 2009
****************************************************************
Revisions and bug fixes:
*/
StmtSequence* transformSwitch(SwitchStmt* pSwitchStmt, ProgFunction* pFunction)
{
	// Get the case list
	const SwitchStmt::CaseList& caseList = pSwitchStmt->getCaseList();

	// Create a vector to store the current statements
	StmtSequence::StmtVector stmts;
	
	// Create a variable for the switching value
	SymbolExpr* pSwitchVar = pFunction->createTemp();
	
	// Assign the switch expression to the variable
	stmts.push_back(new AssignStmt(pSwitchVar, pSwitchStmt->getSwitchExpr()->copy()));
		
	// Get the default case for this switch statement
	StmtSequence* pCurSeq = pSwitchStmt->getDefaultCase()->copy();

	// For each switch case
	for (SwitchStmt::CaseList::const_reverse_iterator itr = caseList.rbegin(); itr != caseList.rend(); ++itr)
	{
		// Get a reference to the current switch case
		const SwitchStmt::SwitchCase& curCase = *itr;
		
		// Create a vector to store the current statements
		StmtSequence::StmtVector stmts;
		
		// Create a variable for the case value
		SymbolExpr* pCaseVar = pFunction->createTemp();
		
		// Assign the case expression to the variable
		stmts.push_back(new AssignStmt(pCaseVar, curCase.first->copy()));
		
		// Create a variable for the comparison test result
		SymbolExpr* pTestVar = pFunction->createTemp();
		
		// Create a variable to indicate if the test variable is a cell array
		SymbolExpr* pCellVar = pFunction->createTemp();
		
		// Test if the case variable is a cell array
		stmts.push_back(
			new AssignStmt(
				pCellVar,
				new ParamExpr(
					SymbolExpr::getSymbol("iscell"),
					ParamExpr::ExprVector(1, pCaseVar)
				)
			)
		);
		
		// Create a vector to store the cell array specific statements
		StmtSequence::StmtVector cellStmts;		
		
		// Initialize the test variable to 0 (false)
		cellStmts.push_back(new AssignStmt(pTestVar, new IntConstExpr(0)));
		
		// Create variables for the testing loop
		SymbolExpr* pNumelVar = pFunction->createTemp();
		SymbolExpr* pLoopItrVar = pFunction->createTemp();
		SymbolExpr* pLoopTestVar = pFunction->createTemp();
		
		// Create statement vectors for the testing loop
		StmtSequence::StmtVector initStmts;
		StmtSequence::StmtVector testStmts;
		StmtSequence::StmtVector bodyStmts;
		StmtSequence::StmtVector incrStmts;
		
		// Test if the test variable is a cell array
		initStmts.push_back(
			new AssignStmt(
				pNumelVar,
				new ParamExpr(
					SymbolExpr::getSymbol("numel"),
					ParamExpr::ExprVector(1, pCaseVar)
				)
			)
		);		
		
		// Initialize the iterator variable to 0
		initStmts.push_back(new AssignStmt(pLoopItrVar, new IntConstExpr(1)));
		
		// Test that the iterator variable is less than or equal to the numel variable
		testStmts.push_back(
			new AssignStmt(
				pLoopTestVar,
				new BinaryOpExpr(
					BinaryOpExpr::LESS_THAN_EQ,
					pLoopItrVar,
					pNumelVar
				)
			)
		);
		
		// Create a variable to store the current cell array element
		SymbolExpr* pElemVar = pFunction->createTemp();
		
		// Assign the current cell array element to the variable
		bodyStmts.push_back(
			new AssignStmt(
				pElemVar,
				new CellIndexExpr(
					pCaseVar,
					Expression::ExprVector(1, pLoopItrVar)
				)
			)
		);
		
		// If the switch variable matches this cell element, set the test variable to 1 (true)
		bodyStmts.push_back(
			new IfElseStmt(
				new BinaryOpExpr(BinaryOpExpr::EQUAL, pSwitchVar, pElemVar),
				new StmtSequence(new AssignStmt(pTestVar, new IntConstExpr(1))),
				new StmtSequence()
			)
		);		
		
		// Increment the iterator variable by 1
		incrStmts.push_back(
			new AssignStmt(
				pLoopItrVar,
				new BinaryOpExpr(
					BinaryOpExpr::PLUS,
					pLoopItrVar,
					new IntConstExpr(1)
				)
			)
		);
		
		// Create a loop statement for the cell array testing
		cellStmts.push_back(
			new LoopStmt(
				pLoopItrVar,
				pLoopTestVar,
				new StmtSequence(initStmts),
				new StmtSequence(testStmts),
				new StmtSequence(bodyStmts),
				new StmtSequence(incrStmts),
                0 // fixme: not really correct, but for now, use 0
			)
		);
		
		// Create an if statement for the non-cell case, comparing the case variable directly
		StmtSequence* nonCellSeq = new StmtSequence(
			new IfElseStmt(
				new BinaryOpExpr(BinaryOpExpr::EQUAL, pSwitchVar, pCaseVar),
				new StmtSequence(new AssignStmt(pTestVar, new IntConstExpr(1))),
				new StmtSequence(new AssignStmt(pTestVar, new IntConstExpr(0)))
			)
		);
		
		// Create a switch for the cell and non-cell testing cases
		stmts.push_back(
			new IfElseStmt(
				pCellVar,
				new StmtSequence(cellStmts),
				nonCellSeq
			)
		);
		
		// Finally, if the test variable is true, execute this case
		stmts.push_back(
			new IfElseStmt(
				pTestVar,
				curCase.second->copy(),
				pCurSeq
			)
		);	
		
		// Update the current statement sequence
		pCurSeq = new StmtSequence(stmts);
	}
	
	// Add the current sequence to the statement list
	stmts.insert(
		stmts.end(), 
		pCurSeq->getStatements().begin(),
		pCurSeq->getStatements().end()
	);
	
	// Return the statements
	return new StmtSequence(stmts);
}
