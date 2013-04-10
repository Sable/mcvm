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
#include "binaryopexpr.h"

/***************************************************************
* Function: BinaryOpExpr::copy()
* Purpose : Copy this IIR node recursively
* Initial : Maxime Chevalier-Boisvert on November 6, 2008
****************************************************************
Revisions and bug fixes:
*/
BinaryOpExpr* BinaryOpExpr::copy() const
{
	// Create and return a copy of this node
	return new BinaryOpExpr(
		m_operator, 
		m_pLeftExpr->copy(), 
		m_pRightExpr->copy()
	);
}	

/***************************************************************
* Function: BinaryOpExpr::toString()
* Purpose : Generate a text representation of this IIR node
* Initial : Maxime Chevalier-Boisvert on November 6, 2008
****************************************************************
Revisions and bug fixes:
*/
std::string BinaryOpExpr::toString() const
{
	// Declare a variable for the operator string
	std::string opString;
	
	// Switch on the operator string
	switch (m_operator)
	{
		// Set the operator string according to the operator
		case PLUS:				opString = "+"; break;
		case MINUS:				opString = "-"; break;
		case MULT:				opString = "*"; break;
		case ARRAY_MULT:		opString = ".*"; break;
		case DIV:				opString = "/"; break;
		case ARRAY_DIV:			opString = "./"; break;
		case LEFT_DIV:			opString = "\\"; break;
		case ARRAY_LEFT_DIV:	opString = ".\\"; break;
		case POWER:				opString = "^"; break;
		case ARRAY_POWER:		opString = ".^"; break;
		case EQUAL:				opString = "=="; break;
		case NOT_EQUAL:			opString = "~="; break;
		case LESS_THAN:			opString = "<"; break;
		case LESS_THAN_EQ:		opString = "<="; break;
		case GREATER_THAN:		opString = ">"; break;
		case GREATER_THAN_EQ:	opString = ">="; break;
		case OR:				opString = "||"; break;
		case ARRAY_OR:			opString = "|"; break;
		case AND:				opString = "&&"; break;
		case ARRAY_AND:			opString = "&"; break;
	}
	
	// Concatenate the expression string 
	std::string output = "(" + m_pLeftExpr->toString() + " " + opString + " " + m_pRightExpr->toString() + ")";

	// Return the output
	return output;
}

/***************************************************************
* Function: BinaryOpExpr::replaceSubExpr()
* Purpose : Replace a sub-expression
* Initial : Maxime Chevalier-Boisvert on March 17, 2008
****************************************************************
Revisions and bug fixes:
*/
void BinaryOpExpr::replaceSubExpr(size_t index, Expression* pNewExpr)
{
	// Switch on the sub-expression index
	switch (index)
	{
		// Replace the appropriate sub-expression
		case 0:	m_pLeftExpr = pNewExpr; break;	
		case 1: m_pRightExpr = pNewExpr; break;
		default: assert (false);
	}
}
