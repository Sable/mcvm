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
#ifndef EXPRSTMT_H_
#define EXPRSTMT_H_

// Header files
#include "statements.h"
#include "expressions.h"

/***************************************************************
* Class   : ExprStmt
* Purpose : Represent an expression statement
* Initial : Maxime Chevalier-Boisvert on November 7, 2008
****************************************************************
Revisions and bug fixes:
*/
class ExprStmt : public Statement
{
public:
	
	// Constructor
	ExprStmt(Expression* expr, bool suppressOut = true)
	: m_pExpr(expr)	{ m_stmtType = EXPR; m_suppressOut = suppressOut; }
	
	// Method to recursively copy this node
	ExprStmt* copy() const;
	
	// Method to obtain a string representation of this node
	std::string toString() const;	
	
	// Method to get all symbols read/used by this expression
	Expression::SymbolSet getSymbolUses() const { return m_pExpr->getSymbolUses(); }
	
	// Accessor to get the expression
	Expression* getExpression() const { return m_pExpr; }
	
private:

	// Condition expression
	Expression* m_pExpr;
};

#endif // #ifndef EXPRSTMT_H_ 
