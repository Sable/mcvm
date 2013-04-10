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

// Include guards
#ifndef SYMBOLEXPR_H_
#define SYMBOLEXPR_H_

// Header files
#include <string>
#include <ext/hash_map>
#include "expressions.h"
#include "utility.h"

/***************************************************************
* Class   : SymbolExpr
* Purpose : Represent a symbol to be evaluated
* Initial : Maxime Chevalier-Boisvert on November 2, 2008
****************************************************************
Revisions and bug fixes:
*/
class SymbolExpr : public Expression
{
public:
	
	// Method to get a symbol object from a name string
	static SymbolExpr* getSymbol(const std::string name);
	
	// Method to recursively copy this node
	SymbolExpr* copy() const;
	
	// Method to obtain a string representation of this node
	std::string toString() const;
	
	// Method to get all symbols read/used by this expression
	SymbolSet getSymbolUses() const { SymbolSet set; set.insert(this->copy()); return set; }
	
	// Accessor to get the symbol name
	const std::string& getSymName() const { return m_symName; }
	
private:
	
	// Private constructor
	SymbolExpr(const std::string& name);
	
	// Symbol name map type definition
	typedef __gnu_cxx::hash_map<std::string, SymbolExpr*, StrHashFunc, __gnu_cxx::equal_to<std::string>, gc_allocator<SymbolExpr*> > NameMap;
	
	// Name string for this symbol
	std::string m_symName;
	
	// Static symbol name map
	static NameMap s_nameMap;
};

#endif // #ifndef SYMBOLEXPR_H_
