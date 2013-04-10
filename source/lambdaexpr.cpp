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
#include "lambdaexpr.h"

/***************************************************************
* Function: LambdaExpr::copy()
* Purpose : Recursively copy this node
* Initial : Maxime Chevalier-Boisvert on February 25, 2009
****************************************************************
Revisions and bug fixes:
*/
LambdaExpr* LambdaExpr::copy() const
{
	// Create a vector to store the input parameter copies
	ParamVector inParams;
	
	// Copy the input parameters
	for (ParamVector::const_iterator itr = m_inParams.begin(); itr != m_inParams.end(); ++itr)
		inParams.push_back((SymbolExpr*)(*itr)->copy());
	
	// Create and return a new lambda expression object
	return new LambdaExpr(
		inParams,
		m_pBodyExpr->copy()
	);
}

/***************************************************************
* Function: LambdaExpr::toString()
* Purpose : Obtain a string representation of this node
* Initial : Maxime Chevalier-Boisvert on February 25, 2009
****************************************************************
Revisions and bug fixes:
*/
std::string LambdaExpr::toString() const
{
	// Create a string to store the output
	std::string output;
	
	// Begin the input parameter list
	output += "@(";
	
	// Copy the input parameters
	for (ParamVector::const_iterator itr = m_inParams.begin(); itr != m_inParams.end(); ++itr)
	{
		// Add this parameter to the output
		output += (*itr)->toString();
		
		// If this is not the last parameter, add a separator
		if (itr != --m_inParams.end())
			output += ", ";
	}
	
	// End the input parameter list and add the body expression
	output += ") " + m_pBodyExpr->toString();	
	
	// Return the output string
	return output;
}

/***************************************************************
* Function: LambdaExpr::replaceSubExpr()
* Purpose : Replace a sub-expression
* Initial : Maxime Chevalier-Boisvert on March 17, 2008
****************************************************************
Revisions and bug fixes:
*/
void LambdaExpr::replaceSubExpr(size_t index, Expression* pNewExpr)
{
	// Switch on the sub-expression index
	switch (index)
	{
		// Replace the appropriate sub-expression
		case 0:	m_pBodyExpr = pNewExpr; break;	
		default: assert (false);
	}
}

/***************************************************************
* Function: LambdaExpr::getSymbolUses()
* Purpose : Get all symbols read/used by this expression
* Initial : Maxime Chevalier-Boisvert on March 17, 2009
****************************************************************
Revisions and bug fixes:
*/
Expression::SymbolSet LambdaExpr::getSymbolUses() const
{
	// Get the symbol uses for the body expression
	SymbolSet symbols = m_pBodyExpr->getSymbolUses(); 
	
	// Remove the input parameters from the symbol set
	// This is done because those parameters are bound
	// We only want the unbound variables as uses
	for (ParamVector::const_iterator itr = m_inParams.begin(); itr != m_inParams.end(); ++itr)
		symbols.erase(*itr);
	
	// Return the set of symbols
	return symbols;
}
