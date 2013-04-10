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
#include "paramexpr.h"

/***************************************************************
* Function: ParamExpr::copy()
* Purpose : Copy this IIR node recursively
* Initial : Maxime Chevalier-Boisvert on November 8, 2008
****************************************************************
Revisions and bug fixes:
*/
ParamExpr* ParamExpr::copy() const
{
	// Create an argument vector to store the argument copies
	ExprVector arguments;
	
	// Copy each argument
	for (ExprVector::const_iterator itr = m_arguments.begin(); itr != m_arguments.end(); ++itr)
		arguments.push_back((Expression*)(*itr)->copy());
		
	// Create and return a copy of this node
	return new ParamExpr(
		m_pSymbolExpr->copy(),
		arguments
	);
}	

/***************************************************************
* Function: ParamExpr::toString()
* Purpose : Generate a text representation of this IIR node
* Initial : Maxime Chevalier-Boisvert on November 8, 2008
****************************************************************
Revisions and bug fixes:
*/
std::string ParamExpr::toString() const
{
	// Declare a string for the output
	std::string output;
	
	// Add the symbol expression string and open the argument list
	output += m_pSymbolExpr->toString() + "(";
	
	// For each argument
	for (ExprVector::const_iterator itr = m_arguments.begin(); itr != m_arguments.end(); ++itr)
	{
		// Add the argument to the output
		output += (*itr)->toString();
		
		// If this isn't the last argument, add a comma
		if (itr != --m_arguments.end())
			output += ", ";
	}
	
	// Close the argument list
	output += ")";

	// Return the output
	return output;
}

/***************************************************************
* Function: ParamExpr::getSubExprs()
* Purpose : Access sub-expressions
* Initial : Maxime Chevalier-Boisvert on January 8, 2009
****************************************************************
Revisions and bug fixes:
*/
Expression::ExprVector ParamExpr::getSubExprs() const
{
	// Create a list to store the sub-expression pointers
	ExprVector list;
	
	// Add the symbol to the list
	list.push_back(m_pSymbolExpr);
	
	// For each argument
	for (size_t i = 0; i < m_arguments.size(); ++i)
	{		
		// Add the sub-expression to the list
		list.push_back(m_arguments[i]);
	}
	
	// Return the list
	return list;
}

/***************************************************************
* Function: ParamExpr::replaceSubExpr()
* Purpose : Replace a sub-expression
* Initial : Maxime Chevalier-Boisvert on March 17, 2008
****************************************************************
Revisions and bug fixes:
*/
void ParamExpr::replaceSubExpr(size_t index, Expression* pNewExpr)
{
	// If the original expression is the symbol expression
	if (index == 0)
	{
		// Ensure the new expression is a symbol
		assert (pNewExpr->getExprType() == Expression::SYMBOL);
		
		// Replace the symbol expression
		m_pSymbolExpr = (SymbolExpr*)pNewExpr;
		return;
	}

	// Decrement the index by 1
	index -= 1;
	
	// Ensure the index is valid
	assert (index < m_arguments.size());
	
	// Replace the corresponding argument
	m_arguments[index] = pNewExpr;	
}
