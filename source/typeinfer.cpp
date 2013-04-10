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
#include "typeinfer.h"
#include "cellarrayobj.h"
#include "utility.h"

/***************************************************************
* Function: TypeInfo::TypeInfo() 
* Purpose : Default constructor for type info class
* Initial : Maxime Chevalier-Boisvert on April 14, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeInfo::TypeInfo()
: m_objType(DataObject::MATRIX_F64),
  m_is2D(false),
  m_isScalar(false),
  m_isInteger(false),
  m_sizeKnown(false),
  m_pFunction(NULL)
{
}
 
/***************************************************************
* Function: TypeInfo::TypeInfo(DataObject, ...) 
* Purpose : Build a type info object from a data object
* Initial : Maxime Chevalier-Boisvert on April 14, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeInfo::TypeInfo(const DataObject* pObject, bool storeMatDims, bool scanMatrices)
: m_is2D(false),
  m_isScalar(false),
  m_isInteger(false),
  m_sizeKnown(false),
  m_pFunction(NULL)
{
	// Store the object type
	m_objType = pObject->getType();
	
	// If this object is a matrix
	if (pObject->isMatrixObj())
	{
		// Get a typed pointer to the matrix
		BaseMatrixObj* pMatrixObj = (BaseMatrixObj*)pObject;
		
		// If the matrix is bidimensional
		if (pMatrixObj->is2D())
		{
			// Set the bidimensional flag
			m_is2D = true;
		}
		
		// If the matrix is a scalar
		if (pMatrixObj->isScalar())
		{
			// Set the flags accordingly
			m_isScalar = true;
			m_sizeKnown = true;
			
			// Set the matrix size to 1x1
			m_matSize.push_back(1);
			m_matSize.push_back(1);
			
			// If this is a 64-bit float matrix
			if (pMatrixObj->getType() == DataObject::MATRIX_F64)
			{
				// Get the scalar value
				float64 scalar = ((MatrixF64Obj*)pObject)->getScalar();
				
				// If the scalar value is an integer, set the integer flag
				if (::isInteger(scalar))
					m_isInteger = true;
			}
		}
			
		// Otherwise, if the matrix is not scalar 
		else 
		{
			// If we should store the matrix dimensions
			if (storeMatDims)
			{
				// Set the size know flag
				m_sizeKnown = true;
				
				// Store the matrix size vector
				const ::DimVector& matSize = pMatrixObj->getSize();
				m_matSize.insert(m_matSize.begin(), matSize.begin(), matSize.end());
				
				// If the matris is empty and not a cell array
				if (pMatrixObj->isEmpty() && pObject->getType() != DataObject::CELLARRAY)
				{
					// Set the integer flag
					m_isInteger = true;
				}
			}
			
			// If this is a 64-bit float matrix and its contents should be scanned
			if (pMatrixObj->getType() == DataObject::MATRIX_F64 && scanMatrices)
			{
				// Get a typed pointer to the matrix
				MatrixF64Obj* pF64Matrix = (MatrixF64Obj*)pMatrixObj;
				
				// Initially, set the integer flag to true
				m_isInteger = true;
				
				// For each value in the matrix
				const float64* pLastElem = pF64Matrix->getElements() + pF64Matrix->getNumElems();
				for (float64* pValue = pF64Matrix->getElements(); pValue < pLastElem; ++pValue)
				{
					// If this value is not integer
					if (::isInteger(*pValue) == false)
					{
						// Set the integer flag to false and break out of the loop
						m_isInteger = false;
						break;
					}
				}				
			}
		}
		
		// If this is a logical array or a character array object
		if (pObject->getType() == DataObject::LOGICALARRAY || pObject->getType() == DataObject::CHARARRAY)
		{
			// Set the integer flag
			m_isInteger = true;
		}
		
		// If this is a cell array object and its contents should be scanned
		else if (pObject->getType() == DataObject::CELLARRAY && scanMatrices)
		{
			// Get a typed pointer to the cell array object
			CellArrayObj* pCellObj = (CellArrayObj*)pObject;
			
			// For each cell array element
			for (size_t i = 1; i <= pCellObj->getNumElems(); ++i)
			{
				// Get a pointer to this element
				DataObject* pElem = pCellObj->getElem1D(i);
				
				// Get a type info object for this element
				TypeInfo elemType(pElem, storeMatDims, scanMatrices);
				
				// Insert the type info into the cell type set
				m_cellTypes.insert(elemType);
			}
		}
	}
	
	// Otherwise, if the object is a function handle
	else if (pObject->getType() == DataObject::FN_HANDLE)
	{
		// Get a typed pointer to the function handle
		FnHandleObj* pFnHandle = (FnHandleObj*)pObject;
		
		// Get the function the handle points to
		Function* pFunction = pFnHandle->getFunction();
		
		// If the function is a closure
		if (pFunction->isProgFunction() && ((ProgFunction*)pFunction)->isClosure())
		{
			// Do not store a pointer to it
			m_pFunction = NULL;
		}
		else
		{
			// Store a pointer to the function object
			m_pFunction = pFunction;
		}
	}
}

/***************************************************************
* Function: TypeInfo::TypeInfo(...) 
* Purpose : Full constructor for the type info class
* Initial : Maxime Chevalier-Boisvert on April 20, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeInfo::TypeInfo(
	DataObject::Type objType,
	bool is2D,
	bool isScalar,
	bool isInteger,
	bool sizeKnown,
	DimVector matSize,
	Function* pFunction,
	std::set<TypeInfo> cellTypes
)
: m_objType(objType),
  m_is2D(is2D),
  m_isScalar(isScalar),
  m_isInteger(isInteger),
  m_sizeKnown(sizeKnown),
  m_matSize(matSize),
  m_pFunction(pFunction),
  m_cellTypes(cellTypes)
{
}

/***************************************************************
* Function: TypeInfo::toString() 
* Purpose : Get a string representation of the type information
* Initial : Maxime Chevalier-Boisvert on April 15, 2009
****************************************************************
Revisions and bug fixes:
*/
std::string TypeInfo::toString() const
{
	// Declare a string for the output
	std::string output;
	
	// Add the type name to the output
	output += DataObject::getTypeName(m_objType);
	
	// If the object is a function handle
	if (m_objType == DataObject::FN_HANDLE)
	{
		// Add the function name to the output, if known
		output += " (" + (m_pFunction? m_pFunction->getFuncName():"unknown function") + ")";
	}
		
	// Otherwise
	else
	{
		// Create a vector for the info strings
		std::vector<std::string> infoStrs;
		
		// If this is a 2D matrix, add the 2D flag
		if (m_is2D)
			infoStrs.push_back("2D");
		
		// If this is a scalar, add the scalar flag
		if (m_isScalar)
			infoStrs.push_back("scalar");

		// If this is an integer, add the integer flag
		if (m_isInteger)
			infoStrs.push_back("integer");
			
		// If the size is known
		if (m_sizeKnown)
		{
			// Declare a string for the matrix size
			std::string sizeStr;
			
			// Write the matrix size to the string
			for (size_t i = 0; i < m_matSize.size(); ++i)
			{
				sizeStr += ::toString(m_matSize[i]);
				if (i != m_matSize.size() - 1) sizeStr += "x";
			}
			
			// Add the size string to the info strings
			infoStrs.push_back(sizeStr);
		}
		
		// If there are any info strings
		if (infoStrs.size() != 0)
		{
			// Add an opening parenthesis to the output
			output += " (";
			
			// Write the info strings to the output
			for (size_t i = 0; i < infoStrs.size(); ++i)
			{
				output += infoStrs[i];
				if (i != infoStrs.size() - 1) output += ", ";
			}
			
			// Add a closing parenthesis to the output
			output += ")";
		}
		
		// If the object i a cell array
		if (m_objType == DataObject::CELLARRAY)
		{
			// Declare a string for the cell type info
			std::string cellTypeOut;
			
			// For each cell array type
			for (std::set<TypeInfo>::iterator itr = m_cellTypes.begin(); itr != m_cellTypes.end(); ++itr)
			{
				// Add info about this type to the string
				cellTypeOut += "\n" + itr->toString();
			}
			
			// Indent the cell type text and add it to the output
			output += indentText(cellTypeOut);
		}
	}
	
	// Return the output string
	return output;
}

