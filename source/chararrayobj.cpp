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
#include <cassert>
#include "chararrayobj.h"

/***************************************************************
* Function: MatrixObj<char, DataObject::CHARARRAY>::toString()
* Purpose : Obtain a string representation of a character array 
* Initial : Maxime Chevalier-Boisvert on February 15, 2009
****************************************************************
Revisions and bug fixes:
*/
template <> std::string MatrixObj<char>::toString() const
{
	// Ensure that the character array has 2 dimesions
	assert (m_size.size() == 2);
	
	// Create a string to store the output
	std::string outStr;
	
	// For each row
	for (size_t rowIndex = 0; rowIndex < m_size[0]; ++rowIndex)
	{
		// For each column
		for (size_t colIndex = 0; colIndex < m_size[1]; ++colIndex)
		{
			// Output this character
			outStr += m_pElements[colIndex * m_size[0] + rowIndex];
		}
		
		// Add a newline after this row
		outStr += '\n';
	}		
	
	// Return the output string
	return outStr;
}
