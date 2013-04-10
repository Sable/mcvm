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
#include "transform_loops.h"
#include "utility.h"
#include "ifelsestmt.h"
#include "switchstmt.h"
#include "rangeexpr.h"
#include "binaryopexpr.h"
#include "constexprs.h"
#include "paramexpr.h"

/***************************************************************
* Function: transformLoops()
* Purpose : Simplify loops in a statement sequence
* Initial : Maxime Chevalier-Boisvert on January 13, 2009
****************************************************************
Revisions and bug fixes:
*/
StmtSequence* transformLoops(StmtSequence* pSeq, ProgFunction* pFunction)
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
				StmtSequence* pNewIfBlock = transformLoops(pIfStmt->getIfBlock(), pFunction);				
				StmtSequence* pNewElseBlock = transformLoops(pIfStmt->getElseBlock(), pFunction);
				
				// Create a new if-else statement and add it to the vector
				output.push_back(new IfElseStmt(pNewTest, pNewIfBlock, pNewElseBlock));
			}
			break;
			
			// For loop statement
			case Statement::FOR:
			{
				// Get a typed pointer to the statement
				ForStmt* pForStmt = (ForStmt*)pStmt;
				
				// Transform the for loop
				transformForLoop(pForStmt, output, pFunction);
			}
			break;
			
			// While loop statement
			case Statement::WHILE:
			{
				// Get a typed pointer to the statement
				WhileStmt* pWhileStmt = (WhileStmt*)pStmt;
				
				// Transform the for loop
				transformWhileLoop(pWhileStmt, output, pFunction);	
			}
			break;
			
			// Switch statement
			case Statement::SWITCH:
			{
				// Get a typed pointer to the statement 
				SwitchStmt* pSwitchStmt = (SwitchStmt*)pStmt;
				
				// Get the case list
				const SwitchStmt::CaseList& caseList = pSwitchStmt->getCaseList();
				
				// Create a vector for the transformed cases
				SwitchStmt::CaseList newCases;
				
				// For each switch case
				for (SwitchStmt::CaseList::const_iterator itr = caseList.begin(); itr != caseList.end(); ++itr)
				{
					// Transform this switch case
					newCases.push_back(
						SwitchStmt::SwitchCase(
							itr->first->copy(),
							transformLoops(itr->second, pFunction)
						)
					);
				}

				// Create and return the transformed switch statement object
				output.push_back(
					new SwitchStmt(
						pSwitchStmt->getSwitchExpr()->copy(),
						newCases,
						pSwitchStmt->getDefaultCase()? pSwitchStmt->getDefaultCase()->copy():NULL
					)
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
* Function: transformForLoop()
* Purpose : Simplify a for loop
* Initial : Maxime Chevalier-Boisvert on January 13, 2009
****************************************************************
Revisions and bug fixes:
*/
void transformForLoop(ForStmt* pForStmt, StmtSequence::StmtVector& stmtVector, ProgFunction* pFunction)
{
	// Transform the loop body
	StmtSequence* pNewBody = transformLoops(pForStmt->getLoopBody(), pFunction);
		
	// Get the loop assignment statement
	AssignStmt* pAssignStmt = pForStmt->getAssignStmt();
	
	// Get the left and right expressions of the assignment
	Expression* pLExpr = pAssignStmt->getLeftExprs().front();
	Expression* pRExpr = pAssignStmt->getRightExpr();
	
	// Ensure that the left expression is a symbol
	assert (pLExpr->getExprType() == Expression::SYMBOL);
	
	// Get a typed pointer to the loop variable
	SymbolExpr* pLoopVar = (SymbolExpr*)pLExpr->copy();
	
	// Declare pointers for the index and test variables
	SymbolExpr* pIndexVar;
	SymbolExpr* pTestVar;
	
	// Get the body statements
	StmtSequence::StmtVector bodyStmts = pNewBody->getStatements();
		
	// If the right expression is a range with a known step value
	if (pRExpr->getExprType() == Expression::RANGE)
	{
		// Declare statement vectors for the initialization, test and incrementation sequences
		StmtSequence::StmtVector initStmts;
		StmtSequence::StmtVector posTestStmts;
		StmtSequence::StmtVector negTestStmts;
		StmtSequence::StmtVector incrStmts;
		
		// Get a typed pointer to the range expression
		RangeExpr* pRangeExpr = (RangeExpr*)pRExpr;
		
		// Create an index variable
		pIndexVar = pFunction->createTemp();
		
		// Create a variable for the step value (accessible outside the loops)
		SymbolExpr* pExtStepValVar = pFunction->createTemp();
		
		// Assign the step expression to the step value variable
		stmtVector.push_back(new AssignStmt(pExtStepValVar, pRangeExpr->getStepExpr()->copy()));
		
		// Initialize the loop index to the range start value
		initStmts.push_back(new AssignStmt(pIndexVar, pRangeExpr->getStartExpr()->copy()));
		
		// Create variables for the range step and end values
		SymbolExpr* pStepValVar = pFunction->createTemp();
		SymbolExpr* pEndValVar = pFunction->createTemp();
		
		// Assign the range step and end values to the variables
		initStmts.push_back(new AssignStmt(pStepValVar, pExtStepValVar));
		initStmts.push_back(new AssignStmt(pEndValVar, pRangeExpr->getEndExpr()->copy()));

		// Create a variable for the loop test value
		pTestVar = pFunction->createTemp();
		
		// Create the loop test statement for positive stepping values
		posTestStmts.push_back(
			new AssignStmt(
				pTestVar,
				new BinaryOpExpr(
					BinaryOpExpr::LESS_THAN_EQ,
					pIndexVar,
					pEndValVar
				)
			)
		);
		
		// Create the loop test statement for negative stepping values
		negTestStmts.push_back(
			new AssignStmt(
				pTestVar,
				new BinaryOpExpr(
					BinaryOpExpr::GREATER_THAN_EQ,
					pIndexVar,
					pEndValVar
				)
			)
		);	
		
		// Assign the index variable to the loop variable at the beginning of the loop body
		bodyStmts.insert(
			bodyStmts.begin(), 
			new AssignStmt(pLoopVar, pIndexVar)
		);
		
		// Create the incrementation statement
		incrStmts.push_back(
			new AssignStmt(
				pIndexVar,
				new BinaryOpExpr(
					BinaryOpExpr::PLUS,
					pIndexVar,
					pStepValVar
				)
			)
		);
				
		// Declare flags to indicate that the stepping value is known to be positive or negative
		bool stepKnownPos = false;
		bool stepKnownNeg = false;		
		
		// If the stepping expression is an integer constant
		if (pRangeExpr->getStepExpr()->getExprType() == Expression::INT_CONST)
		{	
			// Get the stepping constant value
			IntConstExpr* pStepConst = (IntConstExpr*)pRangeExpr->getStepExpr();
			int64 value = pStepConst->getValue();
			
			// Determine whether the constant is positive or negative
			if (value > 0) stepKnownPos = true;
			else if (value <= 0) stepKnownNeg = true;
		}
		
		// If the stepping expression is a floating-point constant
		else if (pRangeExpr->getStepExpr()->getExprType() == Expression::FP_CONST)
		{
			// Get the stepping constant value
			FPConstExpr* pStepConst = (FPConstExpr*)pRangeExpr->getStepExpr();
			float64 value = pStepConst->getValue();	
			
			// Determine whether the constant is positive or negative
			if (value > 0) stepKnownPos = true;
			else if (value <= 0) stepKnownNeg = true;
		}
		
		// Create statement sequences for the init, test, body and incrementation statements
		StmtSequence* pInitSeq = new StmtSequence(initStmts);
		StmtSequence* pPosTestSeq = new StmtSequence(posTestStmts);
		StmtSequence* pNegTestSeq = new StmtSequence(negTestStmts);
		StmtSequence* pBodySeq = new StmtSequence(bodyStmts);
		StmtSequence* pIncrSeq = new StmtSequence(incrStmts);
		
		// If the step value is known to be positive
		if (stepKnownPos)
		{
			// Create and a new loop statement and add it to the statement vector
			stmtVector.push_back(new LoopStmt(
				pIndexVar,
				pTestVar,
				pInitSeq,
				pPosTestSeq,
				pBodySeq,
				pIncrSeq,
                pForStmt->getAnnotations()
			));
		}
		
		// Otherwise, if the step value is known to be negative
		else if (stepKnownNeg)
		{
			// Create and a new loop statement and add it to the statement vector
			stmtVector.push_back(new LoopStmt(
				pIndexVar,
				pTestVar,
				pInitSeq,
				pNegTestSeq,
				pBodySeq,
				pIncrSeq,
                pForStmt->getAnnotations()
			));			
		}
		
		// Otherwise, the step value has unknown sign
		else
		{
			// Create a loop for the positive value case
			LoopStmt* pPosLoop = new LoopStmt(
				pIndexVar,
				pTestVar,
				pInitSeq,
				pPosTestSeq,
				pBodySeq,
				pIncrSeq,
                pForStmt->getAnnotations()	
			);
			
			// Create a loop for the negative value case
			LoopStmt* pNegLoop = new LoopStmt(
				pIndexVar,
				pTestVar,
				pInitSeq->copy(),
				pNegTestSeq,
				pBodySeq->copy(),
				pIncrSeq->copy(),
                pForStmt->getAnnotations()	
			);
			
			// Create an expression to test the sign of the step value
			Expression* pStepTestExpr = new BinaryOpExpr(
				BinaryOpExpr::GREATER_THAN,
				pExtStepValVar,
				new IntConstExpr(0)
			);
			
			// Create an if-else statement to branch to either loop based on the step value sign
			stmtVector.push_back(new IfElseStmt(
				pStepTestExpr,
				new StmtSequence(pPosLoop),
				new StmtSequence(pNegLoop)
			));	
		}		
	}
	
	// Otherwise, if the right expression is not a range
	else
	{
		// Declare statement vectors for the initialization, test and incrementation sequences
		StmtSequence::StmtVector initStmts;
		StmtSequence::StmtVector testStmts;
		StmtSequence::StmtVector incrStmts;
		
		// Create a variable to store the right-hand expression (a vector or matrix)
		SymbolExpr* pVectorVar = pFunction->createTemp();
			
		// Assign the right-hand expression to the vector variable
		initStmts.push_back(new AssignStmt(pVectorVar, pRExpr->copy()));
		
		// Create an index variable
		pIndexVar = pFunction->createTemp();
		
		// Initialize the index variable to 1
		initStmts.push_back(
			new AssignStmt(
				pIndexVar,
				new IntConstExpr(1)
			)
		);
		
		// Create a variable to store the vector length
		SymbolExpr* pLengthVar = pFunction->createTemp();
			
		// Store the vector length in the length variable
		ParamExpr::ExprVector numelArgs;
		numelArgs.push_back(pVectorVar);
		initStmts.push_back(
			new AssignStmt(
				pLengthVar,
				new ParamExpr(
					SymbolExpr::getSymbol("numel"),
					numelArgs
				)
			)
		);
		
		// Create a variable for the loop test value
		pTestVar = pFunction->createTemp();
		
		// Create the loop test statement
		testStmts.push_back(
			new AssignStmt(
				pTestVar,
				new BinaryOpExpr(
					BinaryOpExpr::LESS_THAN_EQ,
					pIndexVar,
					pLengthVar
				)
			)
		);
		
		// Create the index incrementation statement
		incrStmts.push_back(
			new AssignStmt(
				pIndexVar,
				new BinaryOpExpr(
					BinaryOpExpr::PLUS,
					pIndexVar,
					new IntConstExpr(1)
				)
			)
		);
				
		// Assign the loop variable to the current vector element
		ParamExpr::ExprVector indexArgs;
		indexArgs.push_back(pIndexVar);
		bodyStmts.insert(
			bodyStmts.begin(),
			new AssignStmt(
				pLoopVar,
				new ParamExpr(
					pVectorVar,
					indexArgs
				)
			)
		);
		
		// Create and a new loop statement and add it to the statement vector
		stmtVector.push_back(new LoopStmt(
			pIndexVar,
			pTestVar,
			new StmtSequence(initStmts),
			new StmtSequence(testStmts),
			new StmtSequence(bodyStmts),
			new StmtSequence(incrStmts),
            pForStmt->getAnnotations()
		));
	}
}

/***************************************************************
* Function: transformWhileLoop()
* Purpose : Simplify a while loop
* Initial : Maxime Chevalier-Boisvert on January 13, 2009
****************************************************************
Revisions and bug fixes:
*/
void transformWhileLoop(WhileStmt* pWhileStmt, StmtSequence::StmtVector& stmtVector, ProgFunction* pFunction)
{
	// Transform the loop body
	StmtSequence* pNewBody = transformLoops(pWhileStmt->getLoopBody(), pFunction);
	
	// Declare a statement vector for the test sequence
	StmtSequence::StmtVector testStmts;
	
	// Create a variable to store the test condition result
	SymbolExpr* pTestVar = pFunction->createTemp();
	
	// Assign the loop test to this variable
	testStmts.push_back(
		new AssignStmt(
			pTestVar,
			pWhileStmt->getCondExpr()->copy()
		)
	);	
	
	// Create a new loop statement and add it to the statement vector
	stmtVector.push_back(new LoopStmt(
		NULL,
		pTestVar,
		new StmtSequence(),
		new StmtSequence(testStmts),
		pNewBody,
		new StmtSequence(),
        pWhileStmt->getAnnotations()		
	));
}
