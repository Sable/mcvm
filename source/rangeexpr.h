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
#ifndef RANGEEXPR_H_
#define RANGEEXPR_H_

// Header files
#include "expressions.h"

/***************************************************************
* Class   : RangeExpr
* Purpose : Represent a range expression
* Initial : Maxime Chevalier-Boisvert on November 8, 2008
****************************************************************
Revisions and bug fixes:
*/
class RangeExpr : public Expression
{
public:
	
	// Constructor
	RangeExpr(Expression* start, Expression* end, Expression* step)
	: m_pStartExpr(start), m_pEndExpr(end), m_pStepExpr(step) { m_exprType = RANGE; }
	
	// Method to recursively copy this node
	RangeExpr* copy() const;
	
	// Method to access sub-expressions
	ExprVector getSubExprs() const;
	
	// Method to replace a sub-expression
	void replaceSubExpr(size_t index, Expression* pNewExpr);
	
	// Method to obtain a string representation of this node
	std::string toString() const;

	// Method to tell if this is a full range
	bool isFullRange() const { return (m_pStartExpr == NULL); }
	
	// Accessors to get the start value, end value and step size
	Expression* getStartExpr() const { return m_pStartExpr; }
	Expression* getEndExpr() const { return m_pEndExpr; }
	Expression* getStepExpr() const { return m_pStepExpr; }
	
private:
	
	// Start value, step size and end value of the range
	Expression* m_pStartExpr;
	Expression* m_pEndExpr;
	Expression* m_pStepExpr;
};

#endif // #ifndef RANGEEXPR_H_
