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
//   limitations under the License.                                            //
//                                                                             //
// =========================================================================== //

// Header files
#include <cassert>
#include "arrayobj.h"

/***************************************************************
* Function: ArrayObj::copy()
* Purpose : Recursively copy this object
* Initial : Maxime Chevalier-Boisvert on November 16, 2008
****************************************************************
Revisions and bug fixes:
*/
ArrayObj* ArrayObj::copy() const
{
	// Create a new array object
	ArrayObj* arrayCopy = new ArrayObj();
	
	// For each stored object
	for (ObjVector::const_iterator itr = m_objects.begin(); itr != m_objects.end(); ++itr)
		addObject(arrayCopy, (*itr)->copy());
	
	// Return the copy object
	return arrayCopy;
}

/***************************************************************
* Function: ArrayObj::toString()
* Purpose : Obtain a string representation of this object
* Initial : Maxime Chevalier-Boisvert on November 16, 2008
****************************************************************
Revisions and bug fixes:
*/
std::string ArrayObj::toString() const
{
	// Declare a string for the output
	std::string output;
	
	// Add the opening bracket to the output
	output += "(";
	
	// For each stored object
	for (ObjVector::const_iterator itr = m_objects.begin(); itr != m_objects.end(); ++itr)
	{
		// Add the string representation of this object
		output += (*itr)->toString();
		
		// If this isn't the last object, add a comma separator
		if (itr != --m_objects.end())
			output += ", ";		
	}
	
	// Add the closing bracket to the output
	output += ")";
	
	// Return the output string
	return output;
}

/***************************************************************
* Function: ArrayObj::addObject()
* Purpose : Add an object to this array
* Initial : Maxime Chevalier-Boisvert on November 16, 2008
****************************************************************
Revisions and bug fixes:
*/
void ArrayObj::addObject(ArrayObj* pArray, DataObject* pObject)
{
	// Add the object to the internal vector
	pArray->m_objects.push_back(pObject);
}

/***************************************************************
* Function: ArrayObj::append()
* Purpose : Append the contents of another array object
* Initial : Maxime Chevalier-Boisvert on March 15, 2008
****************************************************************
Revisions and bug fixes:
*/
void ArrayObj::append(ArrayObj* pArray, const ArrayObj* pOther)
{
	// Append the contents of the other vector to this one
	pArray->m_objects.insert(
		pArray->m_objects.end(), 
		pOther->m_objects.begin(),
		pOther->m_objects.end()
	);
}

/***************************************************************
* Function: ArrayObj::getObject()
* Purpose : Get an object from the array
* Initial : Maxime Chevalier-Boisvert on November 16, 2008
****************************************************************
Revisions and bug fixes:
*/
DataObject* ArrayObj::getObject(size_t index) const
{
	// Ensure the index is valid
	assert (index < m_objects.size());
	
	// Return the object reference
	return m_objects[index];
}
