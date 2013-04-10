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
#include "transform_split.h"
#include "utility.h"
#include "exprstmt.h"
#include "assignstmt.h"
#include "unaryopexpr.h"
#include "binaryopexpr.h"
#include "symbolexpr.h"
#include "ifelsestmt.h"

/***************************************************************
* Function: splitSequence()
* Purpose : Convert a sequence of statements to split form
* Initial : Maxime Chevalier-Boisvert on January 7, 2009
****************************************************************
Revisions and bug fixes:
*/
StmtSequence* splitSequence(StmtSequence* pSeq, ProgFunction* pFunction)
{
	// Get a reference to the statement vector
	const StmtSequence::StmtVector& seqStmts = pSeq->getStatements();
	
	// Create a statement vector to store the split output
	StmtSequence::StmtVector stmtVector;
	
	// For each statement in the sequence
	for (StmtSequence::StmtVector::const_iterator stmtItr = seqStmts.begin(); stmtItr != seqStmts.end(); ++stmtItr)
	{
		// Convert this statement to split form
		StmtSequence::StmtVector splitOutput = splitStatement(*stmtItr, pFunction);
		
		// Add the converted statements to the output vector
		stmtVector.insert(stmtVector.end(), splitOutput.begin(), splitOutput.end());
	}
	
	// Return the split output
	return new StmtSequence(stmtVector);
}

/***************************************************************
* Function: splitStatement()
* Purpose : Convert a statement to split form
* Initial : Maxime Chevalier-Boisvert on January 7, 2009
****************************************************************
Revisions and bug fixes:
*/
StmtSequence::StmtVector splitStatement(Statement* pStmt, ProgFunction* pFunction)
{
	// Create a statement vector to store the converted statements
	StmtSequence::StmtVector stmtVector;
	
	// Switch on the statement type
	switch (pStmt->getStmtType())
	{
		// Expression statement
		case Statement::EXPR:
		{
			// Get a typed pointer to the statement
			ExprStmt* pExprStmt = (ExprStmt*)pStmt;
			
			// Declare a variable to store the top-level expression created
			Expression* pTopExpr = NULL;
			
			// Split the expression contained in the statement
			splitExpression(pExprStmt->getExpression(), stmtVector, pTopExpr, pFunction);
			
			// Add a statement for the last expression
			stmtVector.push_back(new ExprStmt(pTopExpr, pExprStmt->getSuppressFlag()));
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
			{
				// Declare a variable to store the last right expression	
				Expression* pLExpr = NULL;
				
				// Split the left expression
				splitExpression(*itr, stmtVector, pLExpr, pFunction);
				
				// Add the expression to the list
				newLeftExprs.push_back(pLExpr);
			}
			
			// Declare a variable to store the last right expression
			Expression* pRExpr = NULL;
			
			// Split the right expression
			splitExpression(pAssignStmt->getRightExpr(), stmtVector, pRExpr, pFunction);
			
			// Create a new assignment statement for the top-level expressions
			stmtVector.push_back(new AssignStmt(newLeftExprs, pRExpr, pAssignStmt->getSuppressFlag()));			
		}
		break;
		
		// If-else statement
		case Statement::IF_ELSE:
		{
			// Get a typed pointer to the statement
			IfElseStmt* pIfStmt = (IfElseStmt*)pStmt;
			
			// Declare a variable to store the tSequenceStmt::StmtVectorop-level test expression
			Expression* pTopTest = NULL;
			
			// Split the test expression
			bool exprSplit = splitExpression(pIfStmt->getCondition(), stmtVector, pTopTest, pFunction);
			
			// If the test expression was split
			if (exprSplit)
			{
				// Create a new symbol object for the test variable
				SymbolExpr* pTestVar = pFunction->createTemp();
			
				// Assign the test sub-expression to the temp variable
				stmtVector.push_back(new AssignStmt(pTestVar, pTopTest));
				
				// Replace the top test by the test variable
				pTopTest = pTestVar;
			}
			
			// Convert the if and else blocks to split form
			StmtSequence* pNewIfBlock = splitSequence(pIfStmt->getIfBlock(), pFunction);
			StmtSequence* pNewElseBlock = splitSequence(pIfStmt->getElseBlock(), pFunction);
			
			// Create a new if-else statement and add it to the vector
			stmtVector.push_back(new IfElseStmt(pTopTest, pNewIfBlock, pNewElseBlock));			
		}
		break;
		
		// Loop statement
		case Statement::LOOP:
		{
			// Split the loop statement and add it to the vector
			stmtVector.push_back(splitLoopStmt((LoopStmt*)pStmt, pFunction));
		}
		break;
		
		// Default case, all other statement types
		default:
		{
			// Simply copy the statement without altering it
			stmtVector.push_back(pStmt->copy());
		}
		break;
	}	
	
	// Return the statement vector
	return stmtVector;
}

