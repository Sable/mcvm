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
#include <cassert>
#include "rangeobj.h"
#include "matrixobjs.h"
#include "utility.h"

// Full range constant
const RangeObj RangeObj::FULL_RANGE(FLOAT_INFINITY, FLOAT_INFINITY, FLOAT_INFINITY);

// Range counting epsilon constant
const double RangeObj::COUNT_EPSILON = 1e-5;

/***************************************************************
* Function: RangeObj::toString()
* Purpose : Produce a string representation of this object
* Initial : Maxime Chevalier-Boisvert on February 16, 2008
****************************************************************
Revisions and bug fixes:
*/
std::string RangeObj::toString() const
{
	// Concatenate and return the string representation
	return ::toString(m_startVal) + ":" + ::toString(m_stepVal) + ":" + ::toString(m_endVal);
}

/***************************************************************
* Function: RangeObj::operator == ()
* Purpose : Equality comparison operator for range objects
* Initial : Maxime Chevalier-Boisvert on February 16, 2008
****************************************************************
Revisions and bug fixes:
*/
bool RangeObj::operator == (const RangeObj& other) const
{
	// Compare the start, step and end values
	return (
		(m_startVal == other.m_startVal) &&
		(m_stepVal == other.m_stepVal) &&
		(m_endVal == other.m_endVal)
	);
}

/***************************************************************
* Function: RangeObj::getElemCount()
* Purpose : Compute the number of elements in the range
* Initial : Maxime Chevalier-Boisvert on February 16, 2008
****************************************************************
Revisions and bug fixes:
*/
size_t RangeObj::getElemCount() const
{
	// Ensure that this is not a full range
	assert (!isFullRange());
	
	// Declare a variable for the element count
	size_t elemCount;

	// If the step value is 0
	if (m_stepVal == 0)
	{
		// The range has 0 elements
		elemCount = 0;
	}
	else
	{
		// Compute the range length
		double rangeLen = (m_endVal - m_startVal) / m_stepVal;
	
		// Cast the range length into a positive integer
		size_t rangeLenInt = size_t(rangeLen);

		// If the range length is negative
		if (rangeLen < 0)
		{
			// The range has 0 elements
			elemCount = 0;
		}
		else
		{
			// Compute the element count based on the range length
			elemCount = rangeLenInt + 1;

			// Estimate the position of the next number after the range end
			double estFinalP1 = m_startVal + (elemCount) * m_stepVal;
			
			// If this number is within epsilon of the range end, add it to the range
			if (std::abs(estFinalP1 - m_endVal) < COUNT_EPSILON)
				elemCount += 1;
		}
	}
	
	// Return the element count
	return elemCount;
}

/***************************************************************
* Function: RangeObj::expand()
* Purpose : Expand this range into a vector
* Initial : Maxime Chevalier-Boisvert on February 16, 2008
****************************************************************
Revisions and bug fixes:
*/
DataObject* RangeObj::expand() const
{
	// Ensure that this is not a full range
	assert (!isFullRange());
	
	// Comput the element count
	size_t elemCount = getElemCount();
	
	// Create a matrix to store the expanded range
	MatrixF64Obj* pMatrix = new MatrixF64Obj(1, elemCount);
	
	// Initialize the current value to the start value
	double value = m_startVal;
	
	// For each range element
	for (size_t i = 1; i <= elemCount; ++i)
	{
		// Set the current element
		pMatrix->setElem1D(i, value);
		
		// Update the current value
		value += m_stepVal;
	}
	
	// Return the expanded range
	return pMatrix;
}
