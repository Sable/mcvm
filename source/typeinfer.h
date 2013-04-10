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
#ifndef TYPEINFER_H_
#define TYPEINFER_H_

// Header files
#include <set>
#include <vector>
#include <string>
#include "platform.h"
#include "objects.h"
#include "arrayobj.h"

// Forward declaration of the function class
class Function;

/***************************************************************
* Class   : TypeInfo
* Purpose : Precisely represent an object's type
* Initial : Maxime Chevalier-Boisvert on April 13, 2009
****************************************************************
Revisions and bug fixes:
*/
class TypeInfo
{
public:
	
	// Matrix dimension vector type definition
	typedef std::vector<size_t> DimVector;
	
	// Default constructor
	TypeInfo();	
	
	// Constructor to build a type info object from a data object
	TypeInfo(const DataObject* pObject, bool storeMatDims, bool scanMatrices);
	
	// Full constructor
	TypeInfo(
		DataObject::Type objType,
		bool is2D,
		bool isScalar,
		bool isInteger,
		bool sizeKnown,
		DimVector matSize,
		Function* pFunction,
		std::set<TypeInfo> cellTypes
	);
	
	// Method to get a string representation of the type information
	std::string toString() const;
	
	// Equality comparison operator
	bool operator == (const TypeInfo& other) const;
	
	// Less-than comparison operator (for sorting)
	bool operator < (const TypeInfo& other) const;

	// Mutator to set the object type identifier
	void setObjType(DataObject::Type type) { m_objType = type; }
	
	// Mutator to set the 2D flag
	void set2D(bool is2D) { m_is2D = is2D; }
	
	// Mutator to set the scalar flag
	void setScalar(bool scalar) { m_isScalar = scalar; }
	
	// Mutator to set the integer flag
	void setInteger(bool integer) { m_isInteger = integer; }
	
	// Mutator to set the size known flag
	void setSizeKnown(bool sizeKnown) { m_sizeKnown = sizeKnown; }
	
	// Mutator to set the matrix size vector
	void setMatSize(const DimVector& matSize) { m_matSize = matSize; }
	
	// Mutator to set the cell array stored types
	void setCellTypes(const std::set<TypeInfo>& cellTypes) { m_cellTypes = cellTypes; }
	
	// Accessor to get the object type identifier
	DataObject::Type getObjType() const { return m_objType; }
	
	// Accessor to get the 2D flag
	bool is2D() const { return m_is2D; }
		
	// Accessor to get the scalar flag
	bool isScalar() const { return m_isScalar; }
	
	// Accessor to get the integer flag
	bool isInteger() const { return m_isInteger; }
	
	// Accessor to get the size known flag
	bool getSizeKnown() const { return m_sizeKnown; }
	
	// Accessor to get the matrix size vector
	const DimVector& getMatSize() const { return m_matSize; }
	
	// Accessor to get the function pointer
	Function* getFunction() const { return m_pFunction; }
	
	// Accessor to get the cell array stored types
	const std::set<TypeInfo>& getCellTypes() const { return m_cellTypes; }
	
private:
	
	// Data object type identifier
	DataObject::Type m_objType;
	
	// Bidimensional matrix flag
	bool m_is2D;
	
	// Scalar value flag
	bool m_isScalar;
	
	// Integer scalar value flag
	bool m_isInteger;
	
	// Matrix size known flag
	bool m_sizeKnown;
	
	// Matrix dimension for matrix types (may be unknown/empty)
	DimVector m_matSize;
	
	// Function pointer for handle types (may be unknown/null)
	Function* m_pFunction;
	
	// Type set for cell array stored types
	std::set<TypeInfo> m_cellTypes;
};

// Type set type definition
typedef std::set<TypeInfo> TypeSet;

// Type string type definition
typedef std::vector<TypeInfo> TypeString;

// Type set string type definition
typedef std::vector<TypeSet> TypeSetString;

// Type mapping function pointer definition
typedef TypeSetString (*TypeMapFunc)(const TypeSetString& argTypes);

// Function to validate a set of possible types for a data object
bool validateTypes(const DataObject* pObject, const TypeSet& typeSet);

