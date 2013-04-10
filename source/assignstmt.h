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
#ifndef ASSIGNSTMT_H_
#define ASSIGNSTMT_H_

// Header files
#include "statements.h"
#include "expressions.h"

/***************************************************************
* Class   : AssignStmt
* Purpose : Represent an assignment statement
* Initial : Maxime Chevalier-Boisvert on November 2, 2008
****************************************************************
Revisions and bug fixes:
*/
class AssignStmt : public Statement
{
public:
	
	// Expression vector type definition
	typedef std::vector<Expression*, gc_allocator<Expression*> > ExprVector;
	
	// Full constructor
	AssignStmt(const ExprVector& leftExprs, Expression* rightExpr, bool suppressOut = true)
	: m_leftExprs(leftExprs), m_pRightExpr(rightExpr)
	{ m_stmtType = ASSIGN; m_suppressOut = suppressOut; }
	
	// Single left-expr constructor
	AssignStmt(Expression* pLeftExpr, Expression* rightExpr, bool suppressOut = true)
	: m_leftExprs(1, pLeftExpr), m_pRightExpr(rightExpr)
	{ m_stmtType = ASSIGN; m_suppressOut = suppressOut; }
	
	// Method to recursively copy this node
	AssignStmt* copy() const;
	
	// Method to obtain a string representation of this node
	std::string toString() const;
	
	// Method to get all symbols read/used by this expression
	Expression::SymbolSet getSymbolUses() const; 
	
	// Method to get all symbols written/defined by this expression
	Expression::SymbolSet getSymbolDefs() const;
	
	// Accessor to get the vector of left expressions
	ExprVector getLeftExprs() const { return m_leftExprs; }
	
	// Accessors to get the right expression
	Expression* getRightExpr() const { return m_pRightExpr; }
	
private:
	
	// Left expression (assigned to)
	ExprVector m_leftExprs;
	
	// Right expression (assigned from)
	Expression* m_pRightExpr;
};

#endif // #ifndef ASSIGNSTMT_H_
