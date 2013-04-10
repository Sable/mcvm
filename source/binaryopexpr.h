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
#ifndef BINARYOPEXPR_H_
#define BINARYOPEXPR_H_

// Header files
#include "expressions.h"

/***************************************************************
* Class   : BinaryOpExpr
* Purpose : Represent a binary operator expression
* Initial : Maxime Chevalier-Boisvert on November 6, 2008
****************************************************************
Revisions and bug fixes:
*/
class BinaryOpExpr : public Expression
{
public:

	// Enumerate valid binary operators
	enum Operator
	{
		PLUS,
		MINUS,
		MULT,
		ARRAY_MULT,
		DIV,
		ARRAY_DIV,
		LEFT_DIV,
		ARRAY_LEFT_DIV,
		POWER,
		ARRAY_POWER,
		EQUAL,
		NOT_EQUAL,
		LESS_THAN,
		LESS_THAN_EQ,
		GREATER_THAN,
		GREATER_THAN_EQ,
		OR,
		ARRAY_OR,
		AND,
		ARRAY_AND
	};
	
	// Constructor
	BinaryOpExpr(Operator op, Expression* lExpr, Expression* rExpr)
	: m_operator(op), m_pLeftExpr(lExpr), m_pRightExpr(rExpr) { m_exprType = BINARY_OP; }

	// Method to recursively copy this node
	BinaryOpExpr* copy() const;
	
	// Method to access sub-expressions
	ExprVector getSubExprs() const { ExprVector list; list.push_back(m_pLeftExpr); list.push_back(m_pRightExpr); return list; }

	// Method to replace a sub-expression
	void replaceSubExpr(size_t index, Expression* pNewExpr);
	
	// Method to obtain a string representation of this node
	std::string toString() const;	
	
	// Accessor to get the operator type
	Operator getOperator() const { return m_operator; }
	
	// Accessors to get the left and right operands
	Expression* getLeftExpr() const { return m_pLeftExpr; }
	Expression* getRightExpr() const { return m_pRightExpr; }	
		
private:
	
	// Unary operator type
	Operator m_operator;
	
	// Left and right expressions (operands)
	Expression* m_pLeftExpr;
	Expression* m_pRightExpr;
};

#endif // #ifndef BINARYOPEXPR_H_
