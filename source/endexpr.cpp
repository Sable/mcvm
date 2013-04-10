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
#include "endexpr.h"

/***************************************************************
* Function: EndExpr::copy()
* Purpose : Copy this IIR node recursively
* Initial : Maxime Chevalier-Boisvert on March 2, 2009
****************************************************************
Revisions and bug fixes:
*/
EndExpr* EndExpr::copy() const
{
	// Create a vector for the association copies
	AssocVector newAssocs;
	
	// Copy the association vector
	for (AssocVector::const_iterator itr = m_assocs.begin(); itr != m_assocs.end(); ++itr)
		newAssocs.push_back(Assoc(itr->pSymbol->copy(), itr->dimIndex, itr->lastDim));
	
	// Create and return a new end expression object
	return new EndExpr(newAssocs);	
}

/***************************************************************
* Function: EndExpr::getSymbolUses()
* Purpose : Get all symbols read/used by this expression
* Initial : Maxime Chevalier-Boisvert on April 2, 2009
****************************************************************
Revisions and bug fixes:
*/
Expression::SymbolSet EndExpr::getSymbolUses() const
{
	// Create a set to store the uses
	SymbolSet useSet;
	
	// Add all association symbols to the use set
	for (AssocVector::const_iterator itr = m_assocs.begin(); itr != m_assocs.end(); ++itr)
		useSet.insert(itr->pSymbol);

	// Return the set of uses
	return useSet;
}
