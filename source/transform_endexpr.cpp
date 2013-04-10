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
#include "transform_endexpr.h"
#include "paramexpr.h"
#include "cellindexexpr.h"
#include "constexprs.h"
#include "exprstmt.h"
#include "loopstmts.h"
#include "ifelsestmt.h"
#include "stdlib.h"

/***************************************************************
* Function: processEndExpr(StmtSequence*)
* Purpose : Pre-process range end expressions in sequences
* Initial : Maxime Chevalier-Boisvert on March 1, 2009
****************************************************************
Revisions and bug fixes:
*/
StmtSequence* processEndExpr(StmtSequence* pSeq, ProgFunction* pFunction)
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
				output.push_back(
					new ExprStmt(
						processEndExpr(pExprStmt->getExpression(), pFunction), 
						pExprStmt->getSuppressFlag()
					)
				);
			}
			break;

			// Assignment statement
			case Statement::ASSIGN:
			{
				// Get a typed pointer to the statement 
				AssignStmt* pAssignStmt = (AssignStmt*)pStmt;
			
				// Get a reference to the left expressions
				const AssignStmt::ExprVector& leftExprs = pAssignStmt->getLeftExprs();
				
				// Create a vector for the transformed left-expressions
				AssignStmt::ExprVector newLExprs;
				
				// Transform the left-side expressions
				for (AssignStmt::ExprVector::const_iterator itr = leftExprs.begin(); itr != leftExprs.end(); ++itr)
					newLExprs.push_back(processEndExpr(*itr, pFunction));
				
				// Transform the right-side expression
				Expression* pRightExpr = processEndExpr(pAssignStmt->getRightExpr(), pFunction);
				
				// Create and return the transformed assignment statement object
				output.push_back(new AssignStmt(newLExprs, pRightExpr, pAssignStmt->getSuppressFlag()));
			}
			break;
			
			// If-else statement
			case Statement::IF_ELSE:
			{
				// Get a typed pointer to the statement 
				IfElseStmt* pIfStmt = (IfElseStmt*)pStmt;
				
				// Transform the statement
				output.push_back(
					new IfElseStmt(
						processEndExpr(pIfStmt->getCondition(), pFunction),
						processEndExpr(pIfStmt->getIfBlock(), pFunction),
						processEndExpr(pIfStmt->getElseBlock(), pFunction)
					)
				);
			}
			break;
			
			// Loop statement
			case Statement::LOOP:
			{
				// Get a typed pointer to the statement 
				LoopStmt* pLoopStmt = (LoopStmt*)pStmt;

				// Transform the expression
				output.push_back(
					new LoopStmt(
						pLoopStmt->getIndexVar()? pLoopStmt->getIndexVar()->copy():NULL,
						pLoopStmt->getTestVar()? pLoopStmt->getTestVar()->copy():NULL,
						processEndExpr(pLoopStmt->getInitSeq(), pFunction),
						processEndExpr(pLoopStmt->getTestSeq(), pFunction),
						processEndExpr(pLoopStmt->getBodySeq(), pFunction),
						processEndExpr(pLoopStmt->getIncrSeq(), pFunction),
                        pLoopStmt->getAnnotations()
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
* Function: processEndExpr(Expression*)
* Purpose : Pre-process range end sub-expressions in expressions
* Initial : Maxime Chevalier-Boisvert on March 1, 2009
****************************************************************
Revisions and bug fixes:
*/
Expression* processEndExpr(Expression* pExpr, ProgFunction* pFunction, const EndExpr::AssocVector& assocs)
{
	// Switch on the expression type
	switch (pExpr->getExprType())
	{
		// Parameterized expression
		case Expression::PARAM:
		{
			// Get a typed pointer to the expression
			ParamExpr* pParamExpr = (ParamExpr*)pExpr;

			// Get the symbol expression
			SymbolExpr* pSymbol = pParamExpr->getSymExpr();
			
			// Get the argument vector
			const ParamExpr::ExprVector& args = pParamExpr->getArguments();
			
			// Create a vector for the transformed arguments
			ParamExpr::ExprVector newArgs;
			
			// For each argument
			for (size_t i = 0; i < args.size(); ++i)
			{
				// Create a new association vector
				EndExpr::AssocVector newAssocs;
				
				// Create an association for this argument and add it to the vector
				newAssocs.push_back(
					EndExpr::Assoc(
						pSymbol,
						i,
						i == args.size() - 1
					)
				);
				
				// Add the higher-level associations to the end of the vector
				newAssocs.insert(newAssocs.end(), assocs.begin(), assocs.end());
				
				// Transform this argument expression
				newArgs.push_back(
					processEndExpr(
						args[i],
						pFunction,
						newAssocs
					)
				);
			}
			
			// Return the transformed parameterized expression
			return new ParamExpr(pSymbol->copy(), newArgs);
		}
		break;

		// Cell indexing expression
		case Expression::CELL_INDEX:
		{
			// Get a typed pointer to the expression
			CellIndexExpr* pCellExpr = (CellIndexExpr*)pExpr;
			
			// Get the symbol expression
			SymbolExpr* pSymbol = pCellExpr->getSymExpr();
			
			// Get the argument vector
			const CellIndexExpr::ExprVector& args = pCellExpr->getArguments();
			
			// Create a vector for the transformed arguments
			CellIndexExpr::ExprVector newArgs;
			
			// For each argument
			for (size_t i = 0; i < args.size(); ++i)
			{
				// Create a new association vector
				EndExpr::AssocVector newAssocs;
				
				// Create an association for this argument and add it to the vector
				newAssocs.push_back(
					EndExpr::Assoc(
						pSymbol,
						i,
						i == args.size() - 1
					)
				);
				
				// Add the higher-level associations to the end of the vector
				newAssocs.insert(newAssocs.end(), assocs.begin(), assocs.end());
				
				// Transform this argument expression
				newArgs.push_back(
					processEndExpr(
						args[i],
						pFunction,
						newAssocs
					)
				);
			}
			
			// Return the transformed parameterized expression
			return new CellIndexExpr(pSymbol->copy(), newArgs);			
		}
		break;
		
		// Range end expression
		case Expression::END:
		{			
			// Ensure associations were found
			assert (assocs.empty() == false);

			// Create and return a new range end expression with the matrix associations
			return new EndExpr(assocs);			
		}
		break;
	
		// All other expression types
		default:
		{
			// Copy the expression
			pExpr = pExpr->copy();			
			
			// Get the list of sub-expressions for this expression
			Expression::ExprVector subExprs = pExpr->getSubExprs();

			// For each sub-expression
			for (size_t i = 0; i < subExprs.size(); ++i)
			{
				// Get a pointer to the sub-expression
				Expression* pSubExpr = subExprs[i];
				
				// If this sub-expression is NULL, skip it
				if (pSubExpr == NULL)
					continue;
				
				// Transform this sub-expression
				Expression* pNewExpr = processEndExpr(pSubExpr, pFunction, assocs);
				
				// Replace the sub-expression by the transformed version
				pExpr->replaceSubExpr(i, pNewExpr);
			}			
			
			// Return the modified expression
			return pExpr;
		}
	}
}