// Function to build a type set from a single type object
TypeSet typeSetMake(const TypeInfo& type);

// Function to obtain the union of two type sets
TypeSet typeSetUnion(const TypeSet& setA, const TypeSet& setB);

// Function to reduce the possible types in a type set
TypeSet typeSetReduce(const TypeSet& set);

// Function to build a type set string from a single type object
TypeSetString typeSetStrMake(const TypeInfo& type);

// Function to build a type set string from a vector of objects
TypeSetString typeSetStrMake(const ArrayObj* pArgVector);

// Null type mapping function (returns no information)
TypeSetString nullTypeMapping(const TypeSetString& argTypes);

// Identity type mapping function
TypeSetString identTypeMapping(const TypeSetString& argTypes);

// Scalar complex output type mapping function
TypeSetString complexScalarTypeMapping(const TypeSetString& argTypes);

// Scalar real-valued output type mapping function
TypeSetString realScalarTypeMapping(const TypeSetString& argTypes);

// Scalar integer output type mapping function
TypeSetString intScalarTypeMapping(const TypeSetString& argTypes);

// Scalar boolean output type mapping function
TypeSetString boolScalarTypeMapping(const TypeSetString& argTypes);

// String value output type mapping function
TypeSetString stringValueTypeMapping(const TypeSetString& argTypes);

/***************************************************************
* Function: arrayArithOpTypeMapping()
* Purpose : Array arithmetic operation type mapping function
* Initial : Maxime Chevalier-Boisvert on April 23, 2009
****************************************************************
Revisions and bug fixes:
*/
template <bool intPreserve> TypeSetString arrayArithOpTypeMapping(const TypeSetString& argTypes)
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
				type1->is2D() && type2->is2D(),
				type1->isScalar() && type2->isScalar(),
				(type1->isInteger() && type2->isInteger()) && intPreserve,
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
* Function: unaryOpTypeMapping()
* Purpose : Unary operation type mapping function
* Initial : Maxime Chevalier-Boisvert on May 13, 2009
****************************************************************
Revisions and bug fixes:
*/
template <bool intPreserve> TypeSetString unaryOpTypeMapping(const TypeSetString& argTypes)
{
	// If there is not one argument, return no information
	if (argTypes.size() != 1)
		return TypeSetString();
	
	// Get references to the argument type sets
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
			type1->isInteger() && intPreserve,
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
* Function: vectorOpTypeMapping()
* Purpose : Vector operation type mapping function
* Initial : Maxime Chevalier-Boisvert on May 13, 2009
****************************************************************
Revisions and bug fixes:
*/
template <bool intPreserve> TypeSetString vectorOpTypeMapping(const TypeSetString& argTypes)
{
	// If there is not one argument, return no information
	if (argTypes.size() != 1)
		return TypeSetString();
	
	// Get references to the argument type sets
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
			false,
			type1->isInteger() && intPreserve,
			false,
			TypeInfo::DimVector(),
			NULL,
			TypeSet()
		));	
	}
	
	// Return the possible output types
	return TypeSetString(1, outSet);
}

// Integer output unary operation type mapping function
TypeSetString intUnaryOpTypeMapping(const TypeSetString& argTypes);

// Array logical operation type mapping function
TypeSetString arrayLogicOpTypeMapping(const TypeSetString& argTypes);

// Multiplication operation type mapping function
TypeSetString multOpTypeMapping(const TypeSetString& argTypes);

// Division operation type mapping function
TypeSetString divOpTypeMapping(const TypeSetString& argTypes);

// Left division operation type mapping function
TypeSetString leftDivOpTypeMapping(const TypeSetString& argTypes);

// Power operation type mapping function
TypeSetString powerOpTypeMapping(const TypeSetString& argTypes);

// Transpose operation type mapping function
TypeSetString transpOpTypeMapping(const TypeSetString& argTypes);

// Arithmetic negation operation type mapping function
TypeSetString minusOpTypeMapping(const TypeSetString& argTypes);

// Logical negation operation type mapping function
TypeSetString notOpTypeMapping(const TypeSetString& argTypes);

#endif // #ifndef TYPEINFER_H_ 
