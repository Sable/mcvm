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
#ifndef UNARYOPEXPR_H_
#define UNARYOPEXPR_H_

// Header files
#include "expressions.h"

/***************************************************************
* Class   : UnaryOpExpr
* Purpose : Represent an unary operator expression
* Initial : Maxime Chevalier-Boisvert on November 6, 2008
****************************************************************
Revisions and bug fixes:
*/
class UnaryOpExpr : public Expression
{
public:

	// Enumerate valid operators
	enum Operator
	{
		PLUS,
		MINUS,
		NOT,
		TRANSP,
		ARRAY_TRANSP,
	};
	
	// Constructor
	UnaryOpExpr(Operator op, Expression* pExpr)
	: m_operator(op), m_pOperand(pExpr) { m_exprType = UNARY_OP; }
	
	// Method to recursively copy this node
	UnaryOpExpr* copy() const;
	
	// Method to access sub-expressions
	ExprVector getSubExprs() const { return ExprVector(1, m_pOperand); }
	
	// Method to replace a sub-expression
	void replaceSubExpr(size_t index, Expression* pNewExpr);
	
	// Method to obtain a string representation of this node
	std::string toString() const;	
		
	// Accessor to get the operator type
	Operator getOperator() const { return m_operator; }
	
	// Accessor to get the operand
	Expression* getOperand() const { return m_pOperand; }
	
private:
	
	// Unary operator type
	Operator m_operator;
	
	// Operand expression
	Expression* m_pOperand;
};

#endif // #ifndef UNARYOPEXPR_H_ 