/***************************************************************
* Function: TypeInfo::operator == () 
* Purpose : Equality comparison operator
* Initial : Maxime Chevalier-Boisvert on April 15, 2009
****************************************************************
Revisions and bug fixes:
*/
bool TypeInfo::operator == (const TypeInfo& other) const
{
	// If the object type does not match, types are not equal
	if (m_objType != other.m_objType)
		return false;
	
	// If the object is a function handle
	if (m_objType == DataObject::FN_HANDLE)
	{
		// If the function pointers do not match, types are not equal
		if (m_pFunction != other.m_pFunction)
			return false;
	}
	else
	{
		// If the size known flag does not match, types do not match
		if (m_sizeKnown != other.m_sizeKnown)
			return false;
		
		// If the matrix size is known
		if (m_sizeKnown)
		{
			// If the sizes do not match, the types do not match
			if (m_matSize != other.m_matSize)
				return false;
		}	
	
		// If the object is a cell array
		if (m_objType == DataObject::CELLARRAY)
		{
			// If the cell types do not match, the types do not match
			if (m_cellTypes != other.m_cellTypes)
				return false;
		}
		
		// Otherwise
		else
		{
			// If the 2D flag does not match, types do not match 
			if (m_is2D != other.m_is2D)
				return false;
			
			// If the scalar flag does not match, types do not match 
			if (m_isScalar != other.m_isScalar)
				return false;
			
			// If the integer flag does not match, types do not match 
			if (m_isInteger != other.m_isInteger)
				return false;
		}
	}
	
	// The types are equal
	return true;
}

