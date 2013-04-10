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
#ifndef MATRIXEXPR_H_
#define MATRIXEXPR_H_

// Header files
#include "expressions.h"

/***************************************************************
* Class   : MatrixExpr
* Purpose : Represent a matrix expression
* Initial : Maxime Chevalier-Boisvert on November 10, 2008
****************************************************************
Revisions and bug fixes:
*/
class MatrixExpr : public Expression
{
public:
	
	// Matrix row type definition
	typedef std::vector<Expression*, gc_allocator<Expression*> > Row;
	
	// Row list type definition
	typedef std::vector<Row, gc_allocator<Row> > RowVector;
	
	// Constructor
	MatrixExpr(const RowVector& rows) : m_rows(rows) { m_exprType = MATRIX; }
	
	// Method to recursively copy this node
	MatrixExpr* copy() const;
	
	// Method to access sub-expressions
	ExprVector getSubExprs() const;
	
	// Method to replace a sub-expression
	void replaceSubExpr(size_t index, Expression* pNewExpr);
	
	// Method to obtain a string representation of this node
	std::string toString() const;
	
	// Accessor to get the row list
	const RowVector& getRows() const { return m_rows; }
	
private:
	
	// Vector of rows
	RowVector m_rows;
};

#endif // #ifndef MATRIXEXPR_H_
