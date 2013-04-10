// =========================================================================== //
//                                                                             //
// Copyright 2007 Maxime Chevalier-Boisvert and McGill University.             //
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
#include <cmath>
#include "runtimebase.h"
#include "matrixobjs.h"
#include "matrixops.h"
#include "chararrayobj.h"
#include "cellarrayobj.h"

/***************************************************************
* Function: RunError::RunError()
* Purpose : Constructor for run-time error class
* Initial : Maxime Chevalier-Boisvert on June 15, 2007
****************************************************************
Revisions and bug fixes:
*/	
RunError::RunError(const std::string& text, const IIRNode* pNode)
{
	// Add the error information
	addInfo(text, pNode);
}

/***************************************************************
* Function: RunError::addInfo()
* Purpose : Add error information
* Initial : Maxime Chevalier-Boisvert on June 15, 2007
****************************************************************
Revisions and bug fixes:
*/	
void RunError::addInfo(const std::string& text, const IIRNode* pNode)	
{
	// Create an error info object
	ErrorInfo errorInfo;
	
	// Store the error information
	errorInfo.text = text;
	errorInfo.pNode = pNode;
	
	// Add the error information to the stack
	m_errorStack.push_back(errorInfo);
}

/***************************************************************
* Function: RunError::toString()
* Purpose : Obtain a textual description of the error
* Initial : Maxime Chevalier-Boisvert on June 15, 2007
****************************************************************
Revisions and bug fixes:
*/	
std::string RunError::toString() const
{
	// Declare a string to store the description text
	std::string text;
	
	// For each entry in the stack
	for (size_t i = m_errorStack.size() - 1; i < m_errorStack.size(); --i)
	{
		// Get a reference to the error info
		const ErrorInfo& errorInfo = m_errorStack[i];
		
		// Add the error text to the string
		text += errorInfo.text;
		
		// If there is an associated expression
		if (errorInfo.pNode != NULL)
		{
			// If error text was provided
			if (errorInfo.text != "")
			{
				// Add a colon and a newline
				text += ":\n  ";
			}
			
			// Add a textual representation of the expression
			text += errorInfo.pNode->toString() + "\n"; 
		}
		
		// If this is not the last iteration
		if (i != 0)
		{
			// Add a newline
			text += "\n";
		}
	}	
	
	// Return the error description
	return text;
}

/***************************************************************
* Function: static RunError::throwError()
* Purpose : Throw a runtime error exception
* Initial : Maxime Chevalier-Boisvert on June 15, 2009
****************************************************************
Revisions and bug fixes:
*/
void RunError::throwError(const char* pText, const IIRNode* pNode)
{
	// Throw the runtime error exception
	throw RunError(pText, pNode);
}

/***************************************************************
* Function: getBoolValue()
* Purpose : Evaluate the boolean value of an object
* Initial : Maxime Chevalier-Boisvert on November 18, 2008
****************************************************************
Revisions and bug fixes:
*/
bool getBoolValue(const DataObject* pObject)
{
	// Switch on the type of the object
	switch (pObject->getType())
	{
		// 64-bit matrix
		case DataObject::MATRIX_F64:
		{
			// Get a typed pointer to the matrix
			const MatrixF64Obj* pMatrixObj = (MatrixF64Obj*)pObject;
			
			// Get pointers to the start and end values
			const float64* pStartVal = pMatrixObj->getElements();
			const float64* pEndVal = pStartVal + pMatrixObj->getNumElems();
			
			// For each value in the matrix
			for (const float64* pValue = pStartVal; pValue != pEndVal; ++pValue)
			{
				// If a single matrix element is false, the result is false
				if (*pValue == 0)
					return false;
			}
			
			// The result is a true boolean value
			return true;
		}
		break;

		// Logical array
		case DataObject::LOGICALARRAY:
		{
			// Get a typed pointer to the matrix
			const LogicalArrayObj* pMatrixObj = (LogicalArrayObj*)pObject;
			
			// Get pointers to the start and end values
			const bool* pStartVal = pMatrixObj->getElements();
			const bool* pEndVal = pStartVal + pMatrixObj->getNumElems();
			
			// For each value in the matrix
			for (const bool* pValue = pStartVal; pValue != pEndVal; ++pValue)
			{
				// If a single matrix element is false, the result is false
				if (*pValue == false)
					return false;
			}
			
			// The result is a true boolean value
			return true;
		}
		break;
		
		// Character array
		case DataObject::CHARARRAY:
		{
			// Get a typed pointer to the matrix
			const CharArrayObj* pMatrixObj = (CharArrayObj*)pObject;
			
			// Get pointers to the start and end values
			const char* pStartVal = pMatrixObj->getElements();
			const char* pEndVal = pStartVal + pMatrixObj->getNumElems();
			
			// For each value in the matrix
			for (const char* pValue = pStartVal; pValue != pEndVal; ++pValue)
			{
				// If a single matrix element is false, the result is false
				if (*pValue == 0)
					return false;
			}
			
			// The result is a true boolean value
			return true;
		}
		break;
		
		// Unsupported typed
		default:
		{
			throw RunError("unsupported object type in boolean evaluation");
		}
	}
	
	// Object evaluates to false
	return false;
}

