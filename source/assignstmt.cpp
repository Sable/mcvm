// =========================================================================== //
//                                                                             //
// Copyright 2008 Maxime Chevalier-Boisvert and McGill University.             //
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
#include "assignstmt.h"
#include "paramexpr.h"
#include "cellindexexpr.h"

/***************************************************************
* Function: AssignStmt::copy()
* Purpose : Copy this IIR node recursively
* Initial : Maxime Chevalier-Boisvert on November 6, 2008
****************************************************************
Revisions and bug fixes:
*/
AssignStmt* AssignStmt::copy() const
{
	// Create a vector for the left expression copies
	ExprVector leftExprs;
	
	// Copy each left expression
	for (ExprVector::const_iterator itr = m_leftExprs.begin(); itr != m_leftExprs.end(); ++itr)
		leftExprs.push_back((*itr)->copy());
	
	// Create and return a copy of this node
	return new AssignStmt(
		leftExprs,
		m_pRightExpr->copy(),
		m_suppressOut
	);
}	

/***************************************************************
* Function: AssignStmt::toString()
* Purpose : Generate a text representation of this IIR node
* Initial : Maxime Chevalier-Boisvert on November 6, 2008
****************************************************************
Revisions and bug fixes:
*/
std::string AssignStmt::toString() const
{
	// Create a string for the output
	std::string output;
	
	// If there are more than one left expressions
	if (m_leftExprs.size() > 1)
	{
		// Add an opening bracket to the output
		output += "[";
		
		// For each left expression
		for (ExprVector::const_iterator itr = m_leftExprs.begin(); itr != m_leftExprs.end(); ++itr)
		{
			// Add this expression to the string
			output += (*itr)->toString();
			
			// If this is not the last left expression, add a space separator
			if (itr != --m_leftExprs.end())
				output += " ";
		}
		
		// Add a closing bracket to the output
		output += "]";
	}
	else
	{
		// Add the left expression to the output directly
		output += m_leftExprs.front()->toString();
	}
	
	// Add the equal sign and the right expression to the output 
	output += " = " + m_pRightExpr->toString();
	
	// Return the output string
	return output;
}

/***************************************************************
* Function: AssignStmt::getSymbolUses()
* Purpose : Get all symbols read/used by this expression
* Initial : Maxime Chevalier-Boisvert on March 17, 2009
****************************************************************
Revisions and bug fixes:
*/
Expression::SymbolSet AssignStmt::getSymbolUses() const
{
	// Create a set to store the defined symbols
	Expression::SymbolSet symbols;
	
	// Get the symbols used by the right-expression
	Expression::SymbolSet rightSymbols = m_pRightExpr->getSymbolUses();
	
	// Add those symbols to the set
	symbols.insert(rightSymbols.begin(), rightSymbols.end());
	
	// For each left-expression
	for (ExprVector::const_iterator itr = m_leftExprs.begin(); itr != m_leftExprs.end(); ++itr)
	{
		// Get a pointer to this expression
		Expression* pExpr = *itr;
		
		// Switch on the expression type
		switch (pExpr->getExprType())
		{
			// Symbol expression
			case Expression::SYMBOL:
			break;
			
			// Parameterized expression
			case Expression::PARAM:
			{
				// Get a typed pointer to the expression
				ParamExpr* pLeftExpr = (ParamExpr*)pExpr;
				
				// Get the symbols used by this expression
				Expression::SymbolSet exprSymbols = pLeftExpr->getSymbolUses();
				
				// Add those symbols to the main set
				symbols.insert(exprSymbols.begin(), exprSymbols.end());
			}				
			break;

			// Cell indexing expression
			case Expression::CELL_INDEX:
			{
				// Get a typed pointer to the expression
				CellIndexExpr* pLeftExpr = (CellIndexExpr*)pExpr;
				
				// Get the symbols used by this expression
				Expression::SymbolSet exprSymbols = pLeftExpr->getSymbolUses();
				
				// Add those symbols to the main set
				symbols.insert(exprSymbols.begin(), exprSymbols.end());
			}	
			break;
			
			// Invalid expression type
			default: assert (false);
		}
	}		
	
	// Return the set of symbols
	return symbols;
}

/***************************************************************
* Function: AssignStmt::getSymbolDefs()
* Purpose : Get all symbols written/defined by this expression
* Initial : Maxime Chevalier-Boisvert on March 17, 2009
****************************************************************
Revisions and bug fixes:
*/
Expression::SymbolSet AssignStmt::getSymbolDefs() const
{
	// Create a set to store the defined symbols
	Expression::SymbolSet symbols;
	
	// For each left-expression
	for (ExprVector::const_iterator itr = m_leftExprs.begin(); itr != m_leftExprs.end(); ++itr)
	{
		// Get a pointer to this expression
		Expression* pExpr = *itr;
		
		// Switch on the expression type
		switch (pExpr->getExprType())
		{
			// Symbol expression
			case Expression::SYMBOL:
			symbols.insert((SymbolExpr*)pExpr);
			break;
			
			// Parameterized expression
			case Expression::PARAM:
			symbols.insert(((ParamExpr*)pExpr)->getSymExpr());
			break;

			// Cell indexing expression
			case Expression::CELL_INDEX:
			symbols.insert(((CellIndexExpr*)pExpr)->getSymExpr());
			break;
			
			// Invalid expression type
			default: assert (false);
		}
	}
	
	// Return the set of symbols
	return symbols;
}
