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
#include "objects.h"
#include "cellarrayobj.h"
#include "runtimebase.h"

/***************************************************************
* Function: DataObject::convert()
* Purpose : Convert the object to the requested type
* Initial : Maxime Chevalier-Boisvert on February 18, 2009
****************************************************************
Revisions and bug fixes:
*/
DataObject* DataObject::convert(DataObject::Type outType) const
{
	// If the requested type is the local type, return a copy
	if (outType == m_type)
		return copy();
	
	// By default, throw an exception and deny the type conversion
	throw RunError(
		"unsupported type conversion requested: " +
		getTypeName(m_type) + " => " + getTypeName(outType)
	);
}

/***************************************************************
* Function: DataObject::getTypeName()
* Purpose : Get the name of an object type
* Initial : Maxime Chevalier-Boisvert on February 19, 2009
****************************************************************
Revisions and bug fixes:
*/
std::string DataObject::getTypeName(Type type)
{
	// Switch on the object type
	switch (type)
	{
		// Return the name matching the type
		case UNKNOWN		: return "unknown";
		case MATRIX_I32		: return "i32 matrix";
		case MATRIX_F32		: return "f32 matrix";
		case MATRIX_F64		: return "f64 matrix";
		case MATRIX_C128	: return "c128 matrix";
		case LOGICALARRAY	: return "logical array";
		case CHARARRAY		: return "char array";
		case CELLARRAY		: return "cell array";
		case STRUCT_INST	: return "struct inst";
		case CLASS_INST		: return "class inst";
		case FUNCTION		: return "function";
		case RANGE			: return "range";
		case ARRAY			: return "array";
		case FN_HANDLE		: return "func handle";
	}
	
	// If the type is unmatched, break an assertions
	assert (false);
}

// Static version of the copy method
DataObject* DataObject::copyObject(const DataObject* pObject)
{ 
	PROF_INCR_COUNTER(Profiler::ARRAY_COPY_COUNT); 
	return pObject->copy();
}
	
