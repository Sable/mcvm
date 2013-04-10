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

// Include guards
#ifndef ENDEXPR_H_
#define ENDEXPR_H_

// Header files
#include "expressions.h"
#include "symbolexpr.h"

/***************************************************************
* Class   : EndExpr
* Purpose : Represent a range end expression
* Initial : Maxime Chevalier-Boisvert on March 1, 2009
****************************************************************
Revisions and bug fixes:
*/
class EndExpr : public Expression
{
public:
	
	// Matrix association class
	class Assoc
	{
	public:
		
		// Constructor
		Assoc(SymbolExpr* pSym, size_t dim, bool ldim)
		: pSymbol(pSym), dimIndex(dim), lastDim(ldim) {}
		
		// Associated symbol
		SymbolExpr* pSymbol;
		
		// Dimension index
		size_t dimIndex;
		
		// Last dimension flag
		bool lastDim;		
	};
	
	// Association vector type definition
	typedef std::vector<Assoc, gc_allocator<Assoc> > AssocVector;
	
	// Constructor
	EndExpr(const AssocVector& assocs = AssocVector())
	: m_assocs(assocs) { m_exprType = END; }
	
	// Method to recursively copy this node
	EndExpr* copy() const;
	
	// Method to obtain a string representation of this node
	std::string toString() const { return "end"; }
	
	// Method to get all symbols read/used by this expression
	SymbolSet getSymbolUses() const;
	
	// Mutator to set the association vector
	void setAssocs(const AssocVector& assocs) { m_assocs = assocs; }
	
	// Accessor to get the association vector
	const AssocVector& getAssocs() const { return m_assocs; }
	
private:
	
	// Vector of matrix associations
	AssocVector m_assocs;	
};

#endif // #ifndef ENDEXPR_H_ 
