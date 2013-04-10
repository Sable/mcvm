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
#ifndef FNHANDLEEXPR_H_
#define FNHANDLEEXPR_H_

// Header files
#include "expressions.h"
#include "symbolexpr.h"

/***************************************************************
* Class   : FnHandleExpr
* Purpose : Represent a function handle expression
* Initial : Maxime Chevalier-Boisvert on February 25, 2009
****************************************************************
Revisions and bug fixes:
*/
class FnHandleExpr : public Expression
{
public:
	
	// Constructor
	FnHandleExpr(SymbolExpr* symExpr) : m_pSymExpr(symExpr) { m_exprType = FN_HANDLE; }
	
	// Method to recursively copy this node
	FnHandleExpr* copy() const { return new FnHandleExpr(m_pSymExpr->copy()); }
	
	// Method to obtain a string representation of this node
	std::string toString() const { return "@" + m_pSymExpr->toString(); }
	
	// Method to access sub-expressions
	ExprVector getSubExprs() const { return ExprVector(1, m_pSymExpr); }
	
	// Method to replace a sub-expression
	void replaceSubExpr(size_t index, Expression* pNewExpr);
	
	// Accessor to get the symbol expression
	SymbolExpr* getSymbolExpr() const { return m_pSymExpr; }
	
private:

	// Symbol expression
	SymbolExpr* m_pSymExpr;	
};

#endif // #ifndef FNHANDLEEXPR_H_ 
