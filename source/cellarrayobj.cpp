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
#include "cellarrayobj.h"

/***************************************************************
* Function: MatrixObj<DataObject*>::allocMatrix()
* Purpose : Matrix allocation method for cell arrays 
* Initial : Maxime Chevalier-Boisvert on February 18, 2009
****************************************************************
Revisions and bug fixes:
*/
template <> void MatrixObj<DataObject*>::allocMatrix()
{
	// Compute the number of matrix elements
	m_numElements = m_size[0];
	for (size_t i = 1; i < m_size.size(); ++i)
		m_numElements *= m_size[i];
	
	// Allocate memory for the matrix elements
	// Note that the memory is garbage-collected
	// This allocation is non-atomic to support cell arrays
	m_pElements = (DataObject**)GC_MALLOC_IGNORE_OFF_PAGE(m_numElements * sizeof(DataObject*));
}

/***************************************************************
* Function: MatrixObj<DataObject*>::toString()
* Purpose : Obtain a string representation of a cell array
* Initial : Maxime Chevalier-Boisvert on February 19, 2009
****************************************************************
Revisions and bug fixes:
*/
template <> std::string MatrixObj<DataObject*>::toString() const
{
	// Create a string to store the output
	std::string output;
	
	// Log that we will print the matrix size
	output += "cell array of size ";
	
	// For each dimension of the matrix
	for (size_t i = 0; i < m_size.size(); ++i)
	{
		// Add this dimension to the output
		output += ::toString(m_size[i]);
		
		// If this is not the last dimension, add a separator
		if (i < m_size.size() - 1)
			output += "x";
	}
	
	// Add a newline to the output
	output += "\n";
	
	// If the matrix has 0 elements
	if (m_numElements == 0)
	{
		// Add the empty cell array symbol to the output
		output += "{}\n";
		
		// Return early
		return output;
	}
	
	// Create and initialize a vector for the indices
	DimVector indices(m_size.size(), 1);
	
	// Initialize the current dimension index
	size_t curDim = 2;
	
	// For each 2D slice of the matrix
	for (;;)
	{	
		// If there is more than one slice
		if (indices.size() > 2)
		{
			// Begin the slice index line
			output += "\nmatrix(:,:";
			
			// Output the slice indices
			for (size_t i = 2; i < indices.size(); ++i)
				output += "," + ::toString(indices[i]);
			
			// End the slice index line
			output += ")\n";
		}
		
		// For each row
		for (indices[0] = 1; indices[0] <= m_size[0]; ++indices[0])
		{
			// For each column
			for (indices[1] = 1; indices[1] <= m_size[1]; ++indices[1])
			{
				// Add the type name of the element to the output
				output += "\t[" + getElemND(indices)->getTypeName() + "]";
			}
			
			// Add a newline to the output
			output += "\n";				
		}
		
		// If there are no slice indices to increase, stop
		if (curDim >= indices.size())
			break;
		
		// Until we are done moving to the next slice
		while (curDim < indices.size())
		{
			// Move to the next slice along this dimension
			indices[curDim]++;
			
			// If the index along this dimension is still valid
			if (indices[curDim] <= m_size[curDim])
			{
				// Go back to the first slicing dimension
				curDim = 2;
				
				// Stop the process
				break;
			}
			else
			{
				// Move to the next dimension
				curDim++;
				
				// Reset the indices for prior dimensions
				for (size_t i = 2; i < curDim; ++i)
					indices[i] = 1;
			}				
		}
			
		// If we went through all slices, stop
		if (curDim == indices.size())
			break;			
	}		
	
	// Return the output string
	return output;
}
