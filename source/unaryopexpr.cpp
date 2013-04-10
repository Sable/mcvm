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
#include "unaryopexpr.h"

/***************************************************************
* Function: UnaryOpExpr::copy()
* Purpose : Copy this IIR node recursively
* Initial : Maxime Chevalier-Boisvert on November 6, 2008
****************************************************************
Revisions and bug fixes:
*/
UnaryOpExpr* UnaryOpExpr::copy() const
{
	// Create and return a copy of this node
	return new UnaryOpExpr(m_operator, (Expression*)m_pOperand->copy());
}	

/***************************************************************
* Function: BinaryOpExpr::toString()
* Purpose : Generate a text representation of this IIR node
* Initial : Maxime Chevalier-Boisvert on November 6, 2008
****************************************************************
Revisions and bug fixes:
*/
std::string UnaryOpExpr::toString() const
{
	// Declare a variable for the output string
	std::string output;
	
	// Obtain the string representation of the operand
	std::string opStr = m_pOperand->toString();
	
	// Switch on the operator string
	switch (m_operator)
	{
		// Concatenate the expression string
		case PLUS:			output = "+" + opStr;	break;
		case MINUS:			output = "-" + opStr;	break;
		case NOT:			output = "~" + opStr;	break;
		case TRANSP:		output = opStr + "'";	break;
		case ARRAY_TRANSP:	output = opStr + ".'";	break;
	}

	// Return the output string
	return output;
}

/***************************************************************
* Function: UnaryOpExpr::replaceSubExpr()
* Purpose : Replace a sub-expression
* Initial : Maxime Chevalier-Boisvert on March 17, 2008
****************************************************************
Revisions and bug fixes:
*/
void UnaryOpExpr::replaceSubExpr(size_t index, Expression* pNewExpr)
{
	// Switch on the sub-expression index
	switch (index)
	{
		// Replace the appropriate sub-expression
		case 0:	m_pOperand = pNewExpr; break;	
		default: assert (false);
	}
}
