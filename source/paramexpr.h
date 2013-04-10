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
#ifndef PARAMEXPR_H_
#define PARAMEXPR_H_

// Header files
#include <string>
#include <vector>
#include "objects.h"
#include "expressions.h"
#include "symbolexpr.h"

/***************************************************************
* Class   : ParamExpr
* Purpose : Represent a parameterized expression
* Initial : Maxime Chevalier-Boisvert on November 2, 2008
****************************************************************
Revisions and bug fixes:
*/
class ParamExpr : public Expression
{
public:
	
	// Constructor
	ParamExpr(SymbolExpr* symExpr, const ExprVector& arguments)
	: m_pSymbolExpr(symExpr), m_arguments(arguments)
	{ m_exprType = PARAM; }
	
	// Method to recursively copy this node
	ParamExpr* copy() const;
	
	// Method to access sub-expressions
	ExprVector getSubExprs() const;
	
	// Method to replace a sub-expression
	void replaceSubExpr(size_t index, Expression* pNewExpr);
	
	// Method to obtain a string representation of this node
	virtual std::string toString() const;
	
	// Accessor to get the symbol expression
	SymbolExpr* getSymExpr() const { return m_pSymbolExpr; }
	
	// Accessor to get the arguments
	const ExprVector& getArguments() const { return m_arguments; }
	
protected:
	
	// Symbol expression
	SymbolExpr* m_pSymbolExpr;
	
	// Function arguments
	ExprVector m_arguments;
};

#endif // #ifndef PARAMEXPR_H_
