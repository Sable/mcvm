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
#include "rangeexpr.h"
#include "constexprs.h"
#include "utility.h"

/***************************************************************
* Function: RangeExpr::copy()
* Purpose : Copy this IIR node recursively
* Initial : Maxime Chevalier-Boisvert on November 8, 2008
****************************************************************
Revisions and bug fixes:
*/
RangeExpr* RangeExpr::copy() const
{
	// Create and return a copy of this node
	return new RangeExpr(
		m_pStartExpr,
		m_pEndExpr,
		m_pStepExpr
	);
}	

/***************************************************************
* Function: RangeExpr::toString()
* Purpose : Generate a text representation of this IIR node
* Initial : Maxime Chevalier-Boisvert on November 8, 2008
****************************************************************
Revisions and bug fixes:
*/
std::string RangeExpr::toString() const
{
	// Declare a string for the output
	std::string output;
	
	// If this is an empty range
	if (m_pStartExpr == NULL)
	{
		// Add a colon to the output
		output += ":";
	}
	else
	{
		// Add the start value to the range
		output += m_pStartExpr->toString() + ":";
		
		// Add the step size to the range
		output += m_pStepExpr->toString() + ":";	
		
		// Add the end value to the range
		output += m_pEndExpr->toString();
	}
	
	// Return the output
	return output;
}

/***************************************************************
* Function: RangeExpr::getSubExprs()
* Purpose : Access sub-expressions
* Initial : Maxime Chevalier-Boisvert on January 8, 2009
****************************************************************
Revisions and bug fixes:
*/
Expression::ExprVector RangeExpr::getSubExprs() const
{
	// Create a list to store the sub-expression pointers
	ExprVector list; 
	
	// Add the sub-expressions to the list
	list.push_back(m_pStartExpr); 
	list.push_back(m_pStepExpr);
	list.push_back(m_pEndExpr);
	
	// Return the list
	return list;	
}

/***************************************************************
* Function: RangeExpr::replaceSubExpr()
* Purpose : Replace a sub-expression
* Initial : Maxime Chevalier-Boisvert on March 17, 2008
****************************************************************
Revisions and bug fixes:
*/
void RangeExpr::replaceSubExpr(size_t index, Expression* pNewExpr)
{
	// Switch on the sub-expression index
	switch (index)
	{
		// Replace the appropriate sub-expression
		case 0:	m_pStartExpr = pNewExpr; break;	
		case 1: m_pStepExpr = pNewExpr; break;
		case 2: m_pEndExpr = pNewExpr; break;
		default: assert (false);
	}
}