/***************************************************************
* Function: TypeInfo::operator < () 
* Purpose : Less-than comparison operator (for sorting)
* Initial : Maxime Chevalier-Boisvert on April 15, 2009
****************************************************************
Revisions and bug fixes:
*/
bool TypeInfo::operator < (const TypeInfo& other) const
{
	// Compare the object type
	if (m_objType < other.m_objType)
		return true;
	else if (m_objType > other.m_objType)
		return false;
	
	// If the object is a function handle
	if (m_objType == DataObject::FN_HANDLE)
	{
		// Compare the function pointer
		if (m_pFunction < other.m_pFunction)
			return true;
		else
			return false;
	}
	else
	{
		// Compare the size known flag
		if (m_sizeKnown < other.m_sizeKnown)
			return true;
		else if (m_sizeKnown > other.m_sizeKnown)
			return false;
		
		// If the matrix size is known
		if (m_sizeKnown)
		{
			// Compare the matrix sizes
			if (m_matSize < other.m_matSize)
				return true;
			else if (m_matSize > other.m_matSize)
				return false;
		}	
	
		// If the object is a cell array
		if (m_objType == DataObject::CELLARRAY)
		{
			// Compare the cell types
			if (m_cellTypes < other.m_cellTypes)
				return true;
			else
				return false;
		}
		
		// Otherwise
		else
		{
			// Compare the 2D, scalar and integer flags
			if (m_is2D < other.m_is2D)
				return true;
			else if (m_is2D > other.m_is2D)
				return false;
			else if (m_isScalar < other.m_isScalar)
				return true;
			else if (m_isScalar > other.m_isScalar)
				return false;
			else if (m_isInteger < other.m_isInteger)
				return true;
			else
				return false;
		}
	}	
}

/***************************************************************
* Function: validateTypes()
* Purpose : Validate a set of possible types for a data object
* Initial : Maxime Chevalier-Boisvert on May 5, 2009
****************************************************************
Revisions and bug fixes:
*/
bool validateTypes(const DataObject* pObject, const TypeSet& typeSet)
{
	// If the type set is empty, it is always valid
	if (typeSet.empty())
		return true;
	
	// If the object is null, the type set must be empty
	if (pObject == NULL)
		return false;

	// Create a type info object to represent the object's type
	TypeInfo objType(pObject, true, true);
	
	// For each possible type
	for (TypeSet::const_iterator typeItr = typeSet.begin(); typeItr != typeSet.end(); ++typeItr)
	{
		// Get a reference to this possible type
		const TypeInfo& thisType = *typeItr; 
		
		// If the object types do not match, the types do not match
		if (objType.getObjType() != thisType.getObjType())
			continue;
		
		// If this is a matrix type
		if (objType.getObjType() >= DataObject::MATRIX_I32 && objType.getObjType() <= DataObject::CELLARRAY)
		{
			// If the flags do not match, the types do not match
			if (thisType.is2D() == true && thisType.is2D() != objType.is2D()) continue;
			if (thisType.isScalar() == true && thisType.isScalar() != objType.isScalar()) continue;
			if (thisType.isInteger() == true && thisType.isInteger() != objType.isInteger()) continue;
			if (thisType.getSizeKnown() == true && thisType.getSizeKnown() != objType.getSizeKnown()) continue;
			
			// If the matrix sizes do not match, the types do not match
			if (thisType.getSizeKnown() == true && thisType.getMatSize() != objType.getMatSize())
				continue;

			// If this object is a cell array
			if (objType.getObjType() == DataObject::CELLARRAY)
			{
				// Get the set of possible cell types
				const TypeSet& cellTypes = thisType.getCellTypes();
				
				// Get a typed pointer to the cell array object
				CellArrayObj* pCellObj = (CellArrayObj*)pObject;
				
				// For each cell array element
				for (size_t i = 1; i <= pCellObj->getNumElems(); ++i)
				{
					// Get a pointer to this element
					DataObject* pElem = pCellObj->getElem1D(i);
					
					// If this object does not match the possible object types,
					// then the cell array types do not match
					if (validateTypes(pElem, cellTypes) == false)
					{
						std::cout << "cell type does not match durrr" << std::endl;
						continue;
					}
				}				
			}
		}
		
		// If this ia function handle type
		else if (objType.getObjType() == DataObject::FN_HANDLE)
		{
			// If the function pointers do not match, the types do not match
			if (thisType.getFunction() != NULL && thisType.getFunction() != objType.getFunction())
				continue;	
		}
		
		// The types match, the type set is valid
		return true;		
	}
	
	// No valid type found in the type set
	return false;
}
 
