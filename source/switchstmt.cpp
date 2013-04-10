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

// Header files
#include "switchstmt.h"
#include "utility.h" 

/***************************************************************
* Class   : SwitchStmt::copy()
* Purpose : Recursively copy this node
* Initial : Maxime Chevalier-Boisvert on February 23, 2009
****************************************************************
Revisions and bug fixes:
*/
SwitchStmt* SwitchStmt::copy() const
{
	// Create a list to store the switch case copies
	CaseList newCases;
	
	// For each switch case
	for (CaseList::const_iterator itr = m_caseList.begin(); itr != m_caseList.end(); ++itr)
	{
		// Get a reference to this switch case
		const SwitchCase& switchCase = *itr;
		
		// Copy this switch case
		SwitchCase newCase = SwitchCase(switchCase.first->copy(), switchCase.second->copy());
		
		// Add the new case to the list
		newCases.push_back(newCase);
	}	
	
	// Create and return a copy of the switch statement
	return new SwitchStmt
	(
		m_pSwitchExpr->copy(),
		newCases,
		m_pDefaultCase->copy()
	);
}
 
/***************************************************************
* Class   : SwitchStmt::toString()
* Purpose : Obtain a string representation of this node
* Initial : Maxime Chevalier-Boisvert on February 23, 2009
****************************************************************
Revisions and bug fixes:
*/
std::string SwitchStmt::toString() const
{
	// Create a string object to store the output
	std::string output;
	
	// Add the switch keyaowrd and the switch expression to the output
	output += "switch " + m_pSwitchExpr->toString() + "\n";
	
	// For each switch case
	for (CaseList::const_iterator itr = m_caseList.begin(); itr != m_caseList.end(); ++itr)
	{
		// Get a reference to this switch case
		const SwitchCase& switchCase = *itr;
		
		// Declare a string for the case text
		std::string caseText;
		
		// Add the case keyword and the case expression
		caseText += "case " + switchCase.first->toString() + "\n";
		
		// Add the statement sequence to the text
		caseText += indentText(switchCase.second->toString());
		
		// Indent the case text and add it to the output
		output += indentText(caseText);
	}
	
	// Declare a string for the default case text
	std::string defaultText;

	// Add the otherwise keyword
	defaultText += "otherwise\n";

	// Add the statement sequence to the text
	defaultText += indentText(m_pDefaultCase->toString());

	// Indent the default case text and add it to the output
	output += indentText(defaultText);
	
	// Terminate the output with the end keyword
	output += "end";
	
	// Return the output string
	return output;
}
