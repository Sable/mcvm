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
#include "stmtsequence.h"

/***************************************************************
* Function: StmtSequence::copy()
* Purpose : Copy this IIR node recursively
* Initial : Maxime Chevalier-Boisvert on October 29, 2008
****************************************************************
Revisions and bug fixes:
*/
StmtSequence* StmtSequence::copy() const
{
	// Create a vector to copy to store statement copies
	StmtVector copyVector;
	
	// Copy each internal statement
	for (StmtVector::const_iterator itr = m_statements.begin(); itr != m_statements.end(); ++itr)
		copyVector.push_back((Statement*)(*itr)->copy());	
	
	// Create and return a new sequence statement object
	return new StmtSequence(copyVector);	
}	

/***************************************************************
* Function: StmtSequence::toString()
* Purpose : Generate a text representation of this IIR node
* Initial : Maxime Chevalier-Boisvert on October 29, 2008
****************************************************************
Revisions and bug fixes:
*/
std::string StmtSequence::toString() const
{
	// Declare a string for the output
	std::string output;
	
	// For each contained statement
	for (StmtVector::const_iterator itr = m_statements.begin(); itr != m_statements.end(); ++itr)
	{
		// Get a pointer to the statement
		Statement* pStmt = *itr;
		
		// Add the string representation of the statement to the string
		output += pStmt->toString();
		
		// If this is an expression statement or an assignment statement
		if (pStmt->getStmtType() == Statement::EXPR || 
			pStmt->getStmtType() == Statement::ASSIGN ||
			pStmt->getStmtType() == Statement::RETURN)
		{
			// If the output suppression flag is enabled, add a semicolon
			if (pStmt->getSuppressFlag())
				output += ";";
		}
		
		// Add a line terminator to the string
		output += "\n";
	}
	
	// Return the output
	return output;
}

/***************************************************************
* Function: StmtSequence::getSymbolUses()
* Purpose : Get all symbols used/read in this sequence
* Initial : Maxime Chevalier-Boisvert on April 2, 2009
****************************************************************
Revisions and bug fixes:
*/
Expression::SymbolSet StmtSequence::getSymbolUses() const
{
	// Declare a set for the symbols used
	Expression::SymbolSet useSet;
	
	// For each contained statement
	for (StmtVector::const_iterator itr = m_statements.begin(); itr != m_statements.end(); ++itr)
	{
		// Get a pointer to the statement
		Statement* pStmt = *itr;

		// Get the uses of this statement
		Expression::SymbolSet uses = pStmt->getSymbolUses();
		
		// Add the definitions to the main set
		useSet.insert(uses.begin(), uses.end());
	}	
	
	// Return the set of used symbols
	return useSet;
}

/***************************************************************
* Function: StmtSequence::getSymbolDefs()
* Purpose : Get all symbols written/defined in this sequence
* Initial : Maxime Chevalier-Boisvert on April 2, 2009
****************************************************************
Revisions and bug fixes:
*/
Expression::SymbolSet StmtSequence::getSymbolDefs() const
{
	// Declare a set for the symbols defined
	Expression::SymbolSet defSet;
	
	// For each contained statement
	for (StmtVector::const_iterator itr = m_statements.begin(); itr != m_statements.end(); ++itr)
	{
		// Get a pointer to the statement
		Statement* pStmt = *itr;

		// Get the definitions of this statement
		Expression::SymbolSet defs = pStmt->getSymbolDefs();
		
		// Add the definitions to the main set
		defSet.insert(defs.begin(), defs.end());
	}	
	
	// Return the set of defined symbols
	return defSet;
}
