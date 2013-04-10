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
#include "ifelsestmt.h"
#include "utility.h"

/***************************************************************
* Function: IfElseStmt::copy()
* Purpose : Copy this IIR node recursively
* Initial : Maxime Chevalier-Boisvert on November 6, 2008
****************************************************************
Revisions and bug fixes:
*/
IfElseStmt* IfElseStmt::copy() const
{
	// Create and return a copy of this node
	return new IfElseStmt(
		(Expression*)m_pCondition->copy(),
		(StmtSequence*)m_pIfBlock->copy(),
		(StmtSequence*)m_pElseBlock->copy()
	);
}	

/***************************************************************
* Function: IfElseStmt::toString()
* Purpose : Generate a text representation of this IIR node
* Initial : Maxime Chevalier-Boisvert on November 6, 2008
****************************************************************
Revisions and bug fixes:
*/
std::string IfElseStmt::toString() const
{
	// Declare a string to the output
	std::string output;
	
	// Add the "if" keyword and the condition to the string
	output += "if " + m_pCondition->toString() + "\n";
	
	// Indent and add the if block text
	output += indentText(m_pIfBlock->toString());
	
	// If there is an else block
	if (m_pElseBlock)
	{
		// Add the "else" keyword to the string
		output += "else\n";
	
		// Indent and add the else block text
		output += indentText(m_pElseBlock->toString());
	}
	
	// Add the "end" keyword to the string
	output += "end";
	
	// Return the output
	return output;
}

/***************************************************************
* Function: IfElseStmt::getSymbolUses()
* Purpose : Get all symbols used/read in this statement
* Initial : Maxime Chevalier-Boisvert on April 2, 2009
****************************************************************
Revisions and bug fixes:
*/
Expression::SymbolSet IfElseStmt::getSymbolUses() const
{
	// Create a set to store all the uses
	Expression::SymbolSet useSet;
	
	// Get the symbols used by the conditio expression and the if and else blocks
	Expression::SymbolSet condUses = m_pCondition->getSymbolUses();
	Expression::SymbolSet ifUses = m_pIfBlock->getSymbolUses();
	Expression::SymbolSet elseUses = m_pElseBlock->getSymbolUses();
	
	// Insert the uses into the main set
	useSet.insert(condUses.begin(), condUses.end());
	useSet.insert(ifUses.begin(), ifUses.end());
	useSet.insert(elseUses.begin(), elseUses.end());
	
	// Return the use set
	return useSet;
}

/***************************************************************
* Function: IfElseStmt::getSymbolDefs()
* Purpose : Get all symbols written/defined in this statement
* Initial : Maxime Chevalier-Boisvert on April 2, 2009
****************************************************************
Revisions and bug fixes:
*/
Expression::SymbolSet IfElseStmt::getSymbolDefs() const
{
	// Create a set to store all the definitions
	Expression::SymbolSet defSet;
	
	// Get the symbols defined by the if and else blocks
	Expression::SymbolSet ifDefs = m_pIfBlock->getSymbolDefs();
	Expression::SymbolSet elseDefs = m_pElseBlock->getSymbolDefs();
	
	// Insert the if and else block definitions into the main set
	defSet.insert(ifDefs.begin(), ifDefs.end());
	defSet.insert(elseDefs.begin(), elseDefs.end());
	
	// Return the definition set
	return defSet;
}
