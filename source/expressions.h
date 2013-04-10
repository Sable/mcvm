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
#ifndef EXPRESSIONS_H_
#define EXPRESSIONS_H_

// Header files
#include <cassert>
#include <vector>
#include <set>
#include "iir.h"

// Symbol expression forward declaration
class SymbolExpr;

/***************************************************************
* Class   : Expressions
* Purpose : Base class for expressions
* Initial : Maxime Chevalier-Boisvert on October 27, 2008
****************************************************************
Revisions and bug fixes:
*/
class Expression : public IIRNode
{
public:
	
	// Enumerate expression types
	enum ExprType
	{
		PARAM,
		CELL_INDEX,
		BINARY_OP,
		UNARY_OP,
		SYMBOL,
		INT_CONST,
		FP_CONST,
		STR_CONST,
		RANGE,
		END,
		MATRIX,
		CELLARRAY,
		FN_HANDLE,
		LAMBDA
	};
	
	// Expression vector type definition
	typedef std::vector<Expression*, gc_allocator<Expression*> > ExprVector;
	
	// Symbol set type definition
	typedef std::set<SymbolExpr*> SymbolSet;
	
	// Constructor and destructor
	Expression() {}
	virtual ~Expression() {}
	
	// Method to recursively copy this node
	virtual Expression* copy() const = 0;
		
	// Method to access sub-expressions
	virtual ExprVector getSubExprs() const { return ExprVector(); }
	
	// Method to replace a sub-expression
	virtual void replaceSubExpr(size_t index, Expression* pNewExpr) { assert (false); }
	
	// Method to get all symbols read/used by this expression
	virtual SymbolSet getSymbolUses() const;
	
	// Accessor to get the expression type
	ExprType getExprType() const { return m_exprType; }
	
protected:
	
	// Expression type
	ExprType m_exprType;
};

#endif // #ifndef EXPRESSIONS_H_ 
