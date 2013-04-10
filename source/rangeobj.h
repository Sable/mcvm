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

// Include guards
#ifndef RANGEOBJ_H_
#define RANGEOBJ_H_

// Header files
#include "objects.h"

/***************************************************************
* Class   : RangeObj
* Purpose : Represent a range of real numbers
* Initial : Maxime Chevalier-Boisvert on February 16, 2009
****************************************************************
Revisions and bug fixes:
*/
class RangeObj : public DataObject
{
public:
	
	// Full range constant
	static const RangeObj FULL_RANGE;
	
	// Range counting epsilon constant
	static const double COUNT_EPSILON;
	
	// Constructor
	RangeObj(double start, double step, double end)
	: m_startVal(start), m_stepVal(step), m_endVal(end) { m_type = RANGE; }
	
	// Method to recursively copy this node
	RangeObj* copy() const { return new RangeObj(m_startVal, m_stepVal, m_endVal); }
	
	// Method to obtain a string representation of this node
	std::string toString() const;
	
	// Equality comparison operator
	bool operator == (const RangeObj& other) const;
	
	// Method to test if this is a full range
	bool isFullRange() const { return (*this == FULL_RANGE); }

	// Method to compute the number of elements in the range
	size_t getElemCount() const;
	
	// Method to expand this range into a vector
	DataObject* expand() const;
	
	// Accessors to get the start, step and env values
	double getStartVal() const { return m_startVal; }
	double getStepVal() const { return m_stepVal; }
	double getEndVal() const { return m_endVal; }
	
private:
	
	// Start, step and end values
	double m_startVal;
	double m_stepVal;
	double m_endVal;	
};



#endif // #ifndef RANGEOBJ_H_ 