/***************************************************************
* Function: getInt32Value()
* Purpose : Evaluate the integer value of an object
* Initial : Maxime Chevalier-Boisvert on January 29, 2009
****************************************************************
Revisions and bug fixes:
*/
int32 getInt32Value(const DataObject* pObject)
{
	// Switch on the type of the object
	switch (pObject->getType())
	{
		// 64-bit float matrix
		case DataObject::MATRIX_F64:
		{
			// Get a typed pointer to the matrix
			const MatrixF64Obj* pMatrixObject = (MatrixF64Obj*)pObject;
			
			// If the matrix is not a scalar
			if (!pMatrixObject->isScalar())
			{
				// Throw an exception
				throw RunError("nonscalar matrix in integer evaluation");				
			}
			
			// Get the floating-point value of the scalar
			float64 floatVal = pMatrixObject->getElem1D(1);
			
			// Get the integer and fractional parts of the value
			float64 fracPart;
			float64 intPart;
			fracPart = modf(floatVal, &intPart);
			
			// If the floating-point part is nonzero
			if (fracPart != 0)
			{
				// Throw an exception
				throw RunError("nonzero fractional part in integer evaluation");
			}

			// Return the integer part of the value
			return (int32)intPart;		
		}
		break;
		
		// Unsupported typed
		default:
		{
			throw RunError("unsupported object type in integer evaluation");
		}
	}
}

/***************************************************************
* Function: getInt64Value()
* Purpose : Evaluate the integer value of an object
* Initial : Maxime Chevalier-Boisvert on January 29, 2009
****************************************************************
Revisions and bug fixes:
*/
int64 getInt64Value(const DataObject* pObject)
{
	// Switch on the type of the object
	switch (pObject->getType())
	{
		// 64-bit float matrix
		case DataObject::MATRIX_F64:
		{
			// Get a typed pointer to the matrix
			const MatrixF64Obj* pMatrixObject = (MatrixF64Obj*)pObject;
			
			// If the matrix is not a scalar
			if (!pMatrixObject->isScalar())
			{
				// Throw an exception
				throw RunError("nonscalar matrix in integer evaluation");				
			}
			
			// Get the floating-point value of the scalar
			float64 floatVal = pMatrixObject->getElem1D(1);
			
			// Get the integer and fractional parts of the value
			float64 fracPart;
			float64 intPart;
			fracPart = modf(floatVal, &intPart);
			
			// If the floating-point part is nonzero
			if (fracPart != 0)
			{
				// Throw an exception
				throw RunError("nonzero fractional part in integer evaluation");
			}

			// Return the integer part of the value
			return (int64)intPart;		
		}
		break;
		
		// Character array
		case DataObject::CHARARRAY:
		{
			// Get a typed pointer to the matrix
			const CharArrayObj* pMatrixObject = (CharArrayObj*)pObject;
			
			// If the matrix is not a scalar
			if (!pMatrixObject->isScalar())
			{
				// Throw an exception
				throw RunError("nonscalar matrix in floating-point evaluation");				
			}
			
			// Get the integer value of the scalar
			int64 intVal = (int64)pMatrixObject->getScalar();
			
			// Return the integer value directly
			return intVal;
		}
		break;
		
		// Logical array
		case DataObject::LOGICALARRAY:
		{
			// Get a typed pointer to the matrix
			const LogicalArrayObj* pMatrixObject = (LogicalArrayObj*)pObject;
			
			// If the matrix is not a scalar
			if (!pMatrixObject->isScalar())
			{
				// Throw an exception
				throw RunError("nonscalar matrix in floating-point evaluation");				
			}
			
			// Get the integer value of the scalar
			int64 intVal = pMatrixObject->getScalar()? 1:0;
			
			// Return the integer value directly
			return intVal;
		}
		break;
		
		// Unsupported typed
		default:
		{
			throw RunError("unsupported object type in integer evaluation");
		}
	}
}

