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
#include "symbolexpr.h"

// Static symbol name map
SymbolExpr::NameMap SymbolExpr::s_nameMap;

/***************************************************************
* Function: static SymbolExpr::getSymbol()
* Purpose : Get a symbol object from a name string
* Initial : Maxime Chevalier-Boisvert on March 15, 2008
****************************************************************
Revisions and bug fixes:
*/
SymbolExpr* SymbolExpr::getSymbol(const std::string name)
{
	// Attempt to find the symbol name in the name map
	NameMap::iterator symItr = s_nameMap.find(name);
	
	// If a symbol was found for this name
	if (symItr != s_nameMap.end())
	{
		// Return a pointer to the symbol
		return symItr->second;
	}
	else
	{
		// Create a new symbol object for this name
		SymbolExpr* pNewSym = new SymbolExpr(name);
		
		// Add the new symbol to the map
		s_nameMap[name] = pNewSym;
		
		// Return a pointer to the new symbol
		return pNewSym;
	}	
}

/***************************************************************
* Function: SymbolExpr::copy()
* Purpose : Copy this IIR node recursively
* Initial : Maxime Chevalier-Boisvert on November 7, 2008
****************************************************************
Revisions and bug fixes:
*/
SymbolExpr* SymbolExpr::copy() const
{
	// Symbols are unique for a given identifier
	// Do not copy this object
	return const_cast<SymbolExpr*>(this);
}

/***************************************************************
* Function: SymbolExpr::toString()
* Purpose : Generate a text representation of this IIR node
* Initial : Maxime Chevalier-Boisvert on November 7, 2008
****************************************************************
Revisions and bug fixes:
*/
std::string SymbolExpr::toString() const
{
	// Return the symbol name
	return m_symName;
}

/***************************************************************
* Function: SymbolExpr::SymbolExpr()
* Purpose : Constructor for symbol expression class
* Initial : Maxime Chevalier-Boisvert on November 12, 2008
****************************************************************
Revisions and bug fixes:
*/
SymbolExpr::SymbolExpr(const std::string& name)
{
	// Set the expression type
	m_exprType = SYMBOL;
	
	// Store the symbol name
	m_symName = name;
}
