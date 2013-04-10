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
#include <vector>
#include "cellarrayexpr.h"

/***************************************************************
* Function: CellArrayExpr::copy()
* Purpose : Copy this IIR node recursively
* Initial : Maxime Chevalier-Boisvert on February 23, 2009
****************************************************************
Revisions and bug fixes:
*/
CellArrayExpr* CellArrayExpr::copy() const
{
	// Create a vector to store the row copies
	RowVector rowCopies;
	
	// Copy each row
	for (RowVector::const_iterator rowItr = m_rows.begin(); rowItr != m_rows.end(); ++rowItr)
	{
		// Get a reference to this row
		const Row& row = *rowItr;
		
		// Declare a vector to store the element copies
		Row rowCopy;
		
		// Copy each element of the row
		for (Row::const_iterator elemItr = row.begin(); elemItr != row.end(); ++elemItr)
			rowCopy.push_back((Expression*)(*elemItr)->copy());
		
		// Add the row copy to the vector
		rowCopies.push_back(rowCopy);
	}
	
	// Create and return a copy of this node
	return new CellArrayExpr(rowCopies);
}	

/***************************************************************
* Function: CellArrayExpr::toString()
* Purpose : Generate a text representation of this IIR node
* Initial : Maxime Chevalier-Boisvert on February 23, 2009
****************************************************************
Revisions and bug fixes:
*/
std::string CellArrayExpr::toString() const
{
	// Declare a string to the output
	std::string output;
	
	// Add the opening brace to the string
	output += "{";
	
	// For each row
	for (RowVector::const_iterator rowItr = m_rows.begin(); rowItr != m_rows.end(); ++rowItr)
	{
		// Get a reference to this row
		const Row& row = *rowItr;
		
		// For each element of the row
		for (Row::const_iterator elemItr = row.begin(); elemItr != row.end(); ++elemItr)
		{
			// Add the string representation of this element to the string
			output += (*elemItr)->toString();
			
			// If this isn't the last element, add a space
			if (elemItr != --row.end())
				output += " ";
		}
		
		// If this isn't the last row, add a row-separating semicolon
		if (rowItr != --m_rows.end())
			output += "; ";
	}
	
	// Add the closing brace to the string
	output += "}";
	
	// Return the output
	return output;
}

/***************************************************************
* Function: CellArrayExpr::getSubExprs()
* Purpose : Access sub-expressions
* Initial : Maxime Chevalier-Boisvert on February 23, 2009
****************************************************************
Revisions and bug fixes:
*/
Expression::ExprVector CellArrayExpr::getSubExprs() const
{
	// Create a list to store the sub-expression pointers
	ExprVector list;
	
	// For each row
	for (RowVector::const_iterator rowItr = m_rows.begin(); rowItr != m_rows.end(); ++rowItr)
	{
		// Get a reference to this row
		const Row& row = *rowItr;
		
		// For each element of the row
		for (size_t i = 0; i < row.size(); ++i)
		{			
			// Add the sub-expression pointer to the list
			list.push_back(row[i]);
		}
	}
	
	// Return the list
	return list;
}

/***************************************************************
* Function: CellArrayExpr::replaceSubExpr()
* Purpose : Replace a sub-expression
* Initial : Maxime Chevalier-Boisvert on March 17, 2008
****************************************************************
Revisions and bug fixes:
*/
void CellArrayExpr::replaceSubExpr(size_t index, Expression* pNewExpr)
{
	// Initialize the current index
	size_t curIndex = 0;
	
	// For each row
	for (RowVector::iterator rowItr = m_rows.begin(); rowItr != m_rows.end(); ++rowItr)
	{
		// Get a reference to this row
		Row& row = *rowItr;
		
		// For each element of the row
		for (size_t i = 0; i < row.size(); ++i)
		{			
			// If this is the original expression, replace it
			if (curIndex == index)
			{
				row[i] = pNewExpr;
				return;
			}
			
			// Increment the current index
			++curIndex;
		}
	}
	
	// Ensure the sub-expression was replaced
	assert (false);	
}
