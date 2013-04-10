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

// Include guards
#ifndef ARRAYOBJ_H_
#define ARRAYOBJ_H_

// Header files
#include <vector>
#include "objects.h"

/***************************************************************
* Class   : ArrayObj
* Purpose : Represent an array of data objects
* Initial : Maxime Chevalier-Boisvert on November 16, 2008
****************************************************************
Revisions and bug fixes:
*/
class ArrayObj : public DataObject
{
public:

	// Object vector type definition
	typedef std::vector<DataObject*, gc_allocator<DataObject*> > ObjVector;
	
	// Default constructor
	ArrayObj(size_t reserve = 1) { m_type = ARRAY; m_objects.reserve(reserve); }
	
	// Single object constructor
	ArrayObj(DataObject* pObject) : m_objects(1, pObject) { m_type = ARRAY; }
	
	// Double object constructor
	ArrayObj(DataObject* pObj1, DataObject* pObj2)
	{ m_type = ARRAY; m_objects.push_back(pObj1); m_objects.push_back(pObj2); }
	
	// Static method to create an array object
	static ArrayObj* create(size_t reserve) { return new ArrayObj(reserve); } 
	
	// Method to recursively copy this node
	ArrayObj* copy() const;
	
	// Method to obtain a string representation of this node
	std::string toString() const;
	
	// Method to add an object to the array
	static void addObject(ArrayObj* pArray, DataObject* pObject);
	
	// Method to append the contents of another array object
	static void append(ArrayObj* pArray, const ArrayObj* pOther);
	
	// Method to get an object from the array
	DataObject* getObject(size_t index) const;
	
	// Static method to get an object from an array
	static DataObject* getArrayObj(const ArrayObj* pArray, size_t index) { return pArray->getObject(index); }
	
	// Accessor to get the number of objects contained
	size_t getSize() const { return m_objects.size(); }
	
	// Static accessor to get the number of objects contained
	static size_t getArraySize(const ArrayObj* pArray) { return pArray->getSize(); }
	
private:
	
	// Internal vector of object references
	ObjVector m_objects;
};

#endif // #ifndef ARRAYOBJ_H_
