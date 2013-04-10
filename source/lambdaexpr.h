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
#ifndef LAMBDAEXPR_H_
#define LAMBDAEXPR_H_

// Header files
#include "expressions.h"
#include "symbolexpr.h"

/***************************************************************
* Class   : LambdaExpr
* Purpose : Represent an anynymous function expression
* Initial : Maxime Chevalier-Boisvert on February 25, 2009
****************************************************************
Revisions and bug fixes:
*/
class LambdaExpr : public Expression
{
public:
	
	// Parameter vector type definition
	typedef std::vector<SymbolExpr*, gc_allocator<SymbolExpr*> > ParamVector;
	
	// Constructor
	LambdaExpr(const ParamVector& inParams, Expression* pBodyExpr)
	: m_inParams(inParams), m_pBodyExpr(pBodyExpr)
	{ m_exprType = LAMBDA; }
	
	// Method to recursively copy this node
	LambdaExpr* copy() const;
	
	// Method to access sub-expressions
	ExprVector getSubExprs() const { return ExprVector(1, m_pBodyExpr); }
	
	// Method to replace a sub-expression
	void replaceSubExpr(size_t index, Expression* pNewExpr);
	
	// Method to obtain a string representation of this node
	std::string toString() const;
	
	// Method to get all symbols read/used by this expression
	SymbolSet getSymbolUses() const;
	
	// Accessor to get the input parameters
	const ParamVector& getInParams() const { return m_inParams; }
	
	// Accessor to get the body expression
	Expression* getBodyExpr() const { return m_pBodyExpr; }
	
private:

	// Input parameters
	ParamVector m_inParams;
	
	// Body expression
	Expression* m_pBodyExpr;
};

#endif // #ifndef LAMBDAEXPR_H_ 
