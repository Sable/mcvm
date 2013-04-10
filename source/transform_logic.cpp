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
#include "transform_logic.h"
#include "utility.h"
#include "ifelsestmt.h"
#include "loopstmts.h"
#include "binaryopexpr.h"
#include "assignstmt.h"
#include "exprstmt.h"
#include "constexprs.h"

/*
 * Logical expansion should happen before splitting.
 * 
 * OR case
 * -------
 * 
 * c = a || (b || c)
 * 
 * Becomes:
 * 
 * if a
 *   c = true;
 * else
 *   c = (b || c);
 * end
 * 
 * a || (b || c)	(no assignment)
 * 
 * Becomes:
 * 
 * if a
 *   true;			(dead code)
 * else
 *   b || c;
 * end
 * 
 * AND case
 * --------
 * 
 * c = a && (b || c)
 * 
 * Becomes:
 * 
 * if a
 *   c = (b || c);
 * else
 *   c = false;
 * end
 * 
 * If-Else and Loops
 * -----------------
 * 
 * Assign truth value to test variable to avoid useless code duplication.
 * 
 * May be able to create an assignment statement and have it be expanded.
 * 
 * Assignment statement can use expression splitting.
 * 
 */

/***************************************************************
* Function: transformLoops()
* Purpose : Expand logical operators in a statement sequence
* Initial : Maxime Chevalier-Boisvert on January 14, 2009
****************************************************************
Revisions and bug fixes:
*/
StmtSequence* transformLogic(StmtSequence* pSeq, ProgFunction* pFunction)
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
			// Expression statement
			case Statement::EXPR:
			{
				// Get a typed pointer to the statement
				ExprStmt* pExprStmt = (ExprStmt*)pStmt;
				
				// Transform the expression
				Expression* pNewExpr = transformLogicExpr(pExprStmt->getExpression(), output, pFunction);
				
				// Create a new expression statement
				output.push_back(new ExprStmt(pNewExpr, pExprStmt->getSuppressFlag()));
			}
			break;
			
			// Assignment statement
			case Statement::ASSIGN:
			{
				// Get a typed pointer to the statement
				AssignStmt* pAssignStmt = (AssignStmt*)pStmt;
	
				// Get the left expressions for this assignment
				const AssignStmt::ExprVector& leftExprs = pAssignStmt->getLeftExprs();
				
				// Create a vector to store the transformed left expressions
				AssignStmt::ExprVector newLeftExprs;
				
				// For each left expression
				for (AssignStmt::ExprVector::const_iterator itr = leftExprs.begin(); itr != leftExprs.end(); ++itr)
					newLeftExprs.push_back(transformLogicExpr(*itr, output, pFunction));
				
				// Transform the right expression
				Expression* pNewRExpr = transformLogicExpr(pAssignStmt->getRightExpr(), output, pFunction);
				
				// Create a new assignment statement
				output.push_back(new AssignStmt(newLeftExprs, pNewRExpr, pAssignStmt->getSuppressFlag()));
			}
			break;
		
			// If-else statement
			case Statement::IF_ELSE:
			{
				// Get a typed pointer to the statement
				IfElseStmt* pIfStmt = (IfElseStmt*)pStmt;
								
				// Transform the test expression
				Expression* pNewTest = transformLogicExpr(pIfStmt->getCondition(), output, pFunction);
				
				// Convert the if and else blocks to split form
				StmtSequence* pNewIfBlock = transformLogic(pIfStmt->getIfBlock(), pFunction);
				StmtSequence* pNewElseBlock = transformLogic(pIfStmt->getElseBlock(), pFunction);
				
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
				StmtSequence* pNewInitSeq = transformLogic(pLoopStmt->getInitSeq(), pFunction);
				StmtSequence* pNewTestSeq = transformLogic(pLoopStmt->getTestSeq(), pFunction);
				StmtSequence* pNewBodySeq = transformLogic(pLoopStmt->getBodySeq(), pFunction);
				StmtSequence* pNewIncrSeq = transformLogic(pLoopStmt->getIncrSeq(), pFunction);
				
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

			// Other statement types
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
* Function: transformLoopsExpr()
* Purpose : Expand logical operators in an expression
* Initial : Maxime Chevalier-Boisvert on January 14, 2009
****************************************************************
Revisions and bug fixes:
*/
Expression* transformLogicExpr(Expression* pExpr, StmtSequence::StmtVector& stmtVector, ProgFunction* pFunction)
{
	// If this is a binary expression
	if (pExpr->getExprType() == Expression::BINARY_OP)
	{
		// Get a typed pointer to the expression
		BinaryOpExpr* pBinExpr = (BinaryOpExpr*)pExpr;
		
		// If this is a logical OR expression
		if (pBinExpr->getOperator() == BinaryOpExpr::OR)
		{
			// Create a new temp variable to store the result
			SymbolExpr* pDestVar = pFunction->createTemp();
			
			// Create a variable to store the left-expression value
			SymbolExpr* pLeftVar = pFunction->createTemp();
			
			// Create a statement vector for the left and right expression statements
			StmtSequence::StmtVector leftStmts;
			StmtSequence::StmtVector rightStmts;

			// Transform the left and right expressions
			Expression* pNewLExpr = transformLogicExpr(pBinExpr->getLeftExpr() , stmtVector, pFunction);
			Expression* pNewRExpr = transformLogicExpr(pBinExpr->getRightExpr(), rightStmts, pFunction);
			
			// Assign the left-expression value to a variable
			stmtVector.push_back(new AssignStmt(pLeftVar, pNewLExpr));
			
			// Create an assignment statement for the if block
			leftStmts.push_back(new AssignStmt(pDestVar, pLeftVar));
			
			// Create an assignment statement for the else branch
			rightStmts.push_back(new AssignStmt(pDestVar, pNewRExpr));
			
			// Create a new if-statement to replace the logical operator
			stmtVector.push_back(
				new IfElseStmt(
					pLeftVar,
					new StmtSequence(leftStmts),
					new StmtSequence(rightStmts)
				)
			);
			
			// Return the destination variable as the expression to use
			return pDestVar;
		}
		
		// If this is a logical AND expression
		else if (pBinExpr->getOperator() == BinaryOpExpr::AND)
		{
			// Create a new temp variable to store the result
			SymbolExpr* pDestVar = pFunction->createTemp();
			
			// Create a variable to store the left-expression value
			SymbolExpr* pLeftVar = pFunction->createTemp();
			
			// Create a statement vector for the left and right expression statements
			StmtSequence::StmtVector leftStmts;
			StmtSequence::StmtVector rightStmts;

			// Transform the left and right expressions
			Expression* pNewLExpr = transformLogicExpr(pBinExpr->getLeftExpr() , stmtVector, pFunction);
			Expression* pNewRExpr = transformLogicExpr(pBinExpr->getRightExpr(), rightStmts, pFunction);
			
			// Assign the left-expression value to a variable
			stmtVector.push_back(new AssignStmt(pLeftVar, pNewLExpr));
			
			// Create an assignment statement for the if branch
			leftStmts.push_back(new AssignStmt(pDestVar, pNewRExpr));
			
			// Create an assignment statement for the else block
			rightStmts.push_back(new AssignStmt(pDestVar, pLeftVar));
			
			// Create a new if-statement to replace the logical operator
			stmtVector.push_back(
				new IfElseStmt(
					pLeftVar,
					new StmtSequence(leftStmts),
					new StmtSequence(rightStmts)
				)
			);
			
			// Return the destination variable as the expression to use
			return pDestVar;
		}	
	}
	
	// At this point, we know the expression is not a logical expression
	// However, it could contain a logical sub-expression
	
	// Make a copy of the expression to use as the new expression
	Expression* pNewExpr = pExpr->copy();
	
	// Get the list of sub-expressions for this expression
	Expression::ExprVector subExprs = pNewExpr->getSubExprs();

	// For each sub-expression
	for (size_t i = 0; i < subExprs.size(); ++i)
	{
		// Get a pointer to the sub-expression
		Expression* pSubExpr = subExprs[i];

		// If the sub-expression is null, skip it
		if (pSubExpr == NULL)
			continue;
		
		// Transform the sub-expression recursively
		Expression* pNewSubExpr = transformLogicExpr(pSubExpr, stmtVector, pFunction);
	
		// Replace the sub-expression
		pExpr->replaceSubExpr(i, pNewSubExpr);
	}
	
	// Return the new expression
	return pNewExpr;
}
