/***************************************************************
*
* The programming code contained in this file was written
* by Maxime Chevalier-Boisvert. For information, please
* contact the author.
*
* Contact information:
* e-mail: mcheva@cs.mcgill.ca
* phone : 1-514-935-2809
*
***************************************************************/

// Include guards
#ifndef SWITCHSTMT_H_
#define SWITCHSTMT_H_

// Header files
#include "statements.h"
#include "expressions.h"
#include "stmtsequence.h"

/***************************************************************
* Class   : SwitchStmt
* Purpose : Represent a switch control statement
* Initial : Maxime Chevalier-Boisvert on February 23, 2009
****************************************************************
Revisions and bug fixes:
*/
class SwitchStmt : public Statement
{
public:
	
	// Switch case type definition
	typedef std::pair<Expression*, StmtSequence*> SwitchCase;
	
	// Case list type definition
	typedef std::vector<SwitchCase, gc_allocator<SwitchCase> > CaseList;
	
	// Constructor
	SwitchStmt(Expression* pSwitchExpr, const CaseList& caseList, StmtSequence* pDefaultCase) 
	: m_pSwitchExpr(pSwitchExpr), m_caseList(caseList), m_pDefaultCase(pDefaultCase)
	{ m_stmtType = SWITCH; }
	
	// Method to recursively copy this node
	SwitchStmt* copy() const;
	
	// Method to obtain a string representation of this node
	std::string toString() const;
	
	// Method to get the switching expression
	Expression* getSwitchExpr() const { return m_pSwitchExpr; }
	
	// Method to get the switch case list
	const CaseList& getCaseList() const { return m_caseList; }
	
	// Method to get the default switch case
	StmtSequence* getDefaultCase() const { return m_pDefaultCase; }
	
private:

	// Switching expression
	Expression* m_pSwitchExpr;
	
	// Switch case list
	CaseList m_caseList;
	
	// Default case statement sequence
	StmtSequence* m_pDefaultCase;	
};

#endif // #ifndef SWITCHSTMT_H_ 
