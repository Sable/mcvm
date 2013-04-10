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
#include <cassert>
#include <iostream>
#include "environment.h"
#include "profiling.h"

/***************************************************************
* Function: Environment::Environment()
* Purpose : Public constructor for environment class
* Initial : Maxime Chevalier-Boisvert on November 12, 2008
****************************************************************
Revisions and bug fixes:
*/
Environment::Environment()
: m_pParent(NULL)
{
}

/***************************************************************
* Function: Environment::copy()
* Purpose : Copy this environment object
* Initial : Maxime Chevalier-Boisvert on February 2, 2009
****************************************************************
Revisions and bug fixes:
*/
Environment* Environment::copy() const
{
	// Copy this environment object
	Environment* pNewEnv = new Environment(m_pParent);
	
	// Copy the bindings
	pNewEnv->m_bindings = m_bindings;
	
	// Return the new environment object
	return pNewEnv;
}

/***************************************************************
* Function: static Environment::bind()
* Purpose : Create a binding in this environment
* Initial : Maxime Chevalier-Boisvert on November 12, 2008
****************************************************************
Revisions and bug fixes:
*/
void Environment::bind(Environment* pEnv, const SymbolExpr* pSymbol, DataObject* pObject)
{
	// Get a non-constant pointer for the symbol
	SymbolExpr* pSym = const_cast<SymbolExpr*>(pSymbol);
	
	// Store the binding
	pEnv->m_bindings[pSym] = pObject;
}

/***************************************************************
* Function: static Environment::unbind()
* Purpose : Remove a binding in this environment
* Initial : Maxime Chevalier-Boisvert on February 22, 2008
****************************************************************
Revisions and bug fixes:
*/
bool Environment::unbind(Environment* pEnv, const SymbolExpr* pSymbol)
{
	// Get a non-constant pointer for the symbol
	SymbolExpr* pSym = const_cast<SymbolExpr*>(pSymbol);
	
	// Attempt to find the binding in this environment
	SymbolMap::const_iterator bindItr = pEnv->m_bindings.find(pSym);
	
	// If the binding was found
	if (bindItr != pEnv->m_bindings.end())
	{
		// Remove the binding
		pEnv->m_bindings.erase(pSym);
		
		// Operation successful
		return true;
	}
	
	// Binding not found
	return false;
}

/***************************************************************
* Function: static Environment::lookup()
* Purpose : Lookup a symbol in this environment
* Initial : Maxime Chevalier-Boisvert on November 12, 2008
****************************************************************
Revisions and bug fixes:
*/
DataObject* Environment::lookup(const Environment* pEnv, const SymbolExpr* pSymbol)
{
	// Get a non-constant pointer for the symbol
	SymbolExpr* pSym = const_cast<SymbolExpr*>(pSymbol);
	
	// Attempt to find the binding in this environment
	SymbolMap::const_iterator bindItr = pEnv->m_bindings.find(pSym);
	
	// If the binding was found
	if (bindItr != pEnv->m_bindings.end())
	{
		// Increment the environment lookup count
		PROF_INCR_COUNTER(Profiler::ENV_LOOKUP_COUNT);
		
		// Return the object bound to this symbol
		return bindItr->second;
	}
	
	// If there is a parent environment
	if (pEnv->m_pParent != NULL)
	{
		// lookup the binding in the parent environment
		return lookup(pEnv->m_pParent, pSymbol);
	}
		
	// Symbol binding not found
	return NULL;
}

/***************************************************************
* Function: static Environment::extend()
* Purpose : Extend an environment object
* Initial : Maxime Chevalier-Boisvert on November 12, 2008
****************************************************************
Revisions and bug fixes:
*/
Environment* Environment::extend(Environment* pParent)
{
	// Return a new environment with this one as parent
	return new Environment(pParent);
}

/***************************************************************
* Function: Environment::getSymbols()
* Purpose : Get the symbols bound in this environment
* Initial : Maxime Chevalier-Boisvert on February 22, 2009
****************************************************************
Revisions and bug fixes:
*/
Environment::SymbolVec Environment::getSymbols() const
{
	// Create a vector to store the symbols
	SymbolVec symbols;
	
	// For each binding
	for (SymbolMap::const_iterator itr = m_bindings.begin(); itr != m_bindings.end(); ++itr)
	{
		// Add the symbol to the vector
		symbols.push_back(itr->first);
	}
	
	// Return the vector of symbols
	return symbols;
}

/***************************************************************
* Function: Environment::Environment()
* Purpose : Private constructor for environment class
* Initial : Maxime Chevalier-Boisvert on November 12, 2008
****************************************************************
Revisions and bug fixes:
*/
Environment::Environment(Environment* pParent)
: m_pParent(pParent)
{
}
