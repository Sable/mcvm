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
#include "expressions.h"

/***************************************************************
* Function: Expression::getSymbolUses()
* Purpose : Get all symbols read/used by this expression
* Initial : Maxime Chevalier-Boisvert on March 17, 2009
****************************************************************
Revisions and bug fixes:
*/
Expression::SymbolSet Expression::getSymbolUses() const
{
	// Create a set to store the symbols
	SymbolSet symbols;
	
	// Get the sub-expressions of this expression
	ExprVector subExprs = getSubExprs();
	
	// For each sub-expression
	for (ExprVector::const_iterator subItr = subExprs.begin(); subItr != subExprs.end(); ++subItr)
	{
		// Get a pointer to this expression
		Expression* pSubExpr = *subItr;
		
		// If this sub-expression is null, skip it
		if (pSubExpr == NULL)
			continue;
		
		// Get the symbol uses for the sub-expression
		SymbolSet subSymbols = pSubExpr->getSymbolUses();
		
		// Add the symbols to the set
		symbols.insert(subSymbols.begin(), subSymbols.end());
	}	
	
	// Return the set of symbols
	return symbols;
}