/***************************************************************
* Function: getFloat64Value()
* Purpose : Evaluate an object as an floating-point value
* Initial : Maxime Chevalier-Boisvert on February 4, 2009
****************************************************************
Revisions and bug fixes:
*/
float64 getFloat64Value(const DataObject* pObject)
{	
	// Switch on the type of the object
	switch (pObject->getType())
	{	
		// 64-bit float matrix
		case DataObject::MATRIX_F64:
		{
			// Get a typed pointer to the matrix
			const MatrixF64Obj* pMatrixObject = (MatrixF64Obj*)pObject;
			
			// If the matrix is not a scalar
			if (!pMatrixObject->isScalar())
			{
				// Throw an exception
				throw RunError("nonscalar matrix in floating-point evaluation");				
			}
			
			// Get the floating-point value of the scalar
			float64 floatVal = pMatrixObject->getScalar();
			
			// Return the floating-point value directly
			return floatVal;	
		}
		break;
		
		// Character array
		case DataObject::CHARARRAY:
		{
			// Get a typed pointer to the matrix
			const CharArrayObj* pMatrixObject = (CharArrayObj*)pObject;
			
			// If the matrix is not a scalar
			if (!pMatrixObject->isScalar())
			{
				// Throw an exception
				throw RunError("nonscalar matrix in floating-point evaluation");				
			}
			
			// Get the floating-point value of the scalar
			float64 floatVal = (float64)pMatrixObject->getScalar();
			
			// Return the floating-point value directly
			return floatVal;
		}
		break;
		
		// Logical array
		case DataObject::LOGICALARRAY:
		{
			// Get a typed pointer to the matrix
			const LogicalArrayObj* pMatrixObject = (LogicalArrayObj*)pObject;
			
			// If the matrix is not a scalar
			if (!pMatrixObject->isScalar())
			{
				// Throw an exception
				throw RunError("nonscalar matrix in floating-point evaluation");				
			}
			
			// Get the floating-point value of the scalar
			float64 floatVal = pMatrixObject->getScalar()? 1:0;
			
			// Return the floating-point value directly
			return floatVal;
		}
		break;
		
		// Unsupported typed
		default:
		{
			throw RunError("unsupported object type in floating-point evaluation");
		}
	}
}

/***************************************************************
* Function: getIndexValue()
* Purpose : Evaluate an object as an index (positive int)
* Initial : Maxime Chevalier-Boisvert on January 29, 2009
****************************************************************
Revisions and bug fixes:
*/
size_t getIndexValue(const DataObject* pObject)
{	
	// Switch on the type of the object
	switch (pObject->getType())
	{
		// 64-bit float matrix
		case DataObject::MATRIX_F64:
		{
			// Get a typed pointer to the matrix
			const MatrixF64Obj* pMatrixObject = (MatrixF64Obj*)pObject;
			
			// If the matrix is not a scalar
			if (!pMatrixObject->isScalar())
			{
				// Throw an exception
				throw RunError("nonscalar matrix in integer evaluation");				
			}
			
			// Get the floating-point value of the scalar
			float64 floatVal = pMatrixObject->getElem1D(1);
			
			// Get the integer and fractional parts of the value
			float64 fracPart;
			float64 intPart;
			fracPart = modf(floatVal, &intPart);
			
			// If the floating-point part is nonzero
			if (fracPart != 0)
			{
				// Throw an exception
				throw RunError("nonzero fractional part in index evaluation");
			}

			// If the value is negative
			if (fracPart < 0)
			{
				// Throw an exception
				throw RunError("negative fp-value in index evaluation");
			}

			// Cast the value as a positive integer
			return (size_t)intPart;		
		}
		break;
		
		// Unsupported typed
		default:
		{
			throw RunError("unsupported object type in integer evaluation");
		}
	}
}

/***************************************************************
* Function: createBlankObj()
* Purpose : Create a blank/uninitialized object
* Initial : Maxime Chevalier-Boisvert on February 18, 2009
****************************************************************
Revisions and bug fixes:
*/
DataObject* createBlankObj(DataObject::Type type)
{
	// Switch on the object type
	switch (type)
	{
		// 64-bit float matrix
		case DataObject::MATRIX_F64:
		return new MatrixF64Obj();
		break;

		// 128-bit complex matrix
		case DataObject::MATRIX_C128:
		return new MatrixC128Obj();
		break;
		
		// Logical array
		case DataObject::LOGICALARRAY:
		return new LogicalArrayObj();
		break;

		// Char array
		case DataObject::CHARARRAY:
		return new CharArrayObj();
		break;
		
		// Cell array
		case DataObject::CELLARRAY:
		return new CellArrayObj();
		break;
		
		// For all other object types
		default:
		{
			// Throw an exception
			throw RunError("cannot create an object of this type");
		}
	}
}
