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
#ifndef LOOPSTMT_H_
#define LOOPSTMT_H_

// Header files
#include "statements.h"
#include "assignstmt.h"
#include "stmtsequence.h"
#include "symbolexpr.h"

/***************************************************************
* Class   : ForStmt
* Purpose : Represent a for loop control statement
* Initial : Maxime Chevalier-Boisvert on November 8, 2008
****************************************************************
Revisions and bug fixes:
*/
class ForStmt : public Statement
{
public:

 ForStmt(AssignStmt* assignStmt, StmtSequence* loopBody, unsigned annotations)
  : m_pAssignStmt(assignStmt), m_pLoopBody(loopBody) 
  { m_stmtType = FOR; m_annotations = annotations; }
	
	// Method to recursively copy this node
	ForStmt* copy() const;
	
	// Method to obtain a string representation of this node
	std::string toString() const;	
	
	// Accessor to get the assignment statement
	AssignStmt* getAssignStmt() const { return m_pAssignStmt; }
	
	// Accessor to get the loop body
	StmtSequence* getLoopBody() const { return m_pLoopBody; }

    bool isOutermost() const  { return m_annotations & OUTERMOST; }
    bool isInnermost() const  { return m_annotations & INNERMOST; }
    
private:

	// Condition expression
	AssignStmt* m_pAssignStmt;
	
	// Loop body statements
	StmtSequence* m_pLoopBody;
};

/***************************************************************
* Class   : WhileStmt
* Purpose : Represent a while loop control statement
* Initial : Maxime Chevalier-Boisvert on November 8, 2008
****************************************************************
Revisions and bug fixes:
*/
class WhileStmt : public Statement
{
public:
	
	// Constructor
  WhileStmt(Expression* condExpr, StmtSequence* loopBody, unsigned annotations)
      : m_pCondExpr(condExpr), m_pLoopBody(loopBody) 
  { m_stmtType = WHILE; m_annotations = annotations; }

	// Method to recursively copy this node
	WhileStmt* copy() const;
	
	// Method to obtain a string representation of this node
	std::string toString() const;	
	
	// Accessor to get the condition expression
	Expression* getCondExpr() const { return m_pCondExpr; }
	
	// Accessor to get the loop body
	StmtSequence* getLoopBody() const { return m_pLoopBody; }

    bool isOutermost() const  { return m_annotations & OUTERMOST; }
    bool isInnermost() const  { return m_annotations & INNERMOST; }
	
private:

	// Condition expression
	Expression* m_pCondExpr;
	
	// Loop body statements
	StmtSequence* m_pLoopBody;	
};

/***************************************************************
* Class   : LoopStmt
* Purpose : Generic/simplified loop control statement
* Initial : Maxime Chevalier-Boisvert on January 7, 2009
****************************************************************
Revisions and bug fixes:
*/
class LoopStmt : public Statement
{
public:	

    // Constructor
	LoopStmt(
		SymbolExpr* indVar,
		SymbolExpr* testVar,
		StmtSequence* initSeq,
		StmtSequence* testSeq,
		StmtSequence* bodySeq,
		StmtSequence* incrSeq,
        unsigned annotations
	)
	: m_pIndexVar(indVar),
	  m_pTestVar(testVar),
	  m_pInitSeq(initSeq),
	  m_pTestSeq(testSeq),
	  m_pBodySeq(bodySeq),
      m_pIncrSeq(incrSeq)
      { m_stmtType = LOOP; m_annotations = annotations;}
	// Method to recursively copy this node
	LoopStmt* copy() const;
	
	// Method to obtain a string representation of this node
	std::string toString() const;
	
	// Method to get all symbols used/read in this statement
	Expression::SymbolSet getSymbolUses() const;
	
	// Method to get all symbols written/defined in this statement
	Expression::SymbolSet getSymbolDefs() const;
	
	// Accessor to get the index variable
	SymbolExpr* getIndexVar() const { return m_pIndexVar; }

	// Accessor to get the test variable
	SymbolExpr* getTestVar() const { return m_pTestVar; }
	
	// Accessor to get the initialization sequence
	StmtSequence* getInitSeq() const { return m_pInitSeq; }
	
	// Accessor to get the condition sequence
	StmtSequence* getTestSeq() const { return m_pTestSeq; }
	
	// Accessor to get the loop body
	StmtSequence* getBodySeq() const { return m_pBodySeq; }
	
	// Accessor to get the incrementation sequence
	StmtSequence* getIncrSeq() const { return m_pIncrSeq; }	

    bool isOutermost() const  { return m_annotations & OUTERMOST; }
    bool isInnermost() const  { return m_annotations & INNERMOST; }

	
private:
	
	// Loop index variable (may be null)
	SymbolExpr* m_pIndexVar;
	
	// Loop test variable
	SymbolExpr* m_pTestVar;	
	
	// Index initialization statements
	StmtSequence* m_pInitSeq;	
	
	// Test condition statements
	StmtSequence* m_pTestSeq;	
	
	// Loop body statements
	StmtSequence* m_pBodySeq;
	
	// Index incrementation statements
	StmtSequence* m_pIncrSeq;
};

/***************************************************************
* Class   : BreakStmt
* Purpose : Represent a break statement
* Initial : Maxime Chevalier-Boisvert on February 26, 2009
****************************************************************
Revisions and bug fixes:
*/
class BreakStmt : public Statement
{
public:

	// Constructor
	BreakStmt() { m_stmtType = BREAK; }
	
	// Method to recursively copy this node
	BreakStmt* copy() const { return new BreakStmt(); }
	
	// Method to obtain a string representation of this node
	std::string toString() const { return "break"; }
};

/***************************************************************
* Class   : ContinueStmt
* Purpose : Represent a continue statement
* Initial : Maxime Chevalier-Boisvert on February 26, 2009
****************************************************************
Revisions and bug fixes:
*/
class ContinueStmt : public Statement
{
public:

	// Constructor
	ContinueStmt() { m_stmtType = CONTINUE; }
	
	// Method to recursively copy this node
	ContinueStmt* copy() const { return new ContinueStmt(); }
	
	// Method to obtain a string representation of this node
	std::string toString() const { return "continue"; }
};

#endif // #ifndef LOOPSTMT_H_
