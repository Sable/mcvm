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
#ifndef __OBJECTS_H__
#define __OBJECTS_H__

// Header files
#include <string>
#include <gc_cpp.h>
#include <gc/gc_allocator.h>
#include <iostream>


/***************************************************************
* Class   : DataObject
* Purpose : Base class for data objects
* Initial : Maxime Chevalier-Boisvert on October 16, 2008
* Notes   : All data objects are garbage-collected
****************************************************************
Revisions and bug fixes:
*/
class DataObject : public gc
{
	// Declare the JIT compiler as a friend class
	// This is so the JIT can read the type variable directly
	friend class JITCompiler;
	
public:
	
	// Enumerate data object types
	enum Type
	{
		UNKNOWN,
		MATRIX_I32,
		MATRIX_F32,
		MATRIX_F64,
		MATRIX_C128,
		LOGICALARRAY,
		CHARARRAY,
		CELLARRAY,
		STRUCT_INST,
		CLASS_INST,
		FUNCTION,
		RANGE,
		ARRAY,
		FN_HANDLE
	};
	
	// Constructor and destructor
	DataObject() {}
	virtual ~DataObject() {}
	
	// Method to recursively copy this object
	virtual DataObject* copy() const = 0;
	
	// Method to obtain a string representation of this node
	virtual std::string toString() const = 0;

	// Method to convert the object to the requested type
	virtual DataObject* convert(DataObject::Type outType) const;
	
	// Static version of the conversion method
	static DataObject* convertType(const DataObject* pObject, DataObject::Type outType) { return pObject->convert(outType); }
	
	// Static version of the copy method
	static DataObject* copyObject(const DataObject* pObject);
	
	// Static method to get the name of an object type
	static std::string getTypeName(Type type);
	
	// Method to get the name of the local type
	std::string getTypeName() const { return getTypeName(m_type); }
	
	// Method to test whether or not this object is a matrix object
	bool isMatrixObj() const { return m_type >= MATRIX_I32 && m_type <= CELLARRAY; }
	
	// Accessor to get the type of this data object
	Type getType() const { return m_type; }
	
protected:
	
	// Type of this data object
	Type m_type;
};

#endif // #ifndef __OBJECTS_H__