/***************************************************************
* Function: typeSetMake()
* Purpose : Build a type set from a single type object
* Initial : Maxime Chevalier-Boisvert on April 29, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeSet typeSetMake(const TypeInfo& type)
{
	// Create a type set and insert the type info object
	TypeSet set;
	set.insert(type);	
	
	// Return the type set
	return set;
}

/***************************************************************
* Function: typeSetUnion()
* Purpose : Obtain the union of two type sets
* Initial : Maxime Chevalier-Boisvert on April 17, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeSet typeSetUnion(const TypeSet& setA, const TypeSet& setB)
{
	// If either set is empty, return the empty set (unknown type)
	if (setA.empty() || setB.empty())
		return TypeSet();
	
	// Compute the naive union of both type sets
	TypeSet fullSet;
	fullSet.insert(setA.begin(), setA.end());
	fullSet.insert(setB.begin(), setB.end());
	
	// Reduce the possible types in the set
	return typeSetReduce(fullSet);
}

/***************************************************************
* Function: typeSetReduce()
* Purpose : Reduce the possible types in a type set
* Initial : Maxime Chevalier-Boisvert on April 20, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeSet typeSetReduce(const TypeSet& set)
{
	// If the set is empty, return the empty set
	if (set.empty())
		return set;
	
	// Current object type identifier
	DataObject::Type curType;
	
	// Current flag values
	bool is2D;
	bool isScalar;
	bool isInteger;
	bool sizeKnown;
	
	// Current matrix size info
	TypeInfo::DimVector matSize;
	
	// Current function pointer
	Function* pFunction;
			
	// Current cell array type set
	std::set<TypeInfo> cellTypes;
	
	// Create an iterator to the type set
	TypeSet::iterator itr = set.begin();
	
	// Initialize the current object type info
	curType = itr->getObjType();
	is2D = itr->is2D();
	isScalar = itr->isScalar();
	isInteger = itr->isInteger();
	sizeKnown = itr->getSizeKnown();
	matSize = itr->getMatSize();
	pFunction = itr->getFunction();
	cellTypes = itr->getCellTypes();

	// Create a type set to store the output
	TypeSet outSet;

	// For each type info object
	for (++itr; itr != set.end(); ++itr)
	{
		// If the object type of this object does not match that of the current objects
		if (itr->getObjType() != curType || itr->getObjType() == DataObject::FN_HANDLE)
		{
			// Add the current type info to the output set
			outSet.insert(TypeInfo(
				curType,
				is2D,
				isScalar,
				isInteger,
				sizeKnown,
				matSize,
				pFunction,
				cellTypes
			));
			
			// Reset the current object type info
			curType = itr->getObjType();
			is2D = itr->is2D();
			isScalar = itr->isScalar();
			isInteger = itr->isInteger();
			sizeKnown = itr->getSizeKnown();
			matSize = itr->getMatSize();
			pFunction = itr->getFunction();
			cellTypes = itr->getCellTypes();
		}
		else
		{
			// Update the current flags
			is2D = itr->is2D()? is2D:false;
			isScalar = itr->isScalar()? isScalar:false;
			isInteger = itr->isInteger()? isInteger:false;
			sizeKnown = itr->getSizeKnown()? sizeKnown:false;
			
			// Determine whether the matrix sizes match
			if (matSize != itr->getMatSize())
				sizeKnown = false;
			
			// If the object is a cell array, update the cell type set
			if (curType == DataObject::CELLARRAY)
				cellTypes = typeSetUnion(cellTypes, itr->getCellTypes());
		}
	}	
	
	// Add the current type info to the output set
	outSet.insert(TypeInfo(
		curType,
		is2D,
		isScalar,
		isInteger,
		sizeKnown,
		matSize,
		pFunction,
		cellTypes
	));
	
	// Return the output types
	return outSet;
}

/***************************************************************
* Function: typeSetStrMake(TypeInfo)
* Purpose : Build a type set string from a single type object
* Initial : Maxime Chevalier-Boisvert on April 22, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeSetString typeSetStrMake(const TypeInfo& type)
{
	// Create a type set and insert the type object
	TypeSet set;
	set.insert(type);
	
	// Return a type set string containing this set
	return TypeSetString(1, set);
}

/***************************************************************
* Function: typeSetStrMake(ArrayObj)
* Purpose : Build a type set string from a vector of objects
* Initial : Maxime Chevalier-Boisvert on April 23, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeSetString typeSetStrMake(const ArrayObj* pArgVector)
{
	// Create a type set string object
	TypeSetString typeSetStr;
	
	// Reserve space for all the arguments
	typeSetStr.reserve(pArgVector->getSize());
	
	// For each object in the argument vector
	for (size_t i = 0; i < pArgVector->getSize(); ++i)
	{
		// Get a pointer to this object
		DataObject* pArgument = pArgVector->getObject(i);
		
		// Create a type info object from the argument
		TypeInfo typeInfo(pArgument, false, true);
		
		// Create a type set and insert the type info object
		TypeSet typeSet;
		typeSet.insert(typeInfo);
		
		// Add the type set to the string
		typeSetStr.push_back(typeSet);		
	}
	
	// Return the type set string object
	return typeSetStr;	
}

/***************************************************************
* Function: nullTypeMapping()
* Purpose : Null type mapping function (returns no information)
* Initial : Maxime Chevalier-Boisvert on April 23, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeSetString nullTypeMapping(const TypeSetString& argTypes)
{
	// Return the empty type set string (no type information)
	return TypeSetString();
}

/***************************************************************
* Function: identTypeMapping()
* Purpose : Identity type mapping function
* Initial : Maxime Chevalier-Boisvert on April 17, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeSetString identTypeMapping(const TypeSetString& argTypes)
{
	// Return the argument types unchanged
	return argTypes;
}

/***************************************************************
* Function: complexScalarTypeMapping()
* Purpose : Complex output type mapping function
* Initial : Maxime Chevalier-Boisvert on July 29, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeSetString complexScalarTypeMapping(const TypeSetString& argTypes)
{
	// Return the type info for a scalar integer value
	return typeSetStrMake(TypeInfo(
		DataObject::MATRIX_C128,
		true,
		true,
		false,
		true,
		TypeInfo::DimVector(2,1),
		NULL,
		TypeSet()
	));
}

/***************************************************************
* Function: realScalarTypeMapping()
* Purpose : Scalar real-valued output type mapping function
* Initial : Maxime Chevalier-Boisvert on May 7, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeSetString realScalarTypeMapping(const TypeSetString& argTypes)
{
	// Return the type info for a scalar integer value
	return typeSetStrMake(TypeInfo(
		DataObject::MATRIX_F64,
		true,
		true,
		false,
		true,
		TypeInfo::DimVector(2,1),
		NULL,
		TypeSet()
	));
}

/***************************************************************
* Function: intScalarTypeMapping()
* Purpose : Scalar integer output type mapping function
* Initial : Maxime Chevalier-Boisvert on May 7, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeSetString intScalarTypeMapping(const TypeSetString& argTypes)
{
	// Return the type info for a scalar integer value
	return typeSetStrMake(TypeInfo(
		DataObject::MATRIX_F64,
		true,
		true,
		true,
		true,
		TypeInfo::DimVector(2,1),
		NULL,
		TypeSet()
	));
}

/***************************************************************
* Function: boolScalarTypeMapping()
* Purpose : Scalar boolean output type mapping function
* Initial : Maxime Chevalier-Boisvert on May 7, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeSetString boolScalarTypeMapping(const TypeSetString& argTypes)
{
	// Return the type info for a scalar integer value
	return typeSetStrMake(TypeInfo(
		DataObject::LOGICALARRAY,
		true,
		true,
		true,
		true,
		TypeInfo::DimVector(2,1),
		NULL,
		TypeSet()
	));
}

/***************************************************************
* Function: stringValueTypeMapping()
* Purpose : String value output type mapping function
* Initial : Maxime Chevalier-Boisvert on May 7, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeSetString stringValueTypeMapping(const TypeSetString& argTypes)
{
	// Return the type info for a string value
	return typeSetStrMake(TypeInfo(
		DataObject::CHARARRAY,
		true,
		false,
		true,
		false,
		TypeInfo::DimVector(),
		NULL,
		TypeSet()
	));
}

/***************************************************************
* Function: intUnaryOpTypeMapping()
* Purpose : Integer output unary operation type mapping function
* Initial : Maxime Chevalier-Boisvert on May 12, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeSetString intUnaryOpTypeMapping(const TypeSetString& argTypes)
{	
	// If we have no input type information
	if (argTypes.empty() || argTypes[0].empty())
	{
		// Return information about a generic integer matrix
		return typeSetStrMake(TypeInfo(
			DataObject::MATRIX_F64,
			false,
			false,
			true,
			false,
			TypeInfo::DimVector(),
			NULL,
			TypeSet()
		));
	}
	
	// Get references to the argument type sets
	const TypeSet& argSet1 = argTypes[0];
	
	// Create a set to store the possible output types
	TypeSet outSet; 
		
	// For each possible input type combination
	for (TypeSet::const_iterator type1 = argSet1.begin(); type1 != argSet1.end(); ++type1)
	{
		// Add the resulting type to the output set
		outSet.insert(TypeInfo(
			DataObject::MATRIX_F64,
			type1->is2D(),
			type1->isScalar(),
			true,
			type1->getSizeKnown(),
			type1->getMatSize(),
			NULL,
			TypeSet()
		));	
	}
	
	// Return the possible output types
	return TypeSetString(1, outSet);
}

/***************************************************************
* Function: arrayLogicOpTypeMapping()
* Purpose : Array logical operation type mapping function 
* Initial : Maxime Chevalier-Boisvert on April 26, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeSetString arrayLogicOpTypeMapping(const TypeSetString& argTypes)
{
	// If there are not two arguments, return no information
	if (argTypes.size() != 2)
		return TypeSetString();
	
	// Get references to the argument type sets
	const TypeSet& argSet1 = argTypes[0];
	const TypeSet& argSet2 = argTypes[1];
	
	// Create a set to store the possible output types
	TypeSet outSet; 
	
	// For each possible input type combination
	for (TypeSet::const_iterator type1 = argSet1.begin(); type1 != argSet1.end(); ++type1)
	{
		for (TypeSet::const_iterator type2 = argSet2.begin(); type2 != argSet2.end(); ++type2)
		{			
			// Add the resulting type to the output set
			outSet.insert(TypeInfo(
				DataObject::LOGICALARRAY,
				type1->is2D() && type2->is2D(),
				type1->isScalar() && type2->isScalar(),
				true,
				type1->getSizeKnown() && type2->getSizeKnown(),
				type1->isScalar()? type2->getMatSize():type1->getMatSize(),
				NULL,
				std::set<TypeInfo>()
			));
		}		
	}
	
	// Return the possible output types
	return TypeSetString(1, outSet);
}

/***************************************************************
* Function: multOpTypeMapping()
* Purpose : Multiplication operation type mapping function
* Initial : Maxime Chevalier-Boisvert on April 24, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeSetString multOpTypeMapping(const TypeSetString& argTypes)
{
	// If there are not two arguments, return no information
	if (argTypes.size() != 2)
		return TypeSetString();
	
	// Get references to the argument type sets
	const TypeSet& argSet1 = argTypes[0];
	const TypeSet& argSet2 = argTypes[1];
	
	// Create a set to store the possible output types
	TypeSet outSet; 
	
	// For each possible input type combination
	for (TypeSet::const_iterator type1 = argSet1.begin(); type1 != argSet1.end(); ++type1)
	{
		for (TypeSet::const_iterator type2 = argSet2.begin(); type2 != argSet2.end(); ++type2)
		{
			// Determine the output type
			DataObject::Type objType;
			if (type1->getObjType() == DataObject::MATRIX_C128 || type2->getObjType() == DataObject::MATRIX_C128)
				objType = DataObject::MATRIX_C128;
			else
				objType = DataObject::MATRIX_F64;
			
			// Test if the output size is known
			bool sizeKnown = type1->getSizeKnown() && type2->getSizeKnown();
			
			// Compute the size of the output matrix, if possible
			TypeInfo::DimVector matSize;
			if (sizeKnown)
			{
				if (type1->isScalar())
				{
					matSize = type2->getMatSize();
				}
				else if (type2->isScalar())
				{
					matSize = type1->getMatSize();
				}
				else if (type1->getMatSize().size() == 2 && type2->getMatSize().size() == 2) 
				{				
					matSize.push_back(type1->getMatSize()[0]);
					matSize.push_back(type2->getMatSize()[1]);
				}
				else
				{
					sizeKnown = false;
				}
			}
			
			// Test if the output will be a 2D matrix
			bool is2D = (sizeKnown && matSize.size() == 2);
			
			// Add the resulting type to the output set
			outSet.insert(TypeInfo(
				objType,
				is2D,
				type1->isScalar() && type2->isScalar(),
				type1->isInteger() && type2->isInteger(),
				sizeKnown,
				matSize,
				NULL,
				std::set<TypeInfo>()
			));
		}		
	}
	
	// Return the possible output types
	return TypeSetString(1, outSet);
}

/***************************************************************
* Function: divOpTypeMapping()
* Purpose : Division operation type mapping function
* Initial : Maxime Chevalier-Boisvert on April 24, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeSetString divOpTypeMapping(const TypeSetString& argTypes)
{
	// If there are not two arguments, return no information
	if (argTypes.size() != 2)
		return TypeSetString();
	
	// Get references to the argument type sets
	const TypeSet& argSet1 = argTypes[0];
	const TypeSet& argSet2 = argTypes[1];
	
	// Create a set to store the possible output types
	TypeSet outSet; 
	
	// For each possible input type combination
	for (TypeSet::const_iterator type1 = argSet1.begin(); type1 != argSet1.end(); ++type1)
	{
		for (TypeSet::const_iterator type2 = argSet2.begin(); type2 != argSet2.end(); ++type2)
		{
			// Determine the output type
			DataObject::Type objType;
			if (type1->getObjType() == DataObject::MATRIX_C128 || type2->getObjType() == DataObject::MATRIX_C128)
				objType = DataObject::MATRIX_C128;
			else
				objType = DataObject::MATRIX_F64;
			
			// Test if the output size is known
			bool sizeKnown = 
				type1->getSizeKnown() && 
				type2->getSizeKnown() && 
				type1->getMatSize().size() == 2 && 
				type2->getMatSize().size() == 2; 
			
			// Compute the size of the output matrix, if possible
			TypeInfo::DimVector matSize;
			if (sizeKnown)
			{
				if (type1->isScalar())
				{
					matSize = type2->getMatSize();
				}
				else if (type2->isScalar())
				{
					matSize = type1->getMatSize();
				}
				else
				{				
					matSize.push_back(type2->getMatSize()[0]);
					matSize.push_back(type1->getMatSize()[0]);
				}
			}
			
			// Add the resulting type to the output set
			outSet.insert(TypeInfo(
				objType,
				true,
				type1->isScalar() && type2->isScalar(),
				false,
				sizeKnown,
				matSize,
				NULL,
				std::set<TypeInfo>()
			));
		}		
	}
	
	// Return the possible output types
	return TypeSetString(1, outSet);
}

/***************************************************************
* Function: leftDivOpTypeMapping()
* Purpose : Left division operation type mapping function
* Initial : Maxime Chevalier-Boisvert on April 30, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeSetString leftDivOpTypeMapping(const TypeSetString& argTypes)
{
	// If there are not two arguments, return no information
	if (argTypes.size() != 2)
		return TypeSetString();
	
	// Get references to the argument type sets
	const TypeSet& argSet1 = argTypes[0];
	const TypeSet& argSet2 = argTypes[1];
	
	// Create a set to store the possible output types
	TypeSet outSet; 
	
	// For each possible input type combination
	for (TypeSet::const_iterator type1 = argSet1.begin(); type1 != argSet1.end(); ++type1)
	{
		for (TypeSet::const_iterator type2 = argSet2.begin(); type2 != argSet2.end(); ++type2)
		{
			// Determine the output type
			DataObject::Type objType;
			if (type1->getObjType() == DataObject::MATRIX_C128 || type2->getObjType() == DataObject::MATRIX_C128)
				objType = DataObject::MATRIX_C128;
			else
				objType = DataObject::MATRIX_F64;
			
			// Test if the output size is known
			bool sizeKnown = 
				type1->getSizeKnown() && 
				type2->getSizeKnown() && 
				type1->getMatSize().size() == 2 && 
				type2->getMatSize().size() == 2; 
			
			// Compute the size of the output matrix, if possible
			TypeInfo::DimVector matSize;
			if (sizeKnown)
			{
				if (type1->isScalar())
				{
					matSize = type2->getMatSize();
				}
				else if (type2->isScalar())
				{
					matSize = type1->getMatSize();
				}
				else
				{				
					matSize.push_back(type1->getMatSize()[1]);
					matSize.push_back(type2->getMatSize()[1]);
				}
			}
			
			// Add the resulting type to the output set
			outSet.insert(TypeInfo(
				objType,
				true,
				type1->isScalar() && type2->isScalar(),
				false,
				sizeKnown,
				matSize,
				NULL,
				std::set<TypeInfo>()
			));
		}		
	}
	
	// Return the possible output types
	return TypeSetString(1, outSet);
}

/***************************************************************
* Function: powerOpTypeMapping()
* Purpose : Power operation type mapping function
* Initial : Maxime Chevalier-Boisvert on April 28, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeSetString powerOpTypeMapping(const TypeSetString& argTypes)
{
	// If there are not two arguments, return no information
	if (argTypes.size() != 2)
		return TypeSetString();
	
	// Get references to the argument type sets
	const TypeSet& argSet1 = argTypes[0];
	const TypeSet& argSet2 = argTypes[1];
	
	// Create a set to store the possible output types
	TypeSet outSet; 
	
	// For each possible input type combination
	for (TypeSet::const_iterator type1 = argSet1.begin(); type1 != argSet1.end(); ++type1)
	{
		for (TypeSet::const_iterator type2 = argSet2.begin(); type2 != argSet2.end(); ++type2)
		{
			// Determine the output type
			DataObject::Type objType;
			if (type1->getObjType() == DataObject::MATRIX_C128 || type2->getObjType() == DataObject::MATRIX_C128)
				objType = DataObject::MATRIX_C128;
			else
				objType = DataObject::MATRIX_F64;
			
			// Add the resulting type to the output set
			outSet.insert(TypeInfo(
				objType,
				true,
				type1->isScalar(),
				false,
				type1->getSizeKnown(),
				type1->getMatSize(),
				NULL,
				std::set<TypeInfo>()
			));			
		}		
	}
	
	// Return the possible output types
	return TypeSetString(1, outSet);
}

/***************************************************************
* Function: transOpTypeMapping()
* Purpose : Transpose operation type mapping function
* Initial : Maxime Chevalier-Boisvert on April 25, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeSetString transpOpTypeMapping(const TypeSetString& argTypes)
{
	// If there is not one argument, return no information
	if (argTypes.size() != 1)
		return TypeSetString();
	
	// Get a reference to the argument type set
	const TypeSet& argSet1 = argTypes[0];
	
	// Create a set to store the possible output types
	TypeSet outSet; 
	
	// For each possible input type combination
	for (TypeSet::const_iterator type1 = argSet1.begin(); type1 != argSet1.end(); ++type1)
	{
		// Test if the output size is known
		bool sizeKnown = type1->getSizeKnown() && type1->getMatSize().size() == 2;
		
		// Compute the size of the output matrix, if possible
		TypeInfo::DimVector matSize;
		if (sizeKnown)
		{				
			matSize.push_back(type1->getMatSize()[1]);
			matSize.push_back(type1->getMatSize()[0]);
		}
		
		// Add the resulting type to the output set
		outSet.insert(TypeInfo(
			type1->getObjType(),
			type1->is2D(),
			type1->isScalar(),
			type1->isInteger(),
			sizeKnown,
			matSize,
			NULL,
			std::set<TypeInfo>()
		));
	}
	
	// Return the possible output types
	return TypeSetString(1, outSet);
}

/***************************************************************
* Function: minusOpTypeMapping()
* Purpose : Arithmetic negation operation type mapping function
* Initial : Maxime Chevalier-Boisvert on April 26, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeSetString minusOpTypeMapping(const TypeSetString& argTypes)
{
	// If there is not one argument, return no information
	if (argTypes.size() != 1)
		return TypeSetString();
	
	// Get a reference to the argument type set
	const TypeSet& argSet1 = argTypes[0];
	
	// Create a set to store the possible output types
	TypeSet outSet; 
	
	// For each possible input type combination
	for (TypeSet::const_iterator type1 = argSet1.begin(); type1 != argSet1.end(); ++type1)
	{	
		// Determine the output type
		DataObject::Type objType;
		if (type1->getObjType() == DataObject::MATRIX_C128)
			objType = DataObject::MATRIX_C128;
		else
			objType = DataObject::MATRIX_F64;
		
		// Add the resulting type to the output set
		outSet.insert(TypeInfo(
			objType,
			type1->is2D(),
			type1->isScalar(),
			type1->isInteger(),
			type1->getSizeKnown(),
			type1->getMatSize(),
			NULL,
			std::set<TypeInfo>()
		));		
	}
	
	// Return the possible output types
	return TypeSetString(1, outSet);
}

/***************************************************************
* Function: notOpTypeMapping()
* Purpose : Logical negation operation type mapping function
* Initial : Maxime Chevalier-Boisvert on April 26, 2009
****************************************************************
Revisions and bug fixes:
*/
TypeSetString notOpTypeMapping(const TypeSetString& argTypes)
{
	// If there is not one argument, return no information
	if (argTypes.size() != 1)
		return TypeSetString();
	
	// Get a reference to the argument type set
	const TypeSet& argSet1 = argTypes[0];
	
	// Create a set to store the possible output types
	TypeSet outSet; 
	
	// For each possible input type combination
	for (TypeSet::const_iterator type1 = argSet1.begin(); type1 != argSet1.end(); ++type1)
	{	
		// Add the resulting type to the output set
		outSet.insert(TypeInfo(
			DataObject::LOGICALARRAY,
			type1->is2D(),
			type1->isScalar(),
			true,
			type1->getSizeKnown(),
			type1->getMatSize(),
			NULL,
			std::set<TypeInfo>()
		));		
	}
	
	// Return the possible output types
	return TypeSetString(1, outSet);
}
