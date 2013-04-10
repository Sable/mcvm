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

// Header files
#include <iostream>
#include "loopstmts.h"
#include "utility.h"

/***************************************************************
* Function: ForStmt::copy()
* Purpose : Copy this IIR node recursively
* Initial : Maxime Chevalier-Boisvert on November 8, 2008
****************************************************************
Revisions and bug fixes:
*/
ForStmt* ForStmt::copy() const
{
  // Create and return a copy of this node
  return new ForStmt((AssignStmt*)m_pAssignStmt->copy(),
                     (StmtSequence*)m_pLoopBody->copy(),m_annotations);
}	

/***************************************************************
* Function: ForStmt::toString()
* Purpose : Generate a text representation of this IIR node
* Initial : Maxime Chevalier-Boisvert on November 8, 2008
****************************************************************
Revisions and bug fixes:
*/
std::string ForStmt::toString() const
{
	// Declare a string to the output
	std::string output;
	
	// Add the "for" keyword and the assignment statement to the string
	output += "for " + m_pAssignStmt->toString() + "\n";
	
	// Indent and add the loop body text
	output += indentText(m_pLoopBody->toString());
	
	// Add the "end" keyword to the string
	output += "end";
	
	// Return the output
	return output;
}

/***************************************************************
* Function: WhileStmt::copy()
* Purpose : Copy this IIR node recursively
* Initial : Maxime Chevalier-Boisvert on November 8, 2008
****************************************************************
Revisions and bug fixes:
*/
WhileStmt* WhileStmt::copy() const
{
	// Create and return a copy of this node
  return new WhileStmt((Expression*)m_pCondExpr->copy(),
		(StmtSequence*)m_pLoopBody->copy(), m_annotations);
}	

/***************************************************************
* Function: WhileStmt::toString()
* Purpose : Generate a text representation of this IIR node
* Initial : Maxime Chevalier-Boisvert on November 8, 2008
****************************************************************
Revisions and bug fixes:
*/
std::string WhileStmt::toString() const
{
	// Declare a string to the output
	std::string output;
	
	// Add the "while" keyword and the condition expression to the string
	output += "while " + m_pCondExpr->toString() + "\n";
	
	// Indent and add the loop body text
	output += indentText(m_pLoopBody->toString());
	
	// Add the "end" keyword to the string
	output += "end";
	
	// Return the output
	return output;
}

/***************************************************************
* Function: LoopStmt::copy()
* Purpose : Copy this IIR node recursively
* Initial : Maxime Chevalier-Boisvert on November 12, 2008
****************************************************************
Revisions and bug fixes:
*/
LoopStmt* LoopStmt::copy() const
{
	// Create and return a copy of this node
  return new LoopStmt(
		m_pIndexVar,
		m_pTestVar,	
		m_pInitSeq->copy(),	
		m_pTestSeq->copy(),	
		m_pBodySeq->copy(),
		m_pIncrSeq->copy(),
        m_annotations
	);
}	

/***************************************************************
* Function: LoopStmt::toString()
* Purpose : Generate a text representation of this IIR node
* Initial : Maxime Chevalier-Boisvert on November 12, 2008
****************************************************************
Revisions and bug fixes:
*/
std::string LoopStmt::toString() const
{
	// Declare a string to the output
	std::string output;
	
	// Add the init sequence to the output
	output += m_pInitSeq->toString();
	
	// Add the "while" keyword and an always true condition to the output
	output += "while True\n";
	
	// Add the test sequence to the string
	output += indentText(m_pTestSeq->toString());
	
	// Indent and add the loop test condition to the string
	output += indentText("if ~" + m_pTestVar->toString() + "\n" + indentText("break;\n") + "end\n");  
	
	// Indent and add the loop body text
	output += indentText(m_pBodySeq->toString());
	
	// Indent and add the incrementation sequence to the string
	output += indentText(m_pIncrSeq->toString());
	
	// Add the "end" keyword to the string
	output += "end";	
	
	// Return the output
	return output;
}

/***************************************************************
* Function: LoopStmt::getSymbolUses()
* Purpose : Get all symbols used/read in this statement
* Initial : Maxime Chevalier-Boisvert on April 2, 2009
****************************************************************
Revisions and bug fixes:
*/
Expression::SymbolSet LoopStmt::getSymbolUses() const
{
	// Create a set to store all the uses
	Expression::SymbolSet useSet;
	
	// Get the symbols used by loop statement sequences
	Expression::SymbolSet initUses = m_pInitSeq->getSymbolUses();
	Expression::SymbolSet testUses = m_pTestSeq->getSymbolUses();
	Expression::SymbolSet bodyUses = m_pBodySeq->getSymbolUses();
	Expression::SymbolSet incrUses = m_pIncrSeq->getSymbolUses();
	
	// Insert the sets into the main set
	useSet.insert(initUses.begin(), initUses.end());
	useSet.insert(testUses.begin(), testUses.end());
	useSet.insert(bodyUses.begin(), bodyUses.end());
	useSet.insert(incrUses.begin(), incrUses.end());	
	
	// Add the test variable to the use set
	useSet.insert(m_pTestVar);
	
	// Return the use set
	return useSet;
}

/***************************************************************
* Function: LoopStmt::getSymbolDefs()
* Purpose : Get all symbols written/defined in this statement
* Initial : Maxime Chevalier-Boisvert on April 2, 2009
****************************************************************
Revisions and bug fixes:
*/
Expression::SymbolSet LoopStmt::getSymbolDefs() const
{
	// Create a set to store all the definitions
	Expression::SymbolSet defSet;
	
	// Get the symbols defined by loop statement sequences
	Expression::SymbolSet initDefs = m_pInitSeq->getSymbolDefs();
	Expression::SymbolSet testDefs = m_pTestSeq->getSymbolDefs();
	Expression::SymbolSet bodyDefs = m_pBodySeq->getSymbolDefs();
	Expression::SymbolSet incrDefs = m_pIncrSeq->getSymbolDefs();
	
	// Insert the sets into the main set
	defSet.insert(initDefs.begin(), initDefs.end());
	defSet.insert(testDefs.begin(), testDefs.end());
	defSet.insert(bodyDefs.begin(), bodyDefs.end());
	defSet.insert(incrDefs.begin(), incrDefs.end());	
	
	// Return the definition set
	return defSet;
}
