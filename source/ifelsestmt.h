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
#ifndef IFELSESTMT_H_
#define IFELSESTMT_H_

// Header files
#include "statements.h"
#include "expressions.h"
#include "stmtsequence.h"

/***************************************************************
* Class   : IfElseStmt
* Purpose : Represent an if-else control statement
* Initial : Maxime Chevalier-Boisvert on October 29, 2008
****************************************************************
Revisions and bug fixes:
*/
class IfElseStmt : public Statement
{
public:
	
	// Constructor
	IfElseStmt(Expression* cond, StmtSequence* ifBlock, StmtSequence* elseBlock)
	: m_pCondition(cond), m_pIfBlock(ifBlock), m_pElseBlock(elseBlock) { m_stmtType = IF_ELSE; }
	
	// Method to recursively copy this node
	IfElseStmt* copy() const;
	
	// Method to obtain a string representation of this node
	std::string toString() const;	

	// Method to get all symbols used/read in this statement
	Expression::SymbolSet getSymbolUses() const;
	
	// Method to get all symbols written/defined in this statement
	Expression::SymbolSet getSymbolDefs() const;
	
	// Accessor to get the condition expression
	Expression* getCondition() const { return m_pCondition; }
	
	// Accessors to get the if and else blocks
	StmtSequence* getIfBlock() const { return m_pIfBlock; }
	StmtSequence* getElseBlock() const { return m_pElseBlock; }	
	
private:

	// Condition expression
	Expression* m_pCondition;
	
	// If and else blocks
	StmtSequence* m_pIfBlock;
	StmtSequence* m_pElseBlock;	
};

#endif // #ifndef IFELSESTMT_H_