/***************************************************************
* Function: splitLoopStmt()indentText(m_pLoopBody->toString())
* Purpose : Convert a loop statement to split form
* Initial : Maxime Chevalier-Boisvert on January 12, 2009
****************************************************************
Revisions and bug fixes:
*/
LoopStmt* splitLoopStmt(LoopStmt* pLoopStmt, ProgFunction* pFunction)
{
	// Split the statement sequences of the loop
	StmtSequence* pNewInitSeq = splitSequence(pLoopStmt->getInitSeq(), pFunction);
	StmtSequence* pNewTestSeq = splitSequence(pLoopStmt->getTestSeq(), pFunction);
	StmtSequence* pNewBodySeq = splitSequence(pLoopStmt->getBodySeq(), pFunction);
	StmtSequence* pNewIncrSeq = splitSequence(pLoopStmt->getIncrSeq(), pFunction);
	
	// Return a new loop statement
	return new LoopStmt(
		pLoopStmt->getIndexVar(),
		pLoopStmt->getTestVar(),
		pNewInitSeq,
		pNewTestSeq,
		pNewBodySeq,
		pNewIncrSeq,
        pLoopStmt->getAnnotations()
	);
}

/***************************************************************
* Function: splitExpression()
* Purpose : Convert an expression to split form
* Initial : Maxime Chevalier-Boisvert on January 7, 2009
****************************************************************
Revisions and bug fixes:
*/
bool splitExpression(Expression* pExpr, StmtSequence::StmtVector& stmtVector, Expression*& pTopExpr, ProgFunction* pFunction)
{
	//
	// For each sub-expression:
	// - Split it recursively
	// - Assign top-level expression to a temp
	// - Replace sub-expr by temp var
	//
	// Take a temp from list to assign sub-expr to temp
	// Once temps are used, they become free again
	//
	
	// Make a copy of the expression to use as the top-level expression
	pTopExpr = pExpr->copy();
	
	// If the expression is a lambda expression, perform no splitting and return early
	if (pTopExpr->getExprType() == Expression::LAMBDA)
		return false;
	
	// Get the list of sub-expressions for this expression
	Expression::ExprVector subExprs = pTopExpr->getSubExprs();
	
	// If there are no sub-expressions, perform no splitting and return early
	if (subExprs.empty())
		return false;
	
	// For each sub-expression
	for (size_t i = 0; i < subExprs.size(); ++i)
	{
		// Get a pointer to the sub-expression
		Expression* pSubExpr = subExprs[i];
		
		// If the sub-expression is null, skip it
		if (pSubExpr == NULL)
			continue;
		
		// Declare a variable to store the top-level expression
		Expression* pTopSubExpr = NULL;
		
		// Split the sub-expression
		bool exprSplit = splitExpression(pSubExpr, stmtVector, pTopSubExpr, pFunction);
		
		// If the sub-expression was not split, skip it
		if (!exprSplit)
			continue;
		
		// If the current expression is a parameterized or
		// cell indexing expression and the sub-expression
		// is a range expressions
		if ((pTopExpr->getExprType() == Expression::PARAM ||
			pTopExpr->getExprType() == Expression::CELL_INDEX) &&
			pTopSubExpr->getExprType() == Expression::RANGE)
		{
			// Replace the sub-expression by the top sub-expression directly
			pTopExpr->replaceSubExpr(i, pTopSubExpr);
		}
		// If the current expression is a parameterized 
		// expression and the sub-expression is a cell-indexing
		// expression
		else if (pTopExpr->getExprType() == Expression::PARAM &&
				pTopSubExpr->getExprType() == Expression::CELL_INDEX) 
		{
			// Replace the sub-expression by the top sub-expression directly
			pTopExpr->replaceSubExpr(i, pTopSubExpr);
		}
		else
		{
			// Create a new symbol object for the temp variable
			SymbolExpr* pTempVar = pFunction->createTemp();
			
			// Assign the sub-expression to the temp variable
			stmtVector.push_back(new AssignStmt(pTempVar, pTopSubExpr));
			
			// Replace the sub-expression use by the temp variable
			pTopExpr->replaceSubExpr(i, pTempVar);
		}
	}
	
	// Expression split
	return true;
}
