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
#include <ctime>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <sys/time.h>
#include "stdlib.h"
#include "runtimebase.h"
#include "interpreter.h"
#include "jitcompiler.h"
#include "matrixobjs.h"
#include "matrixops.h"
#include "chararrayobj.h"
#include "cellarrayobj.h"
#include "utility.h"
#include "process.h"
#include "plotting.h"
#include "filesystem.h"

// Standard library name space
namespace StdLib
{
	// Start time value for the tic-toc timer system
	double ticTocStartTime = FLOAT_INFINITY;

	// Map of file ids to open file handles
	typedef std::map<size_t, FILE*> FileHandleMap; 
	FileHandleMap openFileMap;
		
	/***************************************************************
	* Function: parseMatSize()
	* Purpose : Parse matrix sizes from input arguments
	* Initial : Maxime Chevalier-Boisvert on April 14, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	DimVector parseMatSize(ArrayObj* pArguments)
	{	
		// Create a vector to store the matrix size
		DimVector matSize;
		matSize.reserve(pArguments->getSize());		
		
		// If there is only one size argument and it is a matrix
		if (pArguments->getSize() == 1 && pArguments->getObject(0)->isMatrixObj())
		{
			// Get a pointer to the size argument
			DataObject* pSizeArg = pArguments->getObject(0);
			
			// Convert the size matrix to a 64-bit floating point matrix
			MatrixF64Obj* pSizeMatrix;
			if (pSizeArg->getType() != DataObject::MATRIX_F64)
				pSizeMatrix = (MatrixF64Obj*)pSizeArg->convert(DataObject::MATRIX_F64);
			else
				pSizeMatrix = (MatrixF64Obj*)pSizeArg;
			
			// Ensure the size matrix is not empty
			if (pSizeMatrix->isEmpty())
				throw RunError("size matrix should not be empty");
			
			// For each element of the size matrix
			for (size_t i = 1; i <= pSizeMatrix->getNumElems(); ++i)
			{
				// Get the value of this element
				float64 value = pSizeMatrix->getElem1D(i);
				
				// Cast the value into an integer
				size_t intValue = size_t(value);
				
				// Ensure that the value is a positive integer
				if (value < 0 || value - intValue != 0)
					throw RunError("invalid dimension size");
				
				// Add the value to the dst size vector
				matSize.push_back(intValue);
			}
		}
		else
		{		
			// For each argument
			for (size_t i = 0; i < pArguments->getSize(); ++i)
			{
				// Extract this argument
				DataObject* pArg = pArguments->getObject(i);
				
				// Get the argument as an index value
				size_t sizeVal = getIndexValue(pArg);
				
				// Add the value to the size vector
				matSize.push_back(sizeVal);			
			}
		}
		
		// If there is only one dimension
		if (matSize.size() == 1)
		{
			// Make the matrix square
			matSize.push_back(matSize.front());
		}
		
		// Otherwise, if no arguments were specified
		else if (matSize.size() == 0)
		{
			// Make this a scalar matrix
			matSize.push_back(1);
			matSize.push_back(1);
		}
		
		// Return the parsed matrix size
		return matSize;
	}

	/***************************************************************
	* Function: analyzeSizeArgs()
	* Purpose : Produce type information for matrix size arguments
	* Initial : Maxime Chevalier-Boisvert on May 10, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	void analyzeMatSize(const TypeSetString& argTypes, bool& is2D)
	{	
		// Set the 2D flag to be true initially
		is2D = true;
		
		// If we have no type information, cannot guarantee matrix is 2D
		if (argTypes.empty())
			is2D = false;
		
		// If there are more than 2 arguments, matrix is not 2D
		if (argTypes.size() > 2)
			is2D = false;
		
		// If there is only one argument
		if (argTypes.size() == 1)
		{
			// Get the potential types for the first argument
			const TypeSet& firstArg = argTypes[0];
			
			// For each possible argument type
			for (TypeSet::const_iterator typeItr = firstArg.begin(); typeItr != firstArg.end(); ++typeItr)
			{
				// If this matrix is not 2D
				if (typeItr->is2D() == false)
				{
					// Cannot guarantee the matrix is 2D
					is2D = false;
				}
				
				// Otherwise, if the matrix is not scalar
				else if (typeItr->isScalar() == false)
				{
					// If the size is not known
					if (typeItr->getSizeKnown() == false)
					{
						// Cannot guarantee the matrix is 2D
						is2D = false;						
					}
					else
					{
						// Get the matrix size
						const TypeInfo::DimVector& matSize = typeItr->getMatSize();
						
						// If there are not two elements in the matrix
						if (matSize[0] * matSize[1] != 2)
						{
							// Cannot guarantee the matrix is 2D
							is2D = false;							
						}
					}					
				}				
			}			
		}
	}	
	
	/***************************************************************
	* Function: createMatrix()
	* Purpose : Create and initialize a matrix with a scalar value
	* Initial : Maxime Chevalier-Boisvert on January 29, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* createMatrix(ArrayObj* pArguments, float64 value)
	{		
		// Parse the matrix size from the input arguments
		DimVector matSize = parseMatSize(pArguments);
		
		// Create and initialize a new matrix
		MatrixF64Obj* pNewMatrix = new MatrixF64Obj(matSize, value);
		
		// Return the new matrix object
		return new ArrayObj(pNewMatrix);	
	}
	
	/***************************************************************
	* Function: createF64MatTypeMapping()
	* Purpose : Type mapping for f64 matrix creation functions
	* Initial : Maxime Chevalier-Boisvert on May 11, 2009
	****************************************************************
	Revisions and bug fixes:
	*/	
	TypeSetString createF64MatTypeMapping(const TypeSetString& argTypes)
	{
		// Analyze the matrix size arguments
		bool is2D;
		analyzeMatSize(argTypes, is2D);
		
		// Return the type information for the F64 matrix
		return typeSetStrMake(TypeInfo(
			DataObject::MATRIX_F64,
			is2D,
			false,
			false,
			false,
			TypeInfo::DimVector(),
			NULL,
			TypeSet()
		));
	}

	/***************************************************************
	* Function: createLogicalArray()
	* Purpose : Create and initialize a logical array
	* Initial : Maxime Chevalier-Boisvert on March 4, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* createLogicalArray(ArrayObj* pArguments, bool value)
	{
		// Parse the matrix size from the input arguments
		DimVector matSize = parseMatSize(pArguments);
		
		// Create and initialize a new matrix
		LogicalArrayObj* pNewMatrix = new LogicalArrayObj(matSize, value);
		
		// Return the new matrix object
		return new ArrayObj(pNewMatrix);	
	}
	
	/***************************************************************
	* Function: createLogArrTypeMapping()
	* Purpose : Type mapping for logical array creation functions
	* Initial : Maxime Chevalier-Boisvert on May 11, 2009
	****************************************************************
	Revisions and bug fixes:
	*/	
	TypeSetString createLogArrTypeMapping(const TypeSetString& argTypes)
	{
		// Analyze the matrix size arguments
		bool is2D;
		analyzeMatSize(argTypes, is2D);
		
		// Return the type information for the F64 matrix
		return typeSetStrMake(TypeInfo(
			DataObject::LOGICALARRAY,
			is2D,
			false,
			true,
			false,
			TypeInfo::DimVector(),
			NULL,
			TypeSet()
		));
	}
	
	/***************************************************************
	* Function: formatPrint()
	* Purpose : Perform formatted printing
	* Initial : Maxime Chevalier-Boisvert on February 3, 2009
	****************************************************************
	Revisions and bug fixes:
	*/	
	std::string formatPrint(ArrayObj* pArguments)
	{
		// If there are no arguments, throw an exception
		if (pArguments->getSize() == 0)
			throw RunError("insufficient argument count");
		
		// Get the format argument
		DataObject* pFormatArg = pArguments->getObject(0);
		
		// Ensure the format argument is a string
		if (pFormatArg->getType() != DataObject::CHARARRAY)
			throw RunError("the format argument must be a string");
		
		// Extract the format string
		std::string formatStr = ((CharArrayObj*)pFormatArg)->getString();
		
		// Declare an index for the next argument to use 
		size_t nextArg = 1;
		
		// Create a string to store the output
		std::string output;
		
		// For each character of the format string
		for (size_t charIndex = 0; charIndex < formatStr.length(); ++charIndex)
		{
			// Extrac the current character from the format string
			char thisChar = formatStr[charIndex];
	
			// If this is an escape sequence
			if (thisChar == '%')
			{
				// TODO: expand support
				
				char formatChar = formatStr[++charIndex];
				
				// Ensure that the argument count is sufficient
				if (nextArg >= pArguments->getSize())
					throw RunError("missing argument for output formatting");
				
				// Get the output argument
				DataObject* pOutArg = pArguments->getObject(nextArg++);
				
				// Switch on the format character
				switch (formatChar)
				{
					// Fixed-point format
					case 'd':
					case 'f':
					{
						double floatVal = getFloat64Value(pOutArg);
						
						output += ::toString(floatVal);
					}
					break;
					
					// Integer format
					case 'i':
					{
						long int intVal = getInt32Value(pOutArg);
						
						output += ::toString(intVal);
					}
					break;
					
					// String format
					case 's':
					{
						// Ensure that the argument is a string
						if (pOutArg->getType() != DataObject::CHARARRAY)
							throw RunError("invalid value for string format");
						
						// Get the string value
						std::string string = ((CharArrayObj*)pOutArg)->getString();
						
						// Add the string to the output
						output += string;
					}
					break;
					
					// Invalid format character
					default:
					{
						// Throw an exception
						throw RunError("unsupported format character in format string");
					}
				}				
				
				// Do not output this character
				continue;
			}			
			
			// If this is the start of an escape sequence
			if (thisChar == '\\')
			{
				// Get the escape character
				char escChar = formatStr[++charIndex];
					
				// Switch on the escape character
				switch (escChar)
				{
					// Newline
					case 'n':
					output += '\n';
					break;

					// Tab character
					case 't':
					output += '\t';
					break;
					
					// Quotation mark
					case '\'':
					if (formatStr[++charIndex] == '\'')
						output += '\'';
					break;
				}
				
				// Do not output this character
				continue;
			}			
			
			// Add the character to the output
			output += thisChar;
		}		
		
		// Return the output string
		return output;
	}
	
	/***************************************************************
	* Function: parseVectorArgs()
	* Purpose : Parse the arguments for a vector operation
	* Initial : Maxime Chevalier-Boisvert on March 5, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	BaseMatrixObj* parseVectorArgs(ArrayObj* pArguments, size_t& opDim)
	{
		// Ensure the argument count is valid
		if (pArguments->getSize() == 0 || pArguments->getSize() > 2)
			throw RunError("invalid argument count");
	
		// Ensure that the first argument is a matrix
		if (pArguments->getObject(0)->isMatrixObj() == false)
			throw RunError("expected matrix argument");
		
		// Get a pointer to the matrix argument
		BaseMatrixObj* pMatrixArg = (BaseMatrixObj*)pArguments->getObject(0);
			
		// Get the size of the input matrix
		const DimVector& inSize = pMatrixArg->getSize();
		
		// If there is a second argument
		if (pArguments->getSize() > 1)
		{
			// Get the value of the dimension argument
			opDim = getIndexValue(pArguments->getObject(1));
			
			// Convert the dimension index to zero indexing
			opDim = toZeroIndex(opDim);
			
			// Ensure the dimension argument is valid
			if (opDim > inSize.size())
				throw RunError("invalid dimension argument");
		}
		else
		{
			// Initialize the operating dimension to 0
			opDim = 0;
			
			// For each dimension
			for (size_t i = 0; i < inSize.size(); ++i)
			{
				// If this dimension is non-singular
				if (inSize[i] > 1)
				{
					// Store its index, then break out of this loop
					opDim = i;
					break;
				}
			}
		}
		
		// Return the matrix argument
		return pMatrixArg;
	}
	
	/***************************************************************
	* Function: absFunc()
	* Purpose : Compute absolute values of numbers
	* Initial : Maxime Chevalier-Boisvert on February 20, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* absFunc(ArrayObj* pArguments)
	{
		// Ensure there is one argument
		if (pArguments->getSize() != 1)
			throw RunError("invalid argument count");
	
		// Get a pointer to the argument
		DataObject* pArgument = pArguments->getObject(0);

		// If the argument is a 128-bit complex matrix
		if (pArgument->getType() == DataObject::MATRIX_C128)
		{
			// Get a typed pointer to the argument
			MatrixC128Obj* pInMatrix = (MatrixC128Obj*)pArgument;
			
			// Apply the operator to obtain the output
			MatrixF64Obj* pOutMatrix = MatrixC128Obj::arrayOp<AbsOp<Complex128, float64>, float64>(pInMatrix);
			
			// Return the output matrix
			return new ArrayObj(pOutMatrix);
		}
		
		// Convert the argument into a 64-bit floating-point matrix, if necessary
		if (pArgument->getType() != DataObject::MATRIX_F64)
			pArgument = pArgument->convert(DataObject::MATRIX_F64);
			
		// Get a typed pointer to the argument
		MatrixF64Obj* pInMatrix = (MatrixF64Obj*)pArgument;
			
		// Apply the operator to obtain the output
		MatrixF64Obj* pOutMatrix = MatrixF64Obj::arrayOp<AbsOp<float64>, float64>(pInMatrix);		
			
		// Return the output matrix
		return new ArrayObj(pOutMatrix);
	}

	/***************************************************************
	* Function: absFunc()
	* Purpose : Type mapping for the abs function
	* Initial : Maxime Chevalier-Boisvert on May 13, 2009
	****************************************************************
	Revisions and bug fixes:
	*/	
	TypeSetString absFuncTypeMapping(const TypeSetString& argTypes)
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
			// Add the resulting type to the output set
			outSet.insert(TypeInfo(
				DataObject::MATRIX_F64,
				type1->is2D(),
				type1->isScalar(),
				type1->isInteger(),
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
	* Function: anyFunc()
	* Purpose : Test if a matrix contains nonzero elements
	* Initial : Maxime Chevalier-Boisvert on March 5, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* anyFunc(ArrayObj* pArguments)
	{
		// Parse the input arguments
		size_t opDim;
		BaseMatrixObj* pMatrixArg = parseVectorArgs(pArguments, opDim);

		// If the argument is a logical array
		if (pMatrixArg->getType() == DataObject::LOGICALARRAY)
		{			
			// Get a typed pointer to the argument
			LogicalArrayObj* pInMatrix = (LogicalArrayObj*)pMatrixArg;
			
			// Perform the vector operation to get the result
			LogicalArrayObj* pOutMatrix = LogicalArrayObj::vectorOp<AnyOp<bool>, bool >(pInMatrix, opDim);

			// Return the output matrix
			return new ArrayObj(pOutMatrix);
		}
		
		// If the argument is a 64-bit float matrix
		else if (pMatrixArg->getType() == DataObject::MATRIX_F64)
		{
			// Get a typed pointer to the argument
			MatrixF64Obj* pInMatrix = (MatrixF64Obj*)pMatrixArg;
			
			// Perform the vector operation to get the result
			LogicalArrayObj* pOutMatrix = MatrixF64Obj::vectorOp<AnyOp<float64>, bool>(pInMatrix, opDim);

			// Return the output matrix
			return new ArrayObj(pOutMatrix);
		}
		
		// For all other argument types
		else
		{
			// Throw an exception
			throw RunError("unsupported argument type");
		}
	}

	/***************************************************************
	* Function: anyFuncTypeMapping()
	* Purpose : Type mapping for the "any" library function
	* Initial : Maxime Chevalier-Boisvert on May 15, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	TypeSetString anyFuncTypeMapping(const TypeSetString& argTypes)
	{
		// If there is not one argument
		if (argTypes.size() != 1)
		{
			// Return the type info for a generic logical array
			return typeSetStrMake(TypeInfo(
				DataObject::LOGICALARRAY,
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
				DataObject::LOGICALARRAY,
				type1->is2D(),
				false,
				true,
				false,
				TypeInfo::DimVector(),
				NULL,
				TypeSet()
			));	
		}
		
		// Return the possible output types
		return TypeSetString(1, outSet);
	}
	
	/***************************************************************
	* Function: blkdiagFunc()
	* Purpose : Create a block diagonal matrix
	* Initial : Maxime Chevalier-Boisvert on March 11, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* blkdiagFunc(ArrayObj* pArguments)
	{
		// Ensure there is one argument
		if (pArguments->getSize() == 0)
			throw RunError("insufficient argument count");
		
		// Declare arguments for the resultign matrix size
		size_t numRows = 0;
		size_t numCols = 0;
		
		// For each input argument
		for (size_t i = 0; i < pArguments->getSize(); ++i)
		{
			// Get a pointer to this argument
			DataObject* pArg = pArguments->getObject(i); 
			
			// Ensure this argument is a matrix
			if (pArg->isMatrixObj() == false)
				throw RunError("non-matrix object in input");
			
			// Ensure this argument is not a complex matrix
			if (pArg->getType() == DataObject::MATRIX_C128)
				throw RunError("complex matrices not supported");
		
			// Get a typed pointer to the matrix
			BaseMatrixObj* pMatrix = (BaseMatrixObj*)pArg;
			
			// Ensure this matrix is bidimensional
			if (pMatrix->is2D() == false)
				throw RunError("non-2D matrix in input");
			
			// Update the resulting matrix size
			numRows += pMatrix->getSize()[0];
			numCols += pMatrix->getSize()[1];
		}
	
		// Create a matrix to store the output
		MatrixF64Obj* pOutMatrix = new MatrixF64Obj(numRows, numCols);
		
		// Declare variables for the starting row and column
		size_t startRow = 0;
		size_t startCol = 0;
		
		// For each input argument
		for (size_t i = 0; i < pArguments->getSize(); ++i)
		{
			// Get a pointer to this argument
			DataObject* pArg = pArguments->getObject(i);
			
			// Conver the object to a 64-bit floating-point matrix, if necessary
			MatrixF64Obj* pMatrix;
			if (pArg->getType() == DataObject::MATRIX_F64)
				pMatrix = (MatrixF64Obj*)pArg;
			else
				pMatrix = (MatrixF64Obj*)pArg->convert(DataObject::MATRIX_F64);
			
			// Get the number of rows and columns of this matrix
			size_t inRows = pMatrix->getSize()[0];
			size_t inCols = pMatrix->getSize()[1];
			
			// For each row of the input matrix
			for (size_t inRow = 1; inRow <= inRows; ++inRow)
			{
				// For each column
				for (size_t inCol = 1; inCol <= inCols; ++inCol)
				{
					// Read this value from the input matrix
					float64 value = pMatrix->getElem2D(inRow, inCol);
					
					// Write the value in the output matrix
					pOutMatrix->setElem2D(startRow + inRow, startCol + inCol, value);
				}				
			}
			
			// Update the starting row and column indices
			startRow += inRows;
			startCol += inCols;
		}	
		
		// Return the output matrix
		return new ArrayObj(pOutMatrix);
	}
	
	/***************************************************************
	* Function: blkdiagFuncTypeMapping()
	* Purpose : Type mapping for the "blkdiag" library function
	* Initial : Maxime Chevalier-Boisvert on May 14, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	TypeSetString blkdiagFuncTypeMapping(const TypeSetString& argTypes)
	{
		// Create a vector to store the output size
		TypeInfo::DimVector outSize(2, 0);
		
		// Flag to indicate whether the output size is known
		bool sizeKnown = true;
		
		// Flag to indicate whether the output is integer
		bool isInteger = true;
		
		// For each argument
		for (size_t i = 0; i < argTypes.size(); ++i)
		{
			// Get references to the argument type set
			const TypeSet& typeSet = argTypes[0];
			
			// If the type set is empty
			if (typeSet.empty())
			{
				// Set the flags to pessimistic values
				sizeKnown = false;
				isInteger = false;
				
				// Move to the next argument
				continue;
			}
				
			// Variables for the number of rows and columns of the argument
			size_t numRows = 0;
			size_t numCols = 0;
			
			// For each possible input type combination
			for (TypeSet::const_iterator type = typeSet.begin(); type != typeSet.end(); ++type)
			{
				// Get the matrix size
				const TypeInfo::DimVector matSize = type->getMatSize();
				
				// If the size is not known or the matrix is not 2D
				if (type->getSizeKnown() == false || matSize.size() != 2)
				{
					// Cannot know the output size
					sizeKnown = false;
				}
				else
				{
					// If this is not the first type in the set
					if (type != typeSet.begin())
					{
						// If other arguments have different sizes, the output size is unknown
						if (matSize[0] != numRows || matSize[1] != numCols)
							sizeKnown = false;
						
						// Store the matrix size
						numRows = matSize[0];
						numCols = matSize[1];
					}
				}
				
				// If this is not an integer matrix, the output is not integer
				if (type->isInteger() == false)
					isInteger = false;
			}
			
			// Update the output matrix size
			outSize[0] += numRows;
			outSize[1] += numCols;		
		}
		
		// Return the type info for the output matrix
		return typeSetStrMake(TypeInfo(
			DataObject::MATRIX_F64,
			true,
			sizeKnown && outSize[0] == 1 && outSize[1] == 1,
			isInteger,
			sizeKnown,
			outSize,
			NULL,
			TypeSet()
		));
	}
	
	/***************************************************************
	* Function: bitwsandFunc()
	* Purpose : Apply the bitwise AND operation
	* Initial : Maxime Chevalier-Boisvert on February 26, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* bitwsandFunc(ArrayObj* pArguments)
	{
		// Ensure there is one argument
		if (pArguments->getSize() != 2)
			throw RunError("invalid argument count");
	
		// Get a pointer to the arguments
		DataObject* pLArg = pArguments->getObject(0);
		DataObject* pRArg = pArguments->getObject(1);	
		
		// If the arguments are two floating-point matrices
		if (pLArg->getType() == DataObject::MATRIX_F64 || pRArg->getType() == DataObject::MATRIX_F64)
		{
			// Get typed pointers to the arguments
			MatrixF64Obj* pLMatrix = (MatrixF64Obj*)pLArg;
			MatrixF64Obj* pRMatrix = (MatrixF64Obj*)pRArg;

			// Apply the operation to obtain the result
			MatrixF64Obj* pOutMatrix = MatrixF64Obj::binArrayOp<BitAndOp<float64>, float64>(pLMatrix, pRMatrix);
			
			// Return the output value
			return new ArrayObj(pOutMatrix);
		}
		
		// For all other argument types
		else
		{
			// Throw an exception
			throw RunError("unsupported argument type combination");
		}
	}
	
	/***************************************************************
	* Function: cdFunc()
	* Purpose : Change the current working directory
	* Initial : Maxime Chevalier-Boisvert on February 20, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* cdFunc(ArrayObj* pArguments)
	{
		// Ensure there is exactly one argument
		if (pArguments->getSize() != 1)
			throw RunError("invalid argument count");
		
		// Ensure the argument is a string
		if (pArguments->getObject(0)->getType() != DataObject::CHARARRAY)
			throw RunError("expected string argument");
		
		// Get the directory argument
		CharArrayObj* pDirArg = (CharArrayObj*)pArguments->getObject(0);
		
		// Set the working directory
		bool result = setWorkingDir(pDirArg->getString());
		
		// If the operation failed
		if (result == false)
		{
			// Print an error message
			std::cout << "directory change failed" << std::endl;
		}
		
		// Otherwise, if the operation was successful
		else
		{
			// Clear program functions from the interpreter's global environment
			Interpreter::clearProgFuncs();
		}		
		
		// Return nothing
		return new ArrayObj();
	}
	
	/***************************************************************
	* Function: ceilFunc()
	* Purpose : Apply the ceil function
	* Initial : Maxime Chevalier-Boisvert on February 22, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* ceilFunc(ArrayObj* pArguments)
	{
		// Ensure there is one argument
		if (pArguments->getSize() != 1)
			throw RunError("invalid argument count");
	
		// Get a pointer to the argument
		DataObject* pArgument = pArguments->getObject(0);
		
		// If the argument is a matrix
		if (pArgument->getType() == DataObject::MATRIX_F64)
		{
			// Get a typed pointer to the argument
			MatrixF64Obj* pInMatrix = (MatrixF64Obj*)pArgument;
			
			// Create a new matrix to store the output
			MatrixF64Obj* pOutMatrix = new MatrixF64Obj(pInMatrix->getSize());
			
			// Compute a pointer to the last element of the input matrix
			float64* pLastElem = pInMatrix->getElements() + pInMatrix->getNumElems();
			
			// For each element of the matrices
			for (float64 *pIn = pInMatrix->getElements(), *pOut = pOutMatrix->getElements(); pIn < pLastElem; ++pIn, ++pOut)
			{
				// Apply the ceil function to this element
				*pOut = ::ceil(*pIn);
			}			
			
			// Return the output matrix
			return new ArrayObj(pOutMatrix);
		}
		
		// For all other argument types
		else
		{
			// Throw an exception
			throw RunError("unsupported argument type");
		}
	}
	
	/***************************************************************
	* Function: cellFunc()
	* Purpose : Create cell arrays
	* Initial : Maxime Chevalier-Boisvert on March 11, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* cellFunc(ArrayObj* pArguments)
	{
		// Ensure there is at least one argument
		if (pArguments->getSize() == 0)
			throw RunError("insufficient argument count");
		
		// Create a vector to store the matrix size
		DimVector matSize;
		
		// For each argument
		for (size_t i = 0; i < pArguments->getSize(); ++i)
		{
			// Extract this argument
			DataObject* pArg = pArguments->getObject(i);
			
			// Get the argument as an index value
			size_t sizeVal = getIndexValue(pArg);
			
			// Add the value to the size vector
			matSize.push_back(sizeVal);			
		}		
		
		// If there is only one dimension
		if (matSize.size() == 1)
		{
			// Make the matrix square
			matSize.push_back(matSize.front());
		}
		
		// Create and initialize a new cell array
		CellArrayObj* pNewMatrix = new CellArrayObj(matSize, new CellArrayObj());
		
		// Return the new matrix object
		return new ArrayObj(pNewMatrix);
	}
	
	/***************************************************************
	* Function: createCellArrTypeMapping()
	* Purpose : Type mapping for cell array creation functions
	* Initial : Maxime Chevalier-Boisvert on May 11, 2009
	****************************************************************
	Revisions and bug fixes:
	*/	
	TypeSetString createCellArrTypeMapping(const TypeSetString& argTypes)
	{
		// Analyze the matrix size arguments
		bool is2D;
		analyzeMatSize(argTypes, is2D);
		
		// Set the possible cell types to be empty cell arrays
		TypeSet cellTypes;
		cellTypes.insert(TypeInfo(
			DataObject::CELLARRAY,
			true,
			false,
			false,
			true,
			TypeInfo::DimVector(2, 0),
			NULL,
			cellTypes
		));		
		
		// Return the type information for the F64 matrix
		return typeSetStrMake(TypeInfo(
			DataObject::CELLARRAY,
			is2D,
			false,
			false,
			false,
			TypeInfo::DimVector(),
			NULL,
			cellTypes
		));
	}
	
	/***************************************************************
	* Function: clockFunc()
	* Purpose : Return current time information in a vector
	* Initial : Maxime Chevalier-Boisvert on February 2, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* clockFunc(ArrayObj* pArguments)
	{
		// Ensure there are no arguments
		if (pArguments->getSize() != 0)
			throw RunError("invalid argument count");

		// Create a matrix to store the output
		MatrixF64Obj* pOutMatrix = new MatrixF64Obj(1, 6);
		
		// Create a timeval struct to store the time info
		struct timeval timeVal;
		
		// Get the current time
		gettimeofday(&timeVal, NULL);

		// Convert the time info into a structured form
		struct tm* timeInfo = localtime(&timeVal.tv_sec);

		// Compute more precise time in seconds using microsecond information
		double timeSecs = timeInfo->tm_sec + timeVal.tv_usec * 1.0e-6;
		
		// Set the time info into the output matrix
		pOutMatrix->setElem1D(1, (double)timeInfo->tm_year + 1900);
		pOutMatrix->setElem1D(2, (double)timeInfo->tm_mon + 1);
		pOutMatrix->setElem1D(3, (double)timeInfo->tm_mday);
		pOutMatrix->setElem1D(4, (double)timeInfo->tm_hour);
		pOutMatrix->setElem1D(5, (double)timeInfo->tm_min);
		pOutMatrix->setElem1D(6, timeSecs);
		
		// Return the output matrix
		return new ArrayObj(pOutMatrix);
	}
	
	/***************************************************************
	* Function: clockFuncTypeMapping()
	* Purpose : Type mapping for the "clock" library function
	* Initial : Maxime Chevalier-Boisvert on May 14, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	TypeSetString clockFuncTypeMapping(const TypeSetString& argTypes)
	{
		// Return the type info for an 1x6 floating-point vector
		TypeInfo::DimVector outSize;
		outSize.push_back(1);
		outSize.push_back(6);
		return typeSetStrMake(TypeInfo(
			DataObject::MATRIX_F64,
			true,
			false,
			false,
			true,
			outSize,
			NULL,
			TypeSet()
		));	
	}

	/***************************************************************
	* Function: cosFunc()
	* Purpose : Compute the cosine of numbers
	* Initial : Maxime Chevalier-Boisvert on February 26, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* cosFunc(ArrayObj* pArguments)
	{
		// Ensure there is one argument
		if (pArguments->getSize() != 1)
			throw RunError("invalid argument count");
	
		// Get a pointer to the argument
		DataObject* pArgument = pArguments->getObject(0);
		
		// If the argument is a matrix
		if (pArgument->getType() == DataObject::MATRIX_F64)
		{
			// Get a typed pointer to the argument
			MatrixF64Obj* pInMatrix = (MatrixF64Obj*)pArgument;
			
			// Create a new matrix to store the output
			MatrixF64Obj* pOutMatrix = new MatrixF64Obj(pInMatrix->getSize());
			
			// Compute a pointer to the last element of the input matrix
			float64* pLastElem = pInMatrix->getElements() + pInMatrix->getNumElems();
			
			// For each element of the matrices
			for (float64 *pIn = pInMatrix->getElements(), *pOut = pOutMatrix->getElements(); pIn < pLastElem; ++pIn, ++pOut)
			{
				// Compute the sine of this element
				*pOut = ::cos(*pIn);
			}			
			
			// Return the output matrix
			return new ArrayObj(pOutMatrix);
		}
		
		// For all other argument types
		else
		{
			// Throw an exception
			throw RunError("unsupported argument type");
		}
	}
	
	/***************************************************************
	* Function: diagFunc()
	* Purpose : Create diagonal matrices
	* Initial : Maxime Chevalier-Boisvert on March 2, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* diagFunc(ArrayObj* pArguments)
	{
		// Ensure there is either one or two argument
		if (pArguments->getSize() < 1 || pArguments->getSize() > 2)
			throw RunError("invalid argument count");

		// Extract the matrix argument
		BaseMatrixObj* pMatrixArg = (BaseMatrixObj*)pArguments->getObject(0);
		
		// Declare a variable for the diagonal index argument
		int diag = 0;
		
		// If the diagonal argument was supplied, extract it
		if (pArguments->getSize() == 2)
			diag = getInt32Value(pArguments->getObject(1));
		
		// Declare a variable for the diagonal length
		size_t diagLen;
		
		// Declare a variable for the output matrix size
		size_t matSize = 0;
		
		// If the input matrix is a vector
		if (pMatrixArg->isVector())
		{
			// Compute the diagonal length
			diagLen = pMatrixArg->getNumElems();
			
			// Compute the resulting matrix size length
			matSize = diagLen + std::abs(diag);
		}
		else
		{
			// Get the size of the input matrix
			DimVector matSize = pMatrixArg->getSize();
			
			// Ensure the matrix is bidimensional
			if (matSize.size() != 2)
				throw RunError("expected 2D matrix");
			
			// Compute the diagonal length
			diagLen = std::min(matSize[0], matSize[1]) - std::abs(diag);	
		}
		
		// Initialize the row and column offsets
		size_t rowOffset = 0;
		size_t colOffset = 0;
		
		// If the diagonal index is positive
		if (diag > 0)
		{
			// Ensure the diagonal index is valid
			if (pMatrixArg->isVector() == false && size_t(diag) >= pMatrixArg->getSize()[1])
				throw RunError("diagonal index is too large");
			
			// Modify the starting column based on the diagonal index
			colOffset += diag;
		}
		else
		{
			// Ensure the diagonal index is valid
			if (pMatrixArg->isVector() == false && size_t(-diag) >= pMatrixArg->getSize()[0])
				throw RunError("diagonal index is too large");
			
			// Modify the starting row based on the diagonal index
			rowOffset += -diag;
		}
			
		// If the argument is a 64-bit float matrix
		if (pArguments->getObject(0)->getType() == DataObject::MATRIX_F64)
		{
			// Get a typed pointer to the input matrix
			MatrixF64Obj* pInMatrix = (MatrixF64Obj*)pMatrixArg;
			
			// Declare a pointer for the output matrix
			MatrixF64Obj* pOutMatrix;
			
			// If the input matrix is a vector
			if (pInMatrix->isVector())
			{
				// Create a square matrix of this size and initialize it to 0
				pOutMatrix = new MatrixF64Obj(matSize, matSize, 0);
				
				// Set the diagonal of the output matrix from the input matrix
				for (size_t i = 1; i <= diagLen; ++i)
					pOutMatrix->setElem2D(rowOffset + i, colOffset + i, pInMatrix->getElem1D(i));
			}
			
			// Otherwiswe the input matrix is not a vector
			else
			{
				// Create a vector for the output matrix
				pOutMatrix = new MatrixF64Obj(diagLen, 1);
				
				// Set the output matrix from the diagonal of the input matrix
				for (size_t i = 1; i <= diagLen; ++i)
					pOutMatrix->setElem1D(i, pInMatrix->getElem2D(rowOffset + i, colOffset + i));				
			}
			
			// Return the output matrix
			return new ArrayObj(pOutMatrix);
		}
		else
		{
			// Throw an exception
			throw RunError("unsupported input type");
		}		
	}
	
	/***************************************************************
	* Function: diagFuncTypeMapping()
	* Purpose : Type mapping for the "diag" library function
	* Initial : Maxime Chevalier-Boisvert on May 23, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	TypeSetString diagFuncTypeMapping(const TypeSetString& argTypes)
	{
		// If there is are no arguments, return no information
		if (argTypes.empty())
			return TypeSetString();
		
		// Get references to the argument type sets
		const TypeSet& argSet1 = argTypes[0];
		
		// Create a set to store the possible output types
		TypeSet outSet; 
		
		// For each possible input type combination
		for (TypeSet::const_iterator type1 = argSet1.begin(); type1 != argSet1.end(); ++type1)
		{
			// Get the size of the input matrix
			const TypeInfo::DimVector& inSize = type1->getMatSize();
			
			// Determine whether the output size is known
			bool sizeKnown = argTypes.size() == 1 && type1->getSizeKnown() && inSize.size() == 2;  
			
			// Create a vector to store the size of the output matrix
			TypeInfo::DimVector outSize;
			
			// If the output size can be determined
			if (sizeKnown)
			{
				// If the input matrix is a vector
				if (inSize[0] == 1 || inSize[1] == 1)
				{
					// Determine the size of the output matrix
					size_t matSize = inSize[0] * inSize[1];
					outSize.push_back(matSize);
					outSize.push_back(matSize);
				}
				else
				{
					// Determine the size of the output vector
					size_t vecLen = std::min(inSize[0], inSize[1]);
					outSize.push_back(vecLen);
					outSize.push_back(1);
				}
			}			
			
			// Add the resulting type to the output set
			outSet.insert(TypeInfo(
				type1->getObjType(),
				true,
				sizeKnown && outSize[0] == 1 && outSize[1] == 1,
				type1->isInteger(),
				sizeKnown,
				outSize,
				NULL,
				TypeSet()
			));	
		}
		
		// Return the possible output types
		return TypeSetString(1, outSet);
	}	
	
	/***************************************************************
	* Function: dispFunc()
	* Purpose : Display text to the console
	* Initial : Maxime Chevalier-Boisvert on November 16, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* dispFunc(ArrayObj* pArguments)
	{
		// Ensure there is exactly one argument
		if (pArguments->getSize() != 1)
			throw RunError("invalid argument count");
		
		// Display the argument as a string
		std::cout << pArguments->getObject(0)->toString() << std::endl;		
		
		// Return nothing
		return new ArrayObj();
	}

	/***************************************************************
	* Function: dotFunc()
	* Purpose : Compute dot products of vectors
	* Initial : Maxime Chevalier-Boisvert on March 3, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* dotFunc(ArrayObj* pArguments)
	{
		// Ensure there are two arguments
		if (pArguments->getSize() != 2)
			throw RunError("invalid argument count");
	
		// Get pointers to the arguments
		DataObject* pArg0 = pArguments->getObject(0);
		DataObject* pArg1 = pArguments->getObject(1);

		// If the arguments are matrices
		if (pArg0->getType() == DataObject::MATRIX_F64 && pArg1->getType() == DataObject::MATRIX_F64)
		{			
			// Get a typed pointer to the arguments
			MatrixF64Obj* pMatrix0 = (MatrixF64Obj*)pArg0;
			MatrixF64Obj* pMatrix1 = (MatrixF64Obj*)pArg1;

			// Ensure that the matrix dimensions match
			if (pMatrix0->getNumElems() != pMatrix1->getNumElems())
				throw RunError("matrix dimensions do not match");
			
			// If the input is empty, return a copy of it
			if (pMatrix0->isEmpty())
				return new ArrayObj(pMatrix0->copy());
			
			// Get the size of the input matrix
			const DimVector& inSize = pMatrix0->getSize();
			
			// Declare variables to store the index and length of the first non-singular dimension
			size_t firstDim = 0;
			size_t firstDimLen = 0;
			
			// For each dimension
			for (size_t i = 0; i < inSize.size(); ++i)
			{
				// If this dimension is non-singular
				if (inSize[i] > 1)
				{
					// Store its index and length, then break out of this loop
					firstDim = i;
					firstDimLen = inSize[i];
					break;
				}
			}
			
			// Compute the size of the output matrix
			DimVector outSize = inSize;
			outSize[firstDim] = 1;			
			
			// Create a new matrix to store the output
			MatrixF64Obj* pOutMatrix = new MatrixF64Obj(outSize);
				
			// Compute a pointer past the last element of the input matrix
			float64* pLastElem = pMatrix0->getElements() + pMatrix0->getNumElems();
			
			// For each vector inside the input matrices
			for (
				float64 *pVec0 = pMatrix0->getElements(), *pVec1 = pMatrix1->getElements(), *pOut = pOutMatrix->getElements();
				pVec0 < pLastElem;
				pVec0 += firstDimLen, pVec1 += firstDimLen, ++pOut
			)
			{
				// Initialize the sum to 0
				float64 sum = 0;
				
				// Compute a pointer past the last element of this vector
				float64* pLastInVec = pVec0 + firstDimLen;
				
				// Add all vector element products to the sum
				for (float64 *pIn0 = pVec0, *pIn1 = pVec1; pIn0 < pLastInVec; ++pIn0, ++pIn1)
					sum += (*pIn0) * (*pIn1);
				
				// Store the sum in the output
				*pOut = sum;
			}		

			// Return the output matrix
			return new ArrayObj(pOutMatrix);
		}
		
		// For all other argument types
		else
		{
			// Throw an exception
			throw RunError("unsupported argument types");
		}
	}

	/***************************************************************
	* Function: dotFuncTypeMapping()
	* Purpose : Type mapping for the "dot" library function
	* Initial : Maxime Chevalier-Boisvert on May 15, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	TypeSetString dotFuncTypeMapping(const TypeSetString& argTypes)
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
					type1->isInteger() && type2->isInteger(),
					false,
					TypeInfo::DimVector(),
					NULL,
					std::set<TypeInfo>()
				));
			}		
		}
		
		// Return the possible output types
		return TypeSetString(1, outSet);
	}
	
	/***************************************************************
	* Function: evalFunc()
	* Purpose : Evaluate text strings as statements
	* Initial : Maxime Chevalier-Boisvert on March 16, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* evalFunc(ArrayObj* pArguments)
	{
		// Ensure there is one argument
		if (pArguments->getSize() != 1)
			throw RunError("invalid argument count");
	
		// Ensure the argument is a character array
		if (pArguments->getObject(0)->getType() != DataObject::CHARARRAY)
			throw RunError("expected string argument");
		
		// Get the string contents
		std::string string = ((CharArrayObj*)pArguments->getObject(0))->getString();
		
		// Run the command string in the global environment
		Interpreter::runCommand(string);
		
		// Return nothing
		return new ArrayObj();
	}
	
	/***************************************************************
	* Function: epsFunc()
	* Purpose : Obtain floating-point epsilon values
	* Initial : Maxime Chevalier-Boisvert on April 14, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* epsFunc(ArrayObj* pArguments)
	{
		// Ensure there is one argument
		if (pArguments->getSize() != 0)
			throw RunError("invalid argument count");
	
		// Return the standard epsilon value 2^-52
		return new ArrayObj(new MatrixF64Obj(powf(2, -52)));
	}

	/***************************************************************
	* Function: existFunc()
	* Purpose : Determine the existence of an object
	* Initial : Maxime Chevalier-Boisvert on July 11, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* existFunc(ArrayObj* pArguments)
	{
		// Ensure there is one argument
		if (pArguments->getSize() != 1)
			throw RunError("invalid argument count");
	
		// Extract the argument
		DataObject* pArg0 = pArguments->getObject(0);
		
		// Ensure the argument is a character array
		if (pArg0->getType() != DataObject::CHARARRAY)
			throw RunError("expected string argument");
		
		// Extract the symbol name string
		std::string symName = ((CharArrayObj*)pArg0)->getString();
		
		// Get the symbol for this name
		SymbolExpr* pSymbol = SymbolExpr::getSymbol(symName);
		
		// Declare a pointer for the object
		DataObject* pObject;
		
		// Lookup the object in the global environment
		try
		{
			pObject = Interpreter::evalGlobalSym(pSymbol);
		}
		
		// If the lookup failed
		catch (RunError error)
		{
			// The object does not exist, return 0
			return new ArrayObj(new MatrixF64Obj(0));
		}
		
		// Switch on the object type
		switch (pObject->getType())
		{
			// If the object is a function
			case DataObject::FUNCTION:
			{
				// Get a typed pointer to the function
				Function* pFunction = (Function*)pObject;
				
				// If this is a program function
				if (pFunction->isProgFunction())
				{
					// The variable is bound to a program function (M-file)
					return new ArrayObj(new MatrixF64Obj(2));
				}
				else
				{
					// The variable is bound to a library function
					return new ArrayObj(new MatrixF64Obj(5));
				}
			}
			break;
		
			// All other object types
			default:
			{
				// The variable is bound to some data object
				return new ArrayObj(new MatrixF64Obj(1));
			}			
		}		
	}
	
	/***************************************************************
	* Function: expFunc()
	* Purpose : Apply the exponential function
	* Initial : Maxime Chevalier-Boisvert on February 20, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* expFunc(ArrayObj* pArguments)
	{
		// Ensure there is one argument
		if (pArguments->getSize() != 1)
			throw RunError("invalid argument count");
	
		// Get a pointer to the argument
		DataObject* pArgument = pArguments->getObject(0);
		
		// If the argument is a 128-bit complex matrix
		if (pArgument->getType() == DataObject::MATRIX_C128)
		{
			// Get a typed pointer to the argument
			MatrixC128Obj* pInMatrix = (MatrixC128Obj*)pArgument;
			
			// Apply the operator to obtain the output
			MatrixC128Obj* pOutMatrix = MatrixC128Obj::arrayOp<ExpOp<Complex128>, Complex128>(pInMatrix);
			
			// Return the output matrix
			return new ArrayObj(pOutMatrix);
		}
		
		// Convert the argument into a 64-bit floating-point matrix, if necessary
		if (pArgument->getType() != DataObject::MATRIX_F64)
			pArgument = pArgument->convert(DataObject::MATRIX_F64);
			
		// Get a typed pointer to the argument
		MatrixF64Obj* pInMatrix = (MatrixF64Obj*)pArgument;
			
		// Apply the operator to obtain the output
		MatrixF64Obj* pOutMatrix = MatrixF64Obj::arrayOp<ExpOp<float64>, float64>(pInMatrix);
		
		// Return the output matrix
		return new ArrayObj(pOutMatrix);
	}
	
	/***************************************************************
	* Function: eyeFunc()
	* Purpose : Generate identity matrices
	* Initial : Maxime Chevalier-Boisvert on April 14, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* eyeFunc(ArrayObj* pArguments)
	{
		// Parse the matrix size from the input arguments
		DimVector matSize = parseMatSize(pArguments);
		
		// Ensure that there are at most two dimensions
		if (matSize.size() > 2)
			throw RunError("matrix cannot have more than two dimensions");
		
		// Create a square matrix of this size and initialize it to 0
		MatrixF64Obj* pOutMatrix = new MatrixF64Obj(matSize, 0);
		
		// Compute the diagonal length
		size_t diagLen = std::min(matSize[0], matSize[1]); 
		
		// Set the diagonal elements to 1
		for (size_t i = 1; i <= diagLen; ++i)
			pOutMatrix->setElem2D(i, i, 1);
		
		// Return the output matrix
		return new ArrayObj(pOutMatrix);
	}
	
	/***************************************************************
	* Function: falseFunc()
	* Purpose : Create and initialize a logical array
	* Initial : Maxime Chevalier-Boisvert on March 4, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* falseFunc(ArrayObj* pArguments)
	{
		// Create and initialize the logical array
		return createLogicalArray(pArguments, false);
	}

	/***************************************************************
	* Function: fcloseFunc()
	* Purpose : Close files by file handle
	* Initial : Maxime Chevalier-Boisvert on July 11, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* fcloseFunc(ArrayObj* pArguments)
	{
		// Ensure there is one argument
		if (pArguments->getSize() != 1)
			throw RunError("invalid argument count");
		
		// Get the file id of the file to close
		size_t fileId = getIndexValue(pArguments->getObject(0));
		
		// If the file is a standard I/O channel
		if (fileId <= 2)
			throw RunError("cannot close standard I/O channel");
		
		// Attempt to find the file id in the open file map
		FileHandleMap::iterator fileItr = openFileMap.find(fileId);
		
		// If the file is not open, the operation fails, return -1
		if (fileItr == openFileMap.end())
			return new ArrayObj(new MatrixF64Obj(-1));
		
		// Close the file
		::fclose(fileItr->second);
		
		// Remove the entry from the open file map
		openFileMap.erase(fileId);
		
		// The operation was successful, return 0
		return new ArrayObj(new MatrixF64Obj(0));
	}
	
	/***************************************************************
	* Function: fevalFunc()
	* Purpose : Evaluate functions by handle or name
	* Initial : Maxime Chevalier-Boisvert on February 26, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* fevalFunc(ArrayObj* pArguments)
	{
		// Ensure there is at least one argument
		if (pArguments->getSize() < 1)
			throw RunError("insufficient argument count");		
		
		// If the first argument is not a function handle, throw an exception
		if (pArguments->getObject(0)->getType() != DataObject::FN_HANDLE)
			throw RunError("can only apply feval to function handles");
		
		// Get a typed pointer to the function handle
		FnHandleObj* pHandle = (FnHandleObj*)pArguments->getObject(0);
		
		// Get a pointer to the function object
		Function* pFunction = pHandle->getFunction();
		
		// Create an array object for the function arguments
		ArrayObj* pFuncArgs = new ArrayObj(pArguments->getSize() - 1); 
		
		// For each remaining argument
		for (size_t i = 1; i < pArguments->getSize(); ++i)
		{
			// Add the argument to the function arguments
			ArrayObj::addObject(pFuncArgs, pArguments->getObject(i));
		}
		
		// Call the function with the supplied arguments
		return Interpreter::callFunction(pFunction, pFuncArgs);
	}

	/***************************************************************
	* Function: findFunc()
	* Purpose : Find nonzero values in matrices
	* Initial : Maxime Chevalier-Boisvert on April 13, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* findFunc(ArrayObj* pArguments)
	{
		// Ensure there is one argument
		if (pArguments->getSize() != 1)
			throw RunError("invalid argument count");
	
		// Get a pointer to the argument
		DataObject* pArgument = pArguments->getObject(0);

		// Ensure that the input argument is a matrix
		if (pArgument->isMatrixObj() == false)
			throw RunError("invalid input argument");
				
		// Get a typed pointer to the matrix
		BaseMatrixObj* pInMatrix = (BaseMatrixObj*)pArgument;
		
		// If the input matrix is empty, return a copy of it
		if (pInMatrix->isEmpty())
			return new ArrayObj(pInMatrix->copy());
		
		// Create a vector to store the nonzero element indices
		std::vector<float64> indices;
		indices.reserve(pInMatrix->getNumElems());
		
		// If the argument is a floating-point matrix
		if (pArgument->getType() == DataObject::MATRIX_F64)
		{			
			// Get a typed pointer to the argument
			MatrixF64Obj* pMatrix = (MatrixF64Obj*)pInMatrix;
			
			// For each matrix element
			for (size_t i = 1; i <= pMatrix->getNumElems(); ++i)
			{
				// If this element is nonzero, add its index to the vector
				if (pMatrix->getElem1D(i) != 0)
					indices.push_back(i);
			}
		}
		
		// If the argument is a logical array
		else if (pArgument->getType() == DataObject::LOGICALARRAY)
		{			
			// Get a typed pointer to the argument
			LogicalArrayObj* pMatrix = (LogicalArrayObj*)pInMatrix;
			
			// For each matrix element
			for (size_t i = 1; i <= pMatrix->getNumElems(); ++i)
			{
				// If this element is nonzero, add its index to the vector
				if (pMatrix->getElem1D(i) != 0)
					indices.push_back(i);
			}
		}
		
		// Otherwise, for any other matrix type
		else
		{
			// Throw an exception
			throw RunError("unsupported input type");
		}
		
		// Determine the output vector size
		size_t numRows = indices.size();
		size_t numCols = 1;
		
		// If the input matrix is a horizontal vector, change the output vector orientation
		if (pInMatrix->isVector() && pInMatrix->getSize()[0] == 1)
			std::swap(numRows, numCols);
		
		// Create a vector to store the output
		MatrixF64Obj* pOutMatrix = new MatrixF64Obj(numRows, numCols);
		
		// For each index value
		for (size_t i = 0; i < indices.size(); ++i)
		{
			// Write this index in the output matrix
			pOutMatrix->setElem1D(i + 1, indices[i]);
		}		
		
		// Return the output matrix
		return new ArrayObj(pOutMatrix);
	}
	
	/***************************************************************
	* Function: findFuncTypeMapping()
	* Purpose : Type mapping for the "find" library function
	* Initial : Maxime Chevalier-Boisvert on May 15, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	TypeSetString findFuncTypeMapping(const TypeSetString& argTypes)
	{
		// Return type information for the index vector
		return typeSetStrMake(TypeInfo(
			DataObject::MATRIX_F64,
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
	* Function: fixFunc()
	* Purpose : Round towards zero
	* Initial : Maxime Chevalier-Boisvert on February 26, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* fixFunc(ArrayObj* pArguments)
	{
		// Ensure there is one argument
		if (pArguments->getSize() != 1)
			throw RunError("invalid argument count");
	
		// Get a pointer to the argument
		DataObject* pArgument = pArguments->getObject(0);
		
		// If the argument is a matrix
		if (pArgument->getType() == DataObject::MATRIX_F64)
		{
			// Get a typed pointer to the argument
			MatrixF64Obj* pInMatrix = (MatrixF64Obj*)pArgument;
			
			// Create a new matrix to store the output
			MatrixF64Obj* pOutMatrix = new MatrixF64Obj(pInMatrix->getSize());
			
			// Compute a pointer to the last element of the input matrix
			float64* pLastElem = pInMatrix->getElements() + pInMatrix->getNumElems();
			
			// For each element of the matrices
			for (float64 *pIn = pInMatrix->getElements(), *pOut = pOutMatrix->getElements(); pIn < pLastElem; ++pIn, ++pOut)
			{
				// Apply the floor function to this element
				*pOut = ::floor(*pIn);
			}			
			
			// Return the output matrix
			return new ArrayObj(pOutMatrix);
		}
		
		// For all other argument types
		else
		{
			// Throw an exception
			throw RunError("unsupported argument type");
		}
	}
	
	/***************************************************************
	* Function: floorFunc()
	* Purpose : Apply the floor function
	* Initial : Maxime Chevalier-Boisvert on February 22, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* floorFunc(ArrayObj* pArguments)
	{
		// Ensure there is one argument
		if (pArguments->getSize() != 1)
			throw RunError("invalid argument count");
	
		// Get a pointer to the argument
		DataObject* pArgument = pArguments->getObject(0);
		
		// If the argument is a matrix
		if (pArgument->getType() == DataObject::MATRIX_F64)
		{
			// Get a typed pointer to the argument
			MatrixF64Obj* pInMatrix = (MatrixF64Obj*)pArgument;
			
			// Create a new matrix to store the output
			MatrixF64Obj* pOutMatrix = new MatrixF64Obj(pInMatrix->getSize());
			
			// Compute a pointer to the last element of the input matrix
			float64* pLastElem = pInMatrix->getElements() + pInMatrix->getNumElems();
			
			// For each element of the matrices
			for (float64 *pIn = pInMatrix->getElements(), *pOut = pOutMatrix->getElements(); pIn < pLastElem; ++pIn, ++pOut)
			{
				// Apply the floor function to this element
				*pOut = ::floor(*pIn);
			}			
			
			// Return the output matrix
			return new ArrayObj(pOutMatrix);
		}
		
		// For all other argument types
		else
		{
			// Throw an exception
			throw RunError("unsupported argument type");
		}
	}
	
	/***************************************************************
	* Function: fopenFunc()
	* Purpose : Open files and return a file handle
	* Initial : Maxime Chevalier-Boisvert on July 11, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* fopenFunc(ArrayObj* pArguments)
	{
		// Ensure there are two arguments
		if (pArguments->getSize() != 2)
			throw RunError("invalid argument count");
		
		// Extract the firat and second arguments
		DataObject* pArg0 = pArguments->getObject(0);
		DataObject* pArg1 = pArguments->getObject(1);
		
		// Ensure both arguments are strings
		if (pArg0->getType() != DataObject::CHARARRAY || pArg1->getType() != DataObject::CHARARRAY)
			throw RunError("invalid input argument types");
		
		// Get the file name and mode strings from the arguments
		std::string fileNameStr = ((CharArrayObj*)pArg0)->getString();
		std::string modeStr = ((CharArrayObj*)pArg1)->getString();
		
		// Determine if the access mode is valid
		bool validMode = ( 
			modeStr == "r"	||
			modeStr == "w"	||
			modeStr == "a"	||
			modeStr == "r+"	||
			modeStr == "w+"	||
			modeStr == "a+"
		);
		
		// Ensure that the access mode is valid
		if (validMode == false)
			throw RunError("unsupported file access mode \"" + modeStr + "\"");
		
		// Attempt to open the file
		FILE* pFileHandle = ::fopen(fileNameStr.c_str(), modeStr.c_str());
		
		// If the file could not be opened, return the value -1
		if (pFileHandle == NULL)
			return new ArrayObj(new MatrixF64Obj(-1));
		
		// Find the lowest available file id
		size_t fileId = 3;
		while (openFileMap.find(fileId) != openFileMap.end()) ++fileId;
	
		// Add an entry in the open file map for this file
		openFileMap[fileId] = pFileHandle;
	
		// Return the file id of the newly opened file
		return new ArrayObj(new MatrixF64Obj(fileId));
	}
	
	/***************************************************************
	* Function: fprintfFunc()
	* Purpose : Print text strings into streams
	* Initial : Maxime Chevalier-Boisvert on January 30, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* fprintfFunc(ArrayObj* pArguments)
	{
		// Ensure there is at least one argument
		if (pArguments->getSize() == 0)
			throw RunError("insufficient argument count");

		// Extract the firat argument
		DataObject* pFirstArg = pArguments->getObject(0);
		
		// Declare a variable for the output file index
		size_t outIndex;
		
		// Declare a string to the output text
		std::string outText;
		
		// If the first argument is a string
		if (pFirstArg->getType() == DataObject::CHARARRAY)
		{
			// The output file is standard output
			outIndex = 1;
			
			// Perform formatted printing on the input arguments directly
			outText = formatPrint(pArguments);
		}
		else
		{
			// Extract the output index argument
			outIndex = getIndexValue(pFirstArg);
		
			// Store the remaining arguments in a separate array
			ArrayObj* pRemArgs = new ArrayObj();
			for (size_t i = 1; i < pArguments->getSize(); ++i)
				ArrayObj::addObject(pRemArgs, pArguments->getObject(i));
			
			// Perform formatted printing
			outText = formatPrint(pRemArgs);
		}		
		
		// If the desired output is standard out
		if (outIndex == 1)
		{
			// Send the output to stdout
			std::cout << outText;
		}
		
		// For all other output index values
		else
		{
			// Try to find the file in the open file map
			FileHandleMap::iterator fileItr = openFileMap.find(outIndex);
			
			// Ensure that the file is open
			if (fileItr == openFileMap.end())
				throw RunError("invalid file id");
			
			// Write the output text to the file
			::fputs(outText.c_str(), fileItr->second);
		}		
		
		// Return nothing
		return new ArrayObj();
	}
	
	/***************************************************************
	* Function: iFunc()
	* Purpose : Return the imaginary constant i
	* Initial : Maxime Chevalier-Boisvert on July 29, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* iFunc(ArrayObj* pArguments)
	{
		// Return the constant pi
		return new ArrayObj(new MatrixC128Obj(Complex128(0, 1)));
	}

	/***************************************************************
	* Function: iscellFunc()
	* Purpose : Determine if an object is a cell array
	* Initial : Maxime Chevalier-Boisvert on March 17, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* iscellFunc(ArrayObj* pArguments)
	{
		// Ensure there is exactly one argument
		if (pArguments->getSize() != 1)
			throw RunError("invalid argument count");
		
		// Test if the object is a cell array
		bool isCell = (pArguments->getObject(0)->getType() == DataObject::CELLARRAY);
			
		// Return the result
		return new ArrayObj(new LogicalArrayObj(isCell));		
	}
	
	/***************************************************************
	* Function: isemptyFunc()
	* Purpose : Determine if a matrix is empty
	* Initial : Maxime Chevalier-Boisvert on March 4, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* isemptyFunc(ArrayObj* pArguments)
	{
		// Ensure there is exactly one argument
		if (pArguments->getSize() != 1)
			throw RunError("invalid argument count");
		
		// Ensure the argument is a matrix
		if (pArguments->getObject(0)->isMatrixObj() == false)
			throw RunError("input must be a matrix");
		
		// Test if the matrix is empty or not
		bool result = ((BaseMatrixObj*)pArguments->getObject(0))->isEmpty();
		
		// Return the result
		return new ArrayObj(new LogicalArrayObj(result));		
	}

	/***************************************************************
	* Function: isequalFunc()
	* Purpose : Determine if matrices are equal
	* Initial : Maxime Chevalier-Boisvert on March 4, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* isequalFunc(ArrayObj* pArguments)
	{
		// Ensure there are at least two arguments
		if (pArguments->getSize() < 2)
			throw RunError("insufficient argument count");
		
		// Ensure the first argument is a matrix
		if (pArguments->getObject(0)->getType() != DataObject::MATRIX_F64)
			throw RunError("arguments must be matrices");
		
		// Get a reference to the first matrix
		MatrixF64Obj* pPrevMatrix = (MatrixF64Obj*)pArguments->getObject(0);
		
		// For each subsequent matrix
		for (size_t i = 1; i < pArguments->getSize(); ++i)
		{
			// Ensure this argument is a matrix
			if (pArguments->getObject(i)->getType() != DataObject::MATRIX_F64)
				throw RunError("arguments must be matrices");
		
			// Get a reference to this matrix
			MatrixF64Obj* pCurrentMatrix = (MatrixF64Obj*)pArguments->getObject(i);

			// If the matrix sizes do not match, return false
			if (pCurrentMatrix->getSize() != pPrevMatrix->getSize())
				return new ArrayObj(new LogicalArrayObj(false));
			
			// Compare the matrix elements
			int result = memcmp(
				pPrevMatrix->getElements(),
				pCurrentMatrix->getElements(), 
				pPrevMatrix->getNumElems() * sizeof(float64)
			);
			
			// If the matrix elements do not match, return false
			if (result != 0)
				return new ArrayObj(new LogicalArrayObj(false));
			
			// Update the previous matrix pointer
			pPrevMatrix = pCurrentMatrix;
		}
		
		// The matrices are equal
		return new ArrayObj(new LogicalArrayObj(true));		
	}
	
	/***************************************************************
	* Function: isnumericFunc()
	* Purpose : Determine if an object is a numeric value
	* Initial : Maxime Chevalier-Boisvert on July 10, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* isnumericFunc(ArrayObj* pArguments)
	{
		// Ensure there is exactly one argument
		if (pArguments->getSize() != 1)
			throw RunError("invalid argument count");
		
		// Get the object type
		DataObject::Type objType = pArguments->getObject(0)->getType();
		
		// Test whether the object is a numeric value or not
		bool result = (objType == DataObject::MATRIX_F64 || objType == DataObject::MATRIX_C128);
		
		// Return the result
		return new ArrayObj(new LogicalArrayObj(result));
	}
	
	/***************************************************************
	* Function: lengthFunc()
	* Purpose : Apply the length function
	* Initial : Maxime Chevalier-Boisvert on February 22, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* lengthFunc(ArrayObj* pArguments)
	{
		// Ensure there is one argument
		if (pArguments->getSize() != 1)
			throw RunError("invalid argument count");
	
		// Get a pointer to the argument
		DataObject* pArgument = pArguments->getObject(0);
		
		// If the argument is a matrix
		if (pArgument->isMatrixObj())
		{
			// Get a typed pointer to the argument
			BaseMatrixObj* pMatrix = (BaseMatrixObj*)pArgument;
			
			// Get the size vector of the matrix
			const DimVector& sizeVec = pMatrix->getSize();
			
			// Initialize the length value to 0
			size_t length = 0;
			
			// For each size element
			for (DimVector::const_iterator itr = sizeVec.begin(); itr != sizeVec.end(); ++itr)
			{
				// Update the maximum value
				length = std::max(length, *itr);
			}			
			
			// Return the length value
			return new ArrayObj(new MatrixF64Obj(length));
		}
		
		// For all other argument types
		else
		{
			// Throw an exception
			throw RunError("unsupported argument type");
		}
	}

	/***************************************************************
	* Function: loadFunc()
	* Purpose : Load workspace variables from files
	* Initial : Maxime Chevalier-Boisvert on March 4, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* loadFunc(ArrayObj* pArguments)
	{
		// Ensure there is one argument
		if (pArguments->getSize() != 1)
			throw RunError("invalid argument count");
	
		// Get a pointer to the argument
		DataObject* pArgument = pArguments->getObject(0);
		
		// Ensure the argument is a string
		if (pArgument->getType() != DataObject::CHARARRAY)
			throw RunError("the format argument must be a string");
		
		// Extract the filename string
		std::string fileName = ((CharArrayObj*)pArgument)->getString();
		
		// Declare a string to store the text input
		std::string input;
		
		// Attempt to read the text file
		bool result = readTextFile(fileName, input);
		
		// Ensure the file was read successfully
		if (!result)
			throw RunError("could not read input file: \"" + fileName + "\"");
		
		// Tokenize the input into multiple lines
		std::vector<std::string> lines = tokenize(input, "\r\n", false, true);
		
		// Declare a vector of vector to store the values
		std::vector<std::vector<float64> > values;
		
		// For each input line
		for (size_t i = 0; i < lines.size(); ++i)
		{			
			// Tokenize this line based on whitespace
			std::vector<std::string> tokens = tokenize(lines[i], "\t ", false, true);
			
			// Declare a vector for the values in this row
			std::vector<float64> rowVals;

			// Parse the input values
			for (size_t j = 0; j < tokens.size(); ++j)
				rowVals.push_back(atof(tokens[j].c_str()));
			
			// Ensure the length of this row is the same as the previous row
			if (i > 0 && rowVals.size() != values.back().size())
				throw RunError("row length does not match on line " + ::toString(i+1));
			
			// Add this row to the value vector
			values.push_back(rowVals);			
		}
		
		// Get the number of rows and columns
		size_t numRows = values.size();
		size_t numCols = values.size()? values[0].size():0;
		
		// Create a matrix to store the output
		MatrixF64Obj* pOutput = new MatrixF64Obj(numRows, numCols);
		
		// For each row
		for (size_t r = 0; r < values.size(); ++r)
		{
			// For each column
			for (size_t c = 0; c < values.size(); ++c)
			{
				// Get this value
				float64 value = values[r][c];
				
				// Set the value in the output matrix
				pOutput->setElem2D(r+1, c+1, value);
			}	
		}		
		
		// Return the output matrix
		return new ArrayObj(pOutput);
	}
	
	/***************************************************************
	* Function: loadFuncTypeMapping()
	* Purpose : Type mapping for the "load" library function
	* Initial : Maxime Chevalier-Boisvert on May 15, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	TypeSetString loadFuncTypeMapping(const TypeSetString& argTypes)
	{
		// Return the type info for a 2D floating-point matrix
		return typeSetStrMake(TypeInfo(
			DataObject::MATRIX_F64,
			true,
			false,
			false,
			false,
			TypeInfo::DimVector(),
			NULL,
			TypeSet()
		));
	}
	
	/***************************************************************
	* Function: log2Func()
	* Purpose : Compute base 2 logarithms
	* Initial : Maxime Chevalier-Boisvert on February 18, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* log2Func(ArrayObj* pArguments)
	{
		// Ensure there is one argument
		if (pArguments->getSize() != 1)
			throw RunError("invalid argument count");
	
		// Get a pointer to the argument
		DataObject* pArgument = pArguments->getObject(0);
		
		// If the argument is a matrix
		if (pArgument->getType() == DataObject::MATRIX_F64)
		{
			// Get a typed pointer to the argument
			MatrixF64Obj* pInMatrix = (MatrixF64Obj*)pArgument;
			
			// Create a new matrix to store the output
			MatrixF64Obj* pOutMatrix = new MatrixF64Obj(pInMatrix->getSize());
			
			// Compute a pointer to the last element of the input matrix
			float64* pLastElem = pInMatrix->getElements() + pInMatrix->getNumElems();
			
			// For each element of the matrices
			for (float64 *pIn = pInMatrix->getElements(), *pOut = pOutMatrix->getElements(); pIn < pLastElem; ++pIn, ++pOut)
			{
				// If the value is negative
				if (*pIn < 0)
					throw RunError("logarithms of negative numbers unsupported");
							
				// Compute the square root of this element
				*pOut = ::log2(*pIn);
			}			
			
			// Return the output matrix
			return new ArrayObj(pOutMatrix);
		}
		
		// For all other argument types
		else
		{
			// Throw an exception
			throw RunError("unsupported argument type");
		}
	}
	
	/***************************************************************
	* Function: lsFunc()
	* Purpose : List directory contents
	* Initial : Maxime Chevalier-Boisvert on February 20, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* lsFunc(ArrayObj* pArguments)
	{
		// Declare a string for the command input
		std::string cmdInput;
		
		// For each input argument
		for (size_t i = 0; i < pArguments->getSize(); ++i)
		{
			// Get a reference to this argument
			DataObject* pArg = pArguments->getObject(i);
			
			// If this is not a string object, throw an exception
			if (pArg->getType() != DataObject::CHARARRAY)
				throw RunError("non-string argument provided");
			
			// Add the string to the command input
			cmdInput += " " + ((CharArrayObj*)pArg)->getString();
		}
		
		// Declare a string to store the command output
		std::string output;
		
		// Run the "ls" system command
		openPipe("ls" + cmdInput, &output);
		
		// Return the output of the command
		return new ArrayObj(new CharArrayObj(output));		
	}

	/***************************************************************
	* Function: maxFunc()
	* Purpose : Find the largest value in a vector
	* Initial : Maxime Chevalier-Boisvert on February 23, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* maxFunc(ArrayObj* pArguments)
	{
		// If there is one argument
		if (pArguments->getSize() == 1)
		{	
			// Get a pointer to the argument
			DataObject* pArgument = pArguments->getObject(0);
	
			// If the argument is a matrix
			if (pArgument->getType() == DataObject::MATRIX_F64)
			{
				// Get a typed pointer to the argument
				MatrixF64Obj* pInMatrix = (MatrixF64Obj*)pArgument;
	
				// If the input is empty or scalar, return a copy of it
				if (pInMatrix->isEmpty() || pInMatrix->isScalar())
					return new ArrayObj(pInMatrix->copy());
				
				// Get the size of the input matrix
				const DimVector& inSize = pInMatrix->getSize();
				
				// Declare variables to store the index and length of the first non-singular dimension
				size_t firstDim = 0;
				size_t firstDimLen = 0;
				
				// For each dimension
				for (size_t i = 0; i < inSize.size(); ++i)
				{
					// If this dimension is non-singular
					if (inSize[i] > 1)
					{
						// Store its index and length, then break out of this loop
						firstDim = i;
						firstDimLen = inSize[i];
						break;
					}
				}
				
				// Compute the size of the output matrix
				DimVector outSize = inSize;
				outSize[firstDim] = 1;			
				
				// Create a new matrix to store the output
				MatrixF64Obj* pOutMatrix = new MatrixF64Obj(outSize);
					
				// Compute a pointer past the last element of the input matrix
				float64* pLastElem = pInMatrix->getElements() + pInMatrix->getNumElems();
				
				// For each vector inside the input matrix
				for (float64 *pVec = pInMatrix->getElements(), *pOut = pOutMatrix->getElements(); pVec < pLastElem; pVec += firstDimLen, ++pOut)
				{
					// Initialize max value to negative infiity
					float64 max = -FLOAT_INFINITY;
					
					// Compute a pointer past the last element of this vector
					float64* pLastInVec = pVec + firstDimLen;
					
					// Find the max element in the vector
					for (float64* pIn = pVec; pIn < pLastInVec; ++pIn)
						max = std::max(max, *pIn);				
					
					// Store the max value in the output
					*pOut = max;
				}		
	
				// Return the output matrix
				return new ArrayObj(pOutMatrix);
			}
			
			// For all other argument types
			else
			{
				// Throw an exception
				throw RunError("unsupported argument type");
			}
		}
		
		// Otherwise, if there are two arguments
		else if (pArguments->getSize() == 2)
		{
			// Get a pointer to the arguments
			DataObject* pLArg = pArguments->getObject(0);
			DataObject* pRArg = pArguments->getObject(1);	
			
			// If the arguments are two floating-point matrices
			if (pLArg->getType() == DataObject::MATRIX_F64 || pRArg->getType() == DataObject::MATRIX_F64)
			{
				// Get typed pointers to the arguments
				MatrixF64Obj* pLMatrix = (MatrixF64Obj*)pLArg;
				MatrixF64Obj* pRMatrix = (MatrixF64Obj*)pRArg;

				// Apply the operation to obtain the result
				MatrixF64Obj* pOutMatrix = MatrixF64Obj::binArrayOp<MaxOp<float64>, float64>(pLMatrix, pRMatrix);
				
				// Return the output value
				return new ArrayObj(pOutMatrix);
			}
			
			// For all other argument types
			else
			{
				// Throw an exception
				throw RunError("unsupported argument type combination");
			}
		}
		
		// Otherwise, invalid argument cound
		else
		{
			// Throw an exception
			throw RunError("invalid argument count");
		}
	}
	
	/***************************************************************
	* Function: maxFuncTypeMapping()
	* Purpose : Type mapping for the "max" library function
	* Initial : Maxime Chevalier-Boisvert on May 15, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	TypeSetString maxFuncTypeMapping(const TypeSetString& argTypes)
	{
		// If there is one argument
		if (argTypes.size() == 1)
		{
			// This behaves as an integer-preserving vector operation
			return vectorOpTypeMapping<true>(argTypes);
		}
		
		// Otherwise, if there are two arguments
		else if (argTypes.size() == 2)
		{
			// This behaves as an integer-preserving array arithmetic operation
			return arrayArithOpTypeMapping<true>(argTypes);	
		}
		
		// Otherwise
		else
		{
			// Return no type information
			return TypeSetString();
		}		
	}
	
	/***************************************************************
	* Function: meanFunc()
	* Purpose : Compute means of vectors of numbers
	* Initial : Maxime Chevalier-Boisvert on February 2, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* meanFunc(ArrayObj* pArguments)
	{
		// Ensure there is one argument
		if (pArguments->getSize() != 1)
			throw RunError("invalid argument count");
	
		// Get a pointer to the argument
		DataObject* pArgument = pArguments->getObject(0);

		// If the argument is not a 64-bit float matrix, convert its type
		if (pArgument->getType() != DataObject::MATRIX_F64)
			pArgument = pArgument->convert(DataObject::MATRIX_F64);
			
		// Get a typed pointer to the argument
		MatrixF64Obj* pInMatrix = (MatrixF64Obj*)pArgument;

		// If the input is empty or scalar, return a copy of it
		if (pInMatrix->isEmpty() || pInMatrix->isScalar())
			return new ArrayObj(pInMatrix->copy());
		
		// Get the size of the input matrix
		const DimVector& inSize = pInMatrix->getSize();
		
		// Declare variables to store the index and length of the first non-singular dimension
		size_t firstDim = 0;
		size_t firstDimLen = 0;
		
		// For each dimension
		for (size_t i = 0; i < inSize.size(); ++i)
		{
			// If this dimension is non-singular
			if (inSize[i] > 1)
			{
				// Store its index and length, then break out of this loop
				firstDim = i;
				firstDimLen = inSize[i];
				break;
			}
		}
		
		// Compute the size of the output matrix
		DimVector outSize = inSize;
		outSize[firstDim] = 1;			
		
		// Create a new matrix to store the output
		MatrixF64Obj* pOutMatrix = new MatrixF64Obj(outSize);
			
		// Compute a pointer past the last element of the input matrix
		float64* pLastElem = pInMatrix->getElements() + pInMatrix->getNumElems();
		
		// For each vector inside the input matrix
		for (float64 *pVec = pInMatrix->getElements(), *pOut = pOutMatrix->getElements(); pVec < pLastElem; pVec += firstDimLen, ++pOut)
		{
			// Initialize the sum to 0
			float64 meanSum = 0;
			
			// Compute a pointer past the last element of this vector
			float64* pLastInVec = pVec + firstDimLen;
			
			// Add all vector elements to the mean sum
			for (float64* pIn = pVec; pIn < pLastInVec; ++pIn)
				meanSum += *pIn;				
			
			// Normalize the mean sum
			*pOut = meanSum / firstDimLen;
		}		

		// Return the output matrix
		return new ArrayObj(pOutMatrix);
	}

	/***************************************************************
	* Function: minFunc()
	* Purpose : Find the smallest value in a vector
	* Initial : Maxime Chevalier-Boisvert on February 18, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* minFunc(ArrayObj* pArguments)
	{
		// If there is one argument
		if (pArguments->getSize() == 1)
		{	
			// Get a pointer to the argument
			DataObject* pArgument = pArguments->getObject(0);
	
			// If the argument is a matrix
			if (pArgument->getType() == DataObject::MATRIX_F64)
			{
				// Get a typed pointer to the argument
				MatrixF64Obj* pInMatrix = (MatrixF64Obj*)pArgument;
	
				// If the input is empty or scalar, return a copy of it
				if (pInMatrix->isEmpty() || pInMatrix->isScalar())
					return new ArrayObj(pInMatrix->copy());
				
				// Get the size of the input matrix
				const DimVector& inSize = pInMatrix->getSize();
				
				// Declare variables to store the index and length of the first non-singular dimension
				size_t firstDim = 0;
				size_t firstDimLen = 0;
				
				// For each dimension
				for (size_t i = 0; i < inSize.size(); ++i)
				{
					// If this dimension is non-singular
					if (inSize[i] > 1)
					{
						// Store its index and length, then break out of this loop
						firstDim = i;
						firstDimLen = inSize[i];
						break;
					}
				}
				
				// Compute the size of the output matrix
				DimVector outSize = inSize;
				outSize[firstDim] = 1;			
				
				// Create a new matrix to store the output
				MatrixF64Obj* pOutMatrix = new MatrixF64Obj(outSize);
					
				// Compute a pointer past the last element of the input matrix
				float64* pLastElem = pInMatrix->getElements() + pInMatrix->getNumElems();
				
				// For each vector inside the input matrix
				for (float64 *pVec = pInMatrix->getElements(), *pOut = pOutMatrix->getElements(); pVec < pLastElem; pVec += firstDimLen, ++pOut)
				{
					// Initialize min value to infiity
					float64 min = FLOAT_INFINITY;
					
					// Compute a pointer past the last element of this vector
					float64* pLastInVec = pVec + firstDimLen;
					
					// Find the min element in the vector
					for (float64* pIn = pVec; pIn < pLastInVec; ++pIn)
						min = std::min(min, *pIn);				
					
					// Store the min value in the output
					*pOut = min;
				}		
	
				// Return the output matrix
				return new ArrayObj(pOutMatrix);
			}
			
			// For all other argument types
			else
			{
				// Throw an exception
				throw RunError("unsupported argument type");
			}	
		}
		
		// Otherwise, if there are two arguments
		else if (pArguments->getSize() == 2)
		{
			// Get a pointer to the arguments
			DataObject* pLArg = pArguments->getObject(0);
			DataObject* pRArg = pArguments->getObject(1);	
			
			// If the arguments are two floating-point matrices
			if (pLArg->getType() == DataObject::MATRIX_F64 || pRArg->getType() == DataObject::MATRIX_F64)
			{
				// Get typed pointers to the arguments
				MatrixF64Obj* pLMatrix = (MatrixF64Obj*)pLArg;
				MatrixF64Obj* pRMatrix = (MatrixF64Obj*)pRArg;

				// Apply the operation to obtain the result
				MatrixF64Obj* pOutMatrix = MatrixF64Obj::binArrayOp<MinOp<float64>, float64>(pLMatrix, pRMatrix);
				
				// Return the output value
				return new ArrayObj(pOutMatrix);
			}
			
			// For all other argument types
			else
			{
				// Throw an exception
				throw RunError("unsupported argument type combination");
			}
		}
		
		// Otherwise, invalid argument cound
		else
		{
			// Throw an exception
			throw RunError("invalid argument count");
		}
	}
	
	/***************************************************************
	* Function: modFunc()
	* Purpose : Compute modulo values
	* Initial : Maxime Chevalier-Boisvert on February 2, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* modFunc(ArrayObj* pArguments)
	{
		// Ensure there is one argument
		if (pArguments->getSize() != 2)
			throw RunError("invalid argument count");
	
		// Get a pointer to the arguments
		DataObject* pLArg = pArguments->getObject(0);
		DataObject* pRArg = pArguments->getObject(1);	
		
		// If the arguments are two floating-point matrices
		if (pLArg->getType() == DataObject::MATRIX_F64 || pRArg->getType() == DataObject::MATRIX_F64)
		{
			// Get typed pointers to the arguments
			MatrixF64Obj* pLMatrix = (MatrixF64Obj*)pLArg;
			MatrixF64Obj* pRMatrix = (MatrixF64Obj*)pRArg;
			
			// Apply the operation to obtain the result
			MatrixF64Obj* pOutMatrix = MatrixF64Obj::binArrayOp<ModOp<float64>, float64>(pLMatrix, pRMatrix);
			
			// Return the output value
			return new ArrayObj(pOutMatrix);	
		}
		
		// For all other argument types
		else
		{
			// Throw an exception
			throw RunError("unsupported argument type combination");
		}
	}
	
	/***************************************************************
	* Function: notFunc()
	* Purpose : Perform logical negation
	* Initial : Maxime Chevalier-Boisvert on March 4, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* notFunc(ArrayObj* pArguments)
	{
		// Ensure there is one argument
		if (pArguments->getSize() != 1)
			throw RunError("invalid argument count");
	
		// Get a pointer to the argument
		DataObject* pArgument = pArguments->getObject(0);
		
		// If the argument is a logical array
		if (pArgument->getType() == DataObject::LOGICALARRAY)
		{
			// Get a typed pointer to the argument
			LogicalArrayObj* pInMatrix = (LogicalArrayObj*)pArgument;
			
			// Apply the negation operator to the input matrix
			LogicalArrayObj* pOutMatrix = LogicalArrayObj::arrayOp<NotOp<bool>, bool>(pInMatrix);
			
			// Return the output matrix
			return new ArrayObj(pOutMatrix);
		}
		
		// If the argument is a matrix
		else if (pArgument->getType() == DataObject::MATRIX_F64)
		{
			// Get a typed pointer to the argument
			MatrixF64Obj* pInMatrix = (MatrixF64Obj*)pArgument;
			
			// Apply the negation operator to the input matrix
			LogicalArrayObj* pOutMatrix = MatrixF64Obj::arrayOp<NotOp<float64>, bool>(pInMatrix);
			
			// Return the output matrix
			return new ArrayObj(pOutMatrix);
		}
		
		// For all other argument types
		else
		{
			// Throw an exception
			throw RunError("unsupported argument type");
		}
	}
	
	/***************************************************************
	* Function: num2strFunc()
	* Purpose : Convert numerical values to strings
	* Initial : Maxime Chevalier-Boisvert on July 10, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* num2strFunc(ArrayObj* pArguments)
	{
		// Ensure there is one argument
		if (pArguments->getSize() != 1)
			throw RunError("invalid argument count");
	
		// Get a pointer to the argument
		DataObject* pArgument = pArguments->getObject(0);
		
		// If the argument is a numerical value
		if (pArgument->getType() == DataObject::MATRIX_F64 ||
			pArgument->getType() == DataObject::MATRIX_C128 ||
			pArgument->getType() == DataObject::LOGICALARRAY)
		{
			// Get the string value of the argument
			std::string strVal = pArgument->toString();
			
			// Return the string value
			return new ArrayObj(new CharArrayObj(strVal));
		}
		
		// For all other argument types
		else
		{
			// Throw an exception
			throw RunError("unsupported argument type");
		}
	}
	
	/***************************************************************
	* Function: notFuncTypeMapping()
	* Purpose : Type mapping for the "not" library function
	* Initial : Maxime Chevalier-Boisvert on May 13, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	TypeSetString notFuncTypeMapping(const TypeSetString& argTypes)
	{	
		// If we have no input type information
		if (argTypes.empty() || argTypes[0].empty())
		{
			// Return information about a generic logical array
			return typeSetStrMake(TypeInfo(
				DataObject::LOGICALARRAY,
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
				DataObject::LOGICALARRAY,
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
	* Function: numelFunc()
	* Purpose : Obtain the number of elements in a matrix
	* Initial : Maxime Chevalier-Boisvert on January 25, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* numelFunc(ArrayObj* pArguments)
	{
		// Ensure there is one argument
		if (pArguments->getSize() != 1)
			throw RunError("invalid argument count");
		
		// Get a pointer to the first argument
		DataObject* pObject = pArguments->getObject(0);
		
		// If the object is a matrix
		if (pObject->isMatrixObj())
		{
			// Get the number of matrix elements
			size_t numElements = ((BaseMatrixObj*)pObject)->getNumElems();
			
			// Return the number of matrix elements
			return new ArrayObj(new MatrixF64Obj(numElements));
		}
		else
		{
			// Return 1
			return new ArrayObj(new MatrixF64Obj(1));
		}
	}
	
	/***************************************************************
	* Function: onesFunc()
	* Purpose : Create and initialize a matrix
	* Initial : Maxime Chevalier-Boisvert on January 29, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* onesFunc(ArrayObj* pArguments)
	{
		// Create and initialize the matrix
		return createMatrix(pArguments, 1);
	}

	/***************************************************************
	* Function: piFunc()
	* Purpose : Return the constant pi
	* Initial : Maxime Chevalier-Boisvert on July 29, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* piFunc(ArrayObj* pArguments)
	{
		// Return the constant pi
		return new ArrayObj(new MatrixF64Obj(PI));
	}
	
	/*******************************************************************
	 *Expected
	 *plot(Y) 1Arg-> y(matrix m*n) 
	 *plot(X1,Y1) 2Arg-> y(matrix m*n), x(matrix o*n), where o|m == 1
	 *plot(x1,y1,opt_str) 3Arg-> y(matrix m*n), x(matrix o*n), "opt"(string), where o|m == 1
	 *******************************************************************/
	ArrayObj* plotFunc(ArrayObj* pArguments)
	{	

			

		Plotting plot_machina = Plotting(pArguments) ;
		
		
		//parsing and saving the arguments
		plot_machina.parsing();
		

		
		//save the option to a file
		plot_machina.printOpt();
		

		
		//save the matrice's data to a file
		plot_machina.printData();
		

		
		//call to gunplot
		plot_machina.callGnuplot();
		

		
		return new ArrayObj();
	
	}

	/******************************************************************
	 * Function: plotFunc()
	 * Purpose : plot a 2D function using gnuplot
	 * Initial : Olivier Savary on June 1, 2009
	 * Revision: unfinished, untested, unimplimented
	 ********************************************************************/
	TypeSetString plotFuncTypeMapping(const TypeSetString& argTypes)
	{
		
		if (argTypes.empty()){
			return TypeSetString();
		}else if (argTypes.size() == 1){
			//1 args->matrix of floating point
			return typeSetStrMake(TypeInfo(
						DataObject::MATRIX_F64,
						true,
						false,
						false,
						false,
						TypeInfo::DimVector(),
						NULL,
						TypeSet()
				));
		
		}else if(argTypes.size() == 2){
			//2 args->2 matrices of floating point
			return typeSetStrMake(TypeInfo(
								DataObject::MATRIX_F64,
								true,
								false,
								false,
								false,
								TypeInfo::DimVector(),
								NULL,
								TypeSet()
						));
		
		}else if(argTypes.size() == 3){
			return TypeSetString();
		}else{
			return TypeSetString();
		}
			
	}
	
	/***************************************************************
	* Function: pwdFunc()
	* Purpose : Get the current working directory
	* Initial : Maxime Chevalier-Boisvert on February 20, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* pwdFunc(ArrayObj* pArguments)
	{
		// Ensure there is are no arguments
		if (pArguments->getSize() != 0)
			throw RunError("too many arguments");
		
		// Get the current working directory path
		std::string workingDir = getWorkingDir();
		
		// Return the working directory path
		return new ArrayObj(new CharArrayObj(workingDir));
	}
	
	/***************************************************************
	* Function: randFunc()
	* Purpose : Generate uniform random numbers
	* Initial : Maxime Chevalier-Boisvert on February 18, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* randFunc(ArrayObj* pArguments)
	{
		// Parse the matrix size from the input arguments
		DimVector matSize = parseMatSize(pArguments);
		
		// Create the output matrix
		MatrixF64Obj* pMatrix = new MatrixF64Obj(matSize);

		// Compute a pointer to the last element of the matrix
		float64* pLastElem = pMatrix->getElements() + pMatrix->getNumElems();
		
		// For each element of the matrix
		for (float64 *pVal = pMatrix->getElements(); pVal < pLastElem; ++pVal)
		{
			// Generate a pseudo-random number in the range [0,1]
			*pVal = double(::rand()) / double(RAND_MAX);
		}	
		
		// Return the output matrix
		return new ArrayObj(pMatrix);
	}
	
	/***************************************************************
	* Function: reshapeFunc()
	* Purpose : Change the shape of matrices
	* Initial : Maxime Chevalier-Boisvert on April 13, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* reshapeFunc(ArrayObj* pArguments)
	{
		// Ensure that there are enough arguments
		if (pArguments->getSize() < 2)
			throw RunError("insufficient argument count");
		
		// Ensure that the first argument is a matrix
		if (pArguments->getObject(0)->isMatrixObj() == false)
			throw RunError("expected matrix as first argument");
		
		// Get a typed pointer to the first argument
		BaseMatrixObj* pSrcMatrix = (BaseMatrixObj*)pArguments->getObject(0);
		
		// Create a vector for the desired output size
		DimVector dstSize;
		dstSize.reserve(pArguments->getSize() - 1);
		
		// Declare a value for the missing entry index (if any)
		int missIndex = -1;
		
		// If there is only one size argument and it is a matrix
		if (pArguments->getSize() == 2 && pArguments->getObject(1)->isMatrixObj())
		{
			// Get a pointer to the size argument
			DataObject* pSizeArg = pArguments->getObject(1);
			
			// Convert the size matrix to a 64-bit floating point matrix
			MatrixF64Obj* pSizeMatrix;
			if (pSizeArg->getType() != DataObject::MATRIX_F64)
				pSizeMatrix = (MatrixF64Obj*)pSizeArg->convert(DataObject::MATRIX_F64);
			else
				pSizeMatrix = (MatrixF64Obj*)pSizeArg;
			
			// If the size matrix is empty
			if (pSizeMatrix->isEmpty())
			{
				// Mark the missing index as 0
				missIndex = 0;
				
				// Add the value 1 to the dst size vector
				dstSize.push_back(1);
			}
			else
			{
				// For each element of the size matrix
				for (size_t i = 1; i <= pSizeMatrix->getNumElems(); ++i)
				{
					// Get the value of this element
					float64 value = pSizeMatrix->getElem1D(i);
					
					// Cast the value into an integer
					size_t intValue = size_t(value);
					
					// Ensure that the value is a positive integer
					if (value < 0 || value - intValue != 0)
						throw RunError("invalid dimension size");
					
					// Add the value to the dst size vector
					dstSize.push_back(intValue);
				}
			}
		}
		else
		{
			// For each remaining argument
			for (size_t i = 1; i < pArguments->getSize(); ++i)
			{
				// Get a pointer to the argument
				DataObject* pArg = pArguments->getObject(i);
				
				// Ensure that the argument is a matrix
				if (pArg->isMatrixObj() == false)
					throw RunError("expected matrix argument");
				
				// Get a typed pointer to the argument
				BaseMatrixObj* pArgMatrix = (BaseMatrixObj*)pArg;
				
				// If this is an empty matrix
				if (pArgMatrix->isEmpty())
				{
					// Ensure that there was not another missing size entry
					if (missIndex != -1)
						throw RunError("there can be only one missing size entry");
					
					// Set the missing entry index
					missIndex = i - 1;
					
					// Add a size of 1 for this dimension (temporarily)
					dstSize.push_back(1);					
				}
				else
				{
					// Convert the argument to a positive integer and add it to the size vector
					dstSize.push_back(getIndexValue(pArg));
				}				
			}			
		}
		
		// Compute the number of elements in the dst matrix
		size_t dstElemCount = 1;
		for (size_t i = 0; i < dstSize.size(); ++i)
			dstElemCount *= dstSize[i];
		
		// If there is a missing index
		if (missIndex != -1)
		{
			// Ensure that the current dst elem count divides the src elem count
			if (pSrcMatrix->getNumElems() % dstElemCount != 0)
				throw RunError("invalid size values");
			
			// Compute the missing index value
			dstSize[missIndex] = pSrcMatrix->getNumElems() / dstElemCount;
		}
		else
		{
			// Ensure that the number of elements in the output matches the input
			if (dstElemCount != pSrcMatrix->getNumElems())
				throw RunError("output element count does not match input");
		}		
		
		// If the input matrix is a 64-bit floating-point matrix
		if (pSrcMatrix->getType() == DataObject::MATRIX_F64)
		{
			// Get a typed pointer to the input matrix
			MatrixF64Obj* pInMatrix = (MatrixF64Obj*)pSrcMatrix;
			
			// Create a matrix to store the output
			MatrixF64Obj* pOutMatrix = new MatrixF64Obj(dstSize);
			
			// Copy the elements from the input to the output matrix
			memcpy(
				pOutMatrix->getElements(), 
				pInMatrix->getElements(), 
				pInMatrix->getNumElems() * sizeof(float64)
			);
			
			// Return the output matrix
			return new ArrayObj(pOutMatrix);
		}
		
		// For all other argument types
		else
		{
			// Throw an exception
			throw RunError("unsupported argument type");
		}
	}
	
	/***************************************************************
	* Function: reshapeFuncTypeMapping()
	* Purpose : Type mapping for the "reshape" library function
	* Initial : Maxime Chevalier-Boisvert on May 15, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	TypeSetString reshapeFuncTypeMapping(const TypeSetString& argTypes)
	{
		// If there are no arguments, return no information
		if (argTypes.empty())
			return TypeSetString();
		
		// Get a reference to the matrix argument type set
		const TypeSet& argSet1 = argTypes[0];
		
		// Declare a flag to indicate whether the output will be 2D
		bool is2D = true;
		
		// If there are more than 2 size arguments
		if (argTypes.size() > 3)
		{
			// Output is not 2D
			is2D = false;
		}
		
		// Otherwise, if there is only 1 size argument
		else if (argTypes.size() == 1)
		{
			// Get references to the size argument type set
			const TypeSet& argSet2 = argTypes[1];
			
			// If the set is empty, cannot guarantee output is 2D
			if (argSet2.empty())
				is2D = false;
			
			// For each possible size argument
			for (TypeSet::const_iterator type1 = argSet1.begin(); type1 != argSet1.end(); ++type1)
			{
				// If the matrix size is unknown
				if (type1->getSizeKnown() == false)
				{
					// Cannot guarantee output is 2D
					is2D = false;
				}
				else
				{
					// Compute the number of matrix elements
					const TypeInfo::DimVector& matSize = type1->getMatSize();
					size_t numElems = 1;
					for (size_t i = 0; i < matSize.size(); ++i)
						numElems *= matSize[i];
					
					// If there are more than 2 values, output is not 2D
					if (numElems > 2)
						is2D = false;
				}
			}			
		}		
		
		// Create a set to store the possible output types
		TypeSet outSet; 
		
		// For each possible matrix argument
		for (TypeSet::const_iterator type1 = argSet1.begin(); type1 != argSet1.end(); ++type1)
		{
			// Add the resulting type to the output set
			outSet.insert(TypeInfo(
				type1->getObjType(),
				is2D,
				false,
				type1->isInteger(),
				false,
				TypeInfo::DimVector(),
				NULL,
				TypeSet()
			));	
		}
		
		// Return the possible output types
		return TypeSetString(1, outSet);
	}
	
	/***************************************************************
	* Function: roundFunc()
	* Purpose : Round numerical values
	* Initial : Maxime Chevalier-Boisvert on February 2, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* roundFunc(ArrayObj* pArguments)
	{
		// Ensure there is one argument
		if (pArguments->getSize() != 1)
			throw RunError("invalid argument count");
	
		// Get a pointer to the argument
		DataObject* pArgument = pArguments->getObject(0);
		
		// If the argument is a floating-point matrix
		if (pArgument->getType() == DataObject::MATRIX_F64)
		{
			// Get a typed pointer to the argument
			MatrixF64Obj* pInMatrix = (MatrixF64Obj*)pArgument;
			
			// Create a matrix to store the output
			MatrixF64Obj* pOutMatrix = new MatrixF64Obj(pInMatrix->getSize());
			
			// Compute a pointer past the last element of the input matrix
			float64* pLastElem = pInMatrix->getElements() + pInMatrix->getNumElems();
			
			// For each element inside the input matrix
			for (float64 *pIn = pInMatrix->getElements(), *pOut = pOutMatrix->getElements(); pIn < pLastElem; ++pIn, ++pOut)
			{
				// Round the value
				*pOut = ::round(*pIn);
			}
			
			// Return the rounded output
			return new ArrayObj(pOutMatrix);
		}
		
		// For all other argument types
		else
		{
			// Throw an exception
			throw RunError("unsupported argument type");
		}
	}
	
	/***************************************************************
	* Function: signFunc()
	* Purpose : Compute the sign of numbers
	* Initial : Maxime Chevalier-Boisvert on March 1, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* signFunc(ArrayObj* pArguments)
	{
		// Ensure there is one argument
		if (pArguments->getSize() != 1)
			throw RunError("invalid argument count");
	
		// Get a pointer to the argument
		DataObject* pArgument = pArguments->getObject(0);
		
		// If the argument is a matrix
		if (pArgument->getType() == DataObject::MATRIX_F64)
		{
			// Get a typed pointer to the argument
			MatrixF64Obj* pInMatrix = (MatrixF64Obj*)pArgument;
			
			// Apply the sign operator to the input matrix
			MatrixF64Obj* pOutMatrix = MatrixF64Obj::arrayOp<SignOp<float64>, float64>(pInMatrix);
			
			// Return the output matrix
			return new ArrayObj(pOutMatrix);
		}
		
		// For all other argument types
		else
		{
			// Throw an exception
			throw RunError("unsupported argument type");
		}
	}
	
	/***************************************************************
	* Function: sinFunc()
	* Purpose : Compute the sine of numbers
	* Initial : Maxime Chevalier-Boisvert on February 18, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* sinFunc(ArrayObj* pArguments)
	{
		// Ensure there is one argument
		if (pArguments->getSize() != 1)
			throw RunError("invalid argument count");
	
		// Get a pointer to the argument
		DataObject* pArgument = pArguments->getObject(0);
		
		// If the argument is a matrix
		if (pArgument->getType() == DataObject::MATRIX_F64)
		{
			// Get a typed pointer to the argument
			MatrixF64Obj* pInMatrix = (MatrixF64Obj*)pArgument;
			
			// Create a new matrix to store the output
			MatrixF64Obj* pOutMatrix = new MatrixF64Obj(pInMatrix->getSize());
			
			// Compute a pointer to the last element of the input matrix
			float64* pLastElem = pInMatrix->getElements() + pInMatrix->getNumElems();
			
			// For each element of the matrices
			for (float64 *pIn = pInMatrix->getElements(), *pOut = pOutMatrix->getElements(); pIn < pLastElem; ++pIn, ++pOut)
			{
				// Compute the sine of this element
				*pOut = ::sin(*pIn);
			}			
			
			// Return the output matrix
			return new ArrayObj(pOutMatrix);
		}
		
		// For all other argument types
		else
		{
			// Throw an exception
			throw RunError("unsupported argument type");
		}
	}
	
	/***************************************************************
	* Function: sizeFunc()
	* Purpose : Obtain the size of matrices
	* Initial : Maxime Chevalier-Boisvert on January 25, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* sizeFunc(ArrayObj* pArguments)
	{
		// Ensure there is at least 1 argument
		if (pArguments->getSize() < 1)
			throw RunError("insufficient argument count");
		
		// Get a pointer to the first argument
		DataObject* pObject = pArguments->getObject(0);
		
		// Declare a vector to store the matrix size
		DimVector sizeVector;
		
		// If the object is a matrix
		if (pObject->isMatrixObj())
		{
			// Get the size vector of the matrix
			sizeVector = ((BaseMatrixObj*)pObject)->getSize();
		}
		else
		{
			// Set the size to 1x1
			sizeVector.resize(2,1);
		}
		
		// If there is another argument
		if (pArguments->getSize() > 1)
		{
			// If there are too many arguments
			if (pArguments->getSize() > 2)
				throw RunError("too many arguments");
			
			// Get a pointer to the second argument
			DataObject* pDimArg = pArguments->getObject(1);
			
			// Get the value of the dimension argument
			size_t dimArg = toZeroIndex(getIndexValue(pDimArg));
				
			// Get the dimension size
			size_t dimSize = (dimArg < sizeVector.size())? sizeVector[dimArg]:1;
				
			// Return the dimension size
			return new ArrayObj(new MatrixF64Obj(dimSize));
		}
		
		// Create a matrix to store the size information
		MatrixF64Obj* pSizeMatrix = new MatrixF64Obj(1, sizeVector.size());
		
		// Set this element of the size matrix
		for (size_t i = 0; i < sizeVector.size(); ++i)
			pSizeMatrix->setElem2D(1, toOneIndex(i), sizeVector[i]);
		
		// Return the size matrix
		return new ArrayObj(pSizeMatrix);
	}
	
	/***************************************************************
	* Function: sizeFuncTypeMapping()
	* Purpose : Type mapping for the size function
	* Initial : Maxime Chevalier-Boisvert on May 7, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	TypeSetString sizeFuncTypeMapping(const TypeSetString& argTypes)
	{
		// If there is not one argument, return no information
		if (argTypes.size() != 1)
			return TypeSetString();
		
		// If there are two input arguments
		if (argTypes.size() == 2)
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
		
		// Get a reference to the argument type set
		const TypeSet& argSet1 = argTypes[0];
		
		// Create a set to store the possible output types
		TypeSet outSet; 
		
		// For each possible input type combination
		for (TypeSet::const_iterator type1 = argSet1.begin(); type1 != argSet1.end(); ++type1)
		{
			// Test if the output size is known
			bool sizeKnown = type1->getSizeKnown();
			
			// Compute the size of the output matrix, if possible
			TypeInfo::DimVector matSize;
			if (sizeKnown)
			{				
				matSize.push_back(1);
				matSize.push_back(type1->getMatSize().size());
			}
			
			// Add the resulting type to the output set
			outSet.insert(TypeInfo(
				DataObject::MATRIX_F64,
				true,
				false,
				true,
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
	* Function: sortFunc()
	* Purpose : Sort vectors of numbers
	* Initial : Maxime Chevalier-Boisvert on March 5, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* sortFunc(ArrayObj* pArguments)
	{
		// Define sorting value and vector types
		typedef std::pair<float64, size_t> SortVal;
		typedef std::vector<SortVal> SortVec;
		
		// Define a comparison function for sorting values
	    class CompFunc
	    {
	    	bool operator() (const SortVal& A, const SortVal& B)
	    	{ return A.first < B.first;	}
	    };
		
		// Ensure there is one argument
		if (pArguments->getSize() != 1)
			throw RunError("invalid argument count");
	
		// Get a pointer to the argument
		DataObject* pArgument = pArguments->getObject(0);
		
		// If the argument is a matrix
		if (pArgument->getType() == DataObject::MATRIX_F64)
		{
			// Get a typed pointer to the argument
			MatrixF64Obj* pInMatrix = (MatrixF64Obj*)pArgument;

			// If the input is empty return a copy of it
			if (pInMatrix->isEmpty())
				return new ArrayObj(pInMatrix->copy(), pInMatrix->copy()); 
			
			// If the input matrix is not a vector, throw an exception
			if (pInMatrix->isVector() == false)
				throw RunError("only vector matrices are supported");
			
			// Create a vector for the sorting elements
			SortVec vector;
			vector.reserve(pInMatrix->getNumElems());
			
			// Add the input elements to the sorting vector
			for (size_t i = 1; i <= pInMatrix->getNumElems(); ++i)
				vector.push_back(SortVal(pInMatrix->getElem1D(i), i));

			// Sort the values
			std::sort(vector.begin(), vector.end());
			
			// Create matrices to srote the output values and the output indices
			MatrixF64Obj* pOutMatrix = pInMatrix->copy();
			MatrixF64Obj* pIndMatrix = pInMatrix->copy();
			
			// Copy the sorted values and output indices to the output matrices
			for (size_t i = 1; i <= pInMatrix->getNumElems(); ++i)
			{
				pOutMatrix->setElem1D(i, vector[i-1].first);
				pIndMatrix->setElem1D(i, vector[i-1].second);
			}			
			
			// Return the output and index matrices
			return new ArrayObj(pOutMatrix, pIndMatrix);	
		}
		
		// For all other argument types
		else
		{
			// Throw an exception
			throw RunError("unsupported argument type");
		}
	}
	
	/***************************************************************
	* Function: sortFuncTypeMapping()
	* Purpose : Type mapping for the "sort" library function
	* Initial : Maxime Chevalier-Boisvert on May 15, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	TypeSetString sortFuncTypeMapping(const TypeSetString& argTypes)
	{
		// If there are no arguments, return no information
		if (argTypes.empty())
			return TypeSetString();
		
		// Get references to the argument type sets
		const TypeSet& argSet1 = argTypes[0];
		
		// Create type sets to store the sorted matrix and index vector types
		TypeSet sortedTypeSet;
		TypeSet indexTypeSet;
		
		// For each possible input type combination
		for (TypeSet::const_iterator type1 = argSet1.begin(); type1 != argSet1.end(); ++type1)
		{
			// The sorted vector has the same type as the input vector
			sortedTypeSet.insert(*type1);
			
			// Determine the type of the sorted vector
			indexTypeSet.insert(TypeInfo(
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
		
		// Create a type set string to store the output types
		TypeSetString outTypeStr;
		
		// Add the sorted matrix and index vector types
		outTypeStr.push_back(sortedTypeSet);
		outTypeStr.push_back(indexTypeSet);
		
		// Return the output type string
		return outTypeStr;
	}
	
	/***************************************************************
	* Function: sprintfFunc()
	* Purpose : Format text strings
	* Initial : Maxime Chevalier-Boisvert on February 20, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* sprintfFunc(ArrayObj* pArguments)
	{
		// Perform formatted printing
		std::string outText = formatPrint(pArguments);	
		
		// Return the formatted string
		return new ArrayObj(new CharArrayObj(outText));
	}

	/***************************************************************
	* Function: squeezeFunc()
	* Purpose : Eliminate singleton dimensions in matrices
	* Initial : Maxime Chevalier-Boisvert on March 5, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* squeezeFunc(ArrayObj* pArguments)
	{
		// Ensure there is one argument
		if (pArguments->getSize() != 1)
			throw RunError("invalid argument count");
	
		// Get a pointer to the argument
		DataObject* pArgument = pArguments->getObject(0);
		
		// If the argument is a matrix
		if (pArgument->getType() == DataObject::MATRIX_F64)
		{
			// Get a typed pointer to the argument
			MatrixF64Obj* pInMatrix = (MatrixF64Obj*)pArgument;
			
			// Get the input matrix size
			const DimVector& inSize = pInMatrix->getSize();
		
			// Create a vector for the output matrix size
			DimVector outSize;
		
			// Add all nonzero dimensions to the output size
			for (size_t i = 0; i < inSize.size(); ++i)
				if (inSize[i] != 1) outSize.push_back(inSize[i]);
			
			// If the output size vector is empty, add ones to have at least two dimensions
			if (outSize.size() < 2)
				outSize.resize(2, 1);
			
			// Create a new matrix to store the output
			MatrixF64Obj* pOutMatrix = new MatrixF64Obj(outSize);
			
			// Copy the data from the input matrix to the output matrix
			memcpy(pOutMatrix->getElements(), pInMatrix->getElements(), pInMatrix->getNumElems() * sizeof(float64));
			
			// Return the output matrix
			return new ArrayObj(pOutMatrix);
		}
		
		// For all other argument types
		else
		{
			// Throw an exception
			throw RunError("unsupported argument type");
		}
	}
	
	/***************************************************************
	* Function: squeezeFuncTypeMapping()
	* Purpose : Type mapping for the "squeeze" function
	* Initial : Maxime Chevalier-Boisvert on May 15, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	TypeSetString squeezeFuncTypeMapping(const TypeSetString& argTypes)
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
			// Get the matrix size
			const TypeInfo::DimVector& matSize = type1->getMatSize();
			
			// Store all non-singular dimensions
			TypeInfo::DimVector outSize;
			for (size_t i = 0; i < matSize.size(); ++i)
				if (matSize[i] != 1) outSize.push_back(matSize[i]);
			
			// If the output size vector is empty, add ones to have at least two dimensions
			if (outSize.size() < 2)
				outSize.resize(2, 1);
			
			// Add the resulting type to the output set
			outSet.insert(TypeInfo(
				type1->getObjType(),
				type1->getSizeKnown() && outSize.size() == 2,
				type1->getSizeKnown() && outSize.size() == 2 && outSize[0] == 1 && outSize[1] == 1,
				type1->isInteger(),
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
	* Function: sqrtFunc()
	* Purpose : Compute square roots
	* Initial : Maxime Chevalier-Boisvert on February 2, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* sqrtFunc(ArrayObj* pArguments)
	{
		// Ensure there is one argument
		if (pArguments->getSize() != 1)
			throw RunError("invalid argument count");
	
		// Get a pointer to the argument
		DataObject* pArgument = pArguments->getObject(0);
		
		// If the argument is a 128-bit complex matrix
		if (pArgument->getType() == DataObject::MATRIX_C128)
		{
			// Get a typed pointer to the argument
			MatrixC128Obj* pInMatrix = (MatrixC128Obj*)pArgument;
			
			// Apply the operator to obtain the output
			MatrixC128Obj* pOutMatrix = MatrixC128Obj::arrayOp<SqrtOp<Complex128>, Complex128>(pInMatrix);
			
			// Return the output matrix
			return new ArrayObj(pOutMatrix);
		}
		
		// Convert the argument into a 64-bit floating-point matrix, if necessary
		if (pArgument->getType() != DataObject::MATRIX_F64)
			pArgument = pArgument->convert(DataObject::MATRIX_F64);
			
		// Get a typed pointer to the argument
		MatrixF64Obj* pInMatrix = (MatrixF64Obj*)pArgument;
			
		// Apply the operator to obtain the output
		MatrixF64Obj* pOutMatrix = MatrixF64Obj::arrayOp<SqrtOp<float64>, float64>(pInMatrix);
		
		// Return the output matrix
		return new ArrayObj(pOutMatrix);	
	}
	
	/***************************************************************
	* Function: strcatFunc()
	* Purpose : Perform string concatenation
	* Initial : Maxime Chevalier-Boisvert on March 16, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* strcatFunc(ArrayObj* pArguments)
	{
		// Ensure there is at least one argument
		if (pArguments->getSize() < 1)
			throw RunError("insufficient argument count");
	
		// Ensure the first is a character array
		if (pArguments->getObject(0)->getType() != DataObject::CHARARRAY)
			throw RunError("expected string argument");	
		
		// Get a pointer to the first string
		CharArrayObj* pOutString = (CharArrayObj*)pArguments->getObject(0);
		
		// For each remaining string
		for (size_t i = 1; i < pArguments->getSize(); ++i)
		{
			// Get a pointer to the current string
			CharArrayObj* pCurString = (CharArrayObj*)pArguments->getObject(i);
			
			// Concatenate this string with the previous ones
			pOutString = (CharArrayObj*)CharArrayObj::concat(pOutString, pCurString, 1);
		}		
		
		// Return the output string
		return new ArrayObj(pOutString);
	}
	
	/***************************************************************
	* Function: strcmpFunc()
	* Purpose : Perform string equality comparison
	* Initial : Maxime Chevalier-Boisvert on July 11, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* strcmpFunc(ArrayObj* pArguments)
	{
		// Ensure there are two arguments
		if (pArguments->getSize() != 2)
			throw RunError("invalid argument count");
		
		// Extract the firat and second arguments
		DataObject* pArg0 = pArguments->getObject(0);
		DataObject* pArg1 = pArguments->getObject(1);
		
		// Ensure both arguments are strings
		if (pArg0->getType() != DataObject::CHARARRAY || pArg1->getType() != DataObject::CHARARRAY)
			throw RunError("invalid input argument types");
		
		// Get the string values of both arguments
		std::string str1 = ((CharArrayObj*)pArg0)->getString();
		std::string str2 = ((CharArrayObj*)pArg1)->getString();
		
		// Perform equality comparison between the strings
		bool strEq = (str1 == str2);
		
		// Return the boolean value
		return new ArrayObj(new LogicalArrayObj(strEq));
	}
	
	/***************************************************************
	* Function: strcatFuncTypeMapping()
	* Purpose : Type mapping for the "strcat" library function
	* Initial : Maxime Chevalier-Boisvert on May 14, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	TypeSetString strcatFuncTypeMapping(const TypeSetString& argTypes)
	{
		// Create a vector to store the output size
		TypeInfo::DimVector outSize(2, 0);
		
		// Flag to indicate whether the output size is known
		bool sizeKnown = true;
		
		// For each argument
		for (size_t i = 0; i < argTypes.size(); ++i)
		{
			// Get references to the argument type set
			const TypeSet& typeSet = argTypes[i];
			
			// If the type set is empty
			if (typeSet.empty())
			{
				// Set the flags to pessimistic values
				sizeKnown = false;
				
				// Move to the next argument
				continue;
			}
				
			// Variables for the number of rows and columns of the argument
			size_t numRows = 0;
			size_t numCols = 0;
			
			// For each possible input type combination
			for (TypeSet::const_iterator type = typeSet.begin(); type != typeSet.end(); ++type)
			{
				// Get the matrix size
				const TypeInfo::DimVector matSize = type->getMatSize();
				
				// If the size is not known or the matrix is not 2D
				if (type->getSizeKnown() == false || matSize.size() != 2)
				{
					// Cannot know the output size
					sizeKnown = false;
				}
				else
				{
					// If this is not the first type in the set
					if (type != typeSet.begin())
					{
						// If other arguments have different sizes, the output size is unknown
						if (matSize[0] != numRows || matSize[1] != numCols)
							sizeKnown = false;
						
						// Store the matrix size
						numRows = matSize[0];
						numCols = matSize[1];
					}
				}
			}
			
			// If the number of rows do not match previous arguments, the output size is unknown
			if (i != 0 && outSize[0] != numRows)
				sizeKnown = false;
			
			// Update the output matrix size
			outSize[0] = numRows;
			outSize[1] += numCols;	
		}
		
		// Return the type info for the output matrix
		return typeSetStrMake(TypeInfo(
			DataObject::CHARARRAY,
			true,
			sizeKnown && outSize[0] == 1 && outSize[1] == 1,
			true,
			sizeKnown,
			outSize,
			NULL,
			TypeSet()
		));
	}
	
	/***************************************************************
	* Function: sumFunc()
	* Purpose : Compute sums of array elements
	* Initial : Maxime Chevalier-Boisvert on February 20, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* sumFunc(ArrayObj* pArguments)
	{
		// Parse the input arguments
		size_t opDim;
		BaseMatrixObj* pMatrixArg = parseVectorArgs(pArguments, opDim);
		
		// If the argument is a 128-bit complex matrix
		if (pMatrixArg->getType() == DataObject::MATRIX_C128)
		{
			// Get a typed pointer to the argument
			MatrixC128Obj* pInMatrix = (MatrixC128Obj*)pMatrixArg;
			
			// Apply the vector operator to obtain the output
			MatrixC128Obj* pOutMatrix = MatrixC128Obj::vectorOp<SumOp<Complex128>, Complex128>(pInMatrix, opDim);
		
			// Return the output matrix
			return new ArrayObj(pOutMatrix);
		}
		
		// If the argument is not a 64-bit float matrix, convert its type
		if (pMatrixArg->getType() != DataObject::MATRIX_F64)
			pMatrixArg = (BaseMatrixObj*)pMatrixArg->convert(DataObject::MATRIX_F64);
					
		// Get a typed pointer to the argument
		MatrixF64Obj* pInMatrix = (MatrixF64Obj*)pMatrixArg;
		
		// Perform the vector operation to get the result
		MatrixF64Obj* pOutMatrix = MatrixF64Obj::vectorOp<SumOp<float64>, float64>(pInMatrix, opDim);

		// Return the output matrix
		return new ArrayObj(pOutMatrix);
	}
	
	/***************************************************************
	* Function: systemFunc()
	* Purpose : Execute a system command
	* Initial : Maxime Chevalier-Boisvert on July 24, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* systemFunc(ArrayObj* pArguments)
	{
		// Ensure there is one argument
		if (pArguments->getSize() != 1)
			throw RunError("invalid argument count");
	
		// Ensure the argument is a character array
		if (pArguments->getObject(0)->getType() != DataObject::CHARARRAY)
			throw RunError("expected string argument");
		
		// Get the command string
		std::string cmdString = ((CharArrayObj*)pArguments->getObject(0))->getString();
		
		// Declare a string to store the command output
		std::string output;
		
		// Run the command string as a system command and store the output
		bool result = openPipe(cmdString, &output);
		
		// Create an array object for the output values
		ArrayObj* pOutVals = new ArrayObj();
		
		// Add the status value
		ArrayObj::addObject(pOutVals, new MatrixF64Obj(result? 0:1));
		
		// Add the command output
		ArrayObj::addObject(pOutVals, new CharArrayObj(output));
		
		// Return the output values
		return pOutVals;	
	}
	
	/***************************************************************
	* Function: systemFuncTypeMapping()
	* Purpose : Type mapping for the "system" library function
	* Initial : Maxime Chevalier-Boisvert on July 24 , 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	TypeSetString systemFuncTypeMapping(const TypeSetString& argTypes)
	{
		// Declare a type set string for the output types
		TypeSetString outTypes;
			
		// Add the type info for a scalar integer value
		TypeSet intType;
		intType.insert(TypeInfo(
			DataObject::MATRIX_F64,
			true,
			true,
			true,
			true,
			TypeInfo::DimVector(2,1),
			NULL,
			TypeSet()
		));
		outTypes.push_back(intType);
		
		// Add the type info for a string value
		TypeSet strType;
		intType.insert(TypeInfo(
			DataObject::CHARARRAY,
			true,
			false,
			true,
			false,
			TypeInfo::DimVector(),
			NULL,
			TypeSet()
		));
		outTypes.push_back(strType);
		
		// Return the output types
		return outTypes;
	}
	
	/***************************************************************
	* Function: ticFunc()
	* Purpose : Begin timing measurements
	* Initial : Maxime Chevalier-Boisvert on February 26, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* ticFunc(ArrayObj* pArguments)
	{		
		// Create a timeval struct to store the time info
		struct timeval timeVal;
		
		// Get the current time
		gettimeofday(&timeVal, NULL);

		// Compute the time in seconds using microsecond information
		double timeSecs = timeVal.tv_sec + timeVal.tv_usec * 1.0e-6;
		
		// Set the timer start time
		ticTocStartTime = timeSecs;
		
		// Return nothing
		return new ArrayObj();
	}
	
	/***************************************************************
	* Function: tocFunc()
	* Purpose : Stop timing measurements
	* Initial : Maxime Chevalier-Boisvert on February 26, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* tocFunc(ArrayObj* pArguments)
	{
		// If the start time was not set
		if (ticTocStartTime == FLOAT_INFINITY)
		{
			// Throw an exception
			throw RunError("timer start time not set");
		}
		
		// Create a timeval struct to store the time info
		struct timeval timeVal;
		
		// Get the current time
		gettimeofday(&timeVal, NULL);

		// Compute the time in seconds using microsecond information
		double timeSecs = timeVal.tv_sec + timeVal.tv_usec * 1.0e-6;
		
		// Compute the time difference
		double deltaT = timeSecs - ticTocStartTime;
		
		// Return the time difference value
		return new ArrayObj(new MatrixF64Obj(deltaT));
	}
	
	/***************************************************************
	* Function: toeplitzFunc()
	* Purpose : Create a toeplitz matrix
	* Initial : Maxime Chevalier-Boisvert on March 2, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* toeplitzFunc(ArrayObj* pArguments)
	{
		// Declare pointers for the column and row parameters
		DataObject* pColObject;
		DataObject* pRowObject;
		
		// If there are two arguments
		if (pArguments->getSize() == 2)
		{
			// Extract the column and row parameters
			pColObject = pArguments->getObject(0);
			pRowObject = pArguments->getObject(1);
		}
		
		// Otherwise, if there is 1 argument
		else if (pArguments->getSize() == 1)
		{
			// The column and row arguments are the same
			pColObject = pArguments->getObject(0);
			pRowObject = pArguments->getObject(0);
		}
		
		// Otherwise, the argument count is invalid
		else
		{
			// Throw an exception
			throw RunError("invalid argument count");
		}
		
		// If both arguments are 64-bit floating point matrices
		if (pColObject->getType() == DataObject::MATRIX_F64 && pRowObject->getType() == DataObject::MATRIX_F64)
		{
			// Get typed pointers to the arguments
			MatrixF64Obj* pColMatrix = (MatrixF64Obj*)pColObject;
			MatrixF64Obj* pRowMatrix = (MatrixF64Obj*)pRowObject;
			
			// Get the size of the output matrix
			size_t numRows = pColMatrix->getNumElems();
			size_t numCols = pRowMatrix->getNumElems();
			
			// Create the output matrix
			MatrixF64Obj* pOutMatrix = new MatrixF64Obj(numRows, numCols); 
			
			// Set the elements of the first column
			for (size_t rowIndex = 1; rowIndex <= numRows; ++rowIndex)
				pOutMatrix->setElem2D(rowIndex, 1, pColMatrix->getElem1D(rowIndex));
			
			// Set the elements of the first row
			for (size_t colIndex = 2; colIndex <= numCols; ++colIndex)
				pOutMatrix->setElem2D(1, colIndex, pRowMatrix->getElem1D(colIndex));
			
			// For each row
			for (size_t rowIndex = 2; rowIndex <= numRows; ++rowIndex)
			{				
				// For each column
				for (size_t colIndex = 2; colIndex <= numCols; ++colIndex)
				{
					// Set the element at this position from its predecessor along the diagonal
					pOutMatrix->setElem2D(rowIndex, colIndex, pOutMatrix->getElem2D(rowIndex - 1, colIndex - 1));
				}				
			}
			
			// Return the output matrix
			return new ArrayObj(pOutMatrix);
		}
		else
		{
			// Throw an exception
			throw RunError("unsupported input types");
		}	
	}
	
	/***************************************************************
	* Function: toeplitzFuncTypeMapping()
	* Purpose : Type mapping for the "toeplitz" library function 
	* Initial : Maxime Chevalier-Boisvert on May 13, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	TypeSetString toeplitzFuncTypeMapping(const TypeSetString& argTypes)
	{
		// If no type information is provided
		if (argTypes.empty() || argTypes[0].empty() || (argTypes.size() == 2 && argTypes[1].empty()))
		{
			// Return information about a generic 2D matrix
			return typeSetStrMake(TypeInfo(
				DataObject::MATRIX_F64,
				true,
				false,
				false,
				false,
				TypeInfo::DimVector(),
				NULL,
				TypeSet()
			));			
		}
		
		// Create a set to store the possible output types
		TypeSet outSet;
		
		// If there is only one argument
		if (argTypes.size() == 1)
		{
			// Get references to the argument type sets
			const TypeSet& argSet1 = argTypes[0];

			// For each possible input type combination
			for (TypeSet::const_iterator type1 = argSet1.begin(); type1 != argSet1.end(); ++type1)
			{	
				// Infer the output size, if possible
				TypeInfo::DimVector outSize;
				if (type1->getSizeKnown())
				{
					const TypeInfo::DimVector& m1Size = type1->getMatSize();
					
					size_t m1Length = 1;
					for (size_t i = 0; i < m1Size.size(); ++i)
						m1Length *= m1Size[i];
					
					outSize.push_back(m1Length);
					outSize.push_back(m1Length);
				}					
				
				// Add the resulting type to the output set
				outSet.insert(TypeInfo(
					DataObject::MATRIX_F64,
					true,
					type1->getSizeKnown() && outSize[0] == 1 && outSize[1] == 1,
					type1->isInteger(),
					type1->getSizeKnown(),
					outSize,
					NULL,
					TypeSet()
				));	
			}	
		}
		
		// Otherwise, if there are two arguments
		else
		{
			// Get references to the argument type sets
			const TypeSet& argSet1 = argTypes[0];
			const TypeSet& argSet2 = argTypes[1];

			// For each possible input type combination
			for (TypeSet::const_iterator type1 = argSet1.begin(); type1 != argSet1.end(); ++type1)
			{
				for (TypeSet::const_iterator type2 = argSet2.begin(); type2 != argSet2.end(); ++type2)
				{		
					// Test whether or not the output size is known
					bool sizeKnown = type1->getSizeKnown() && type2->getSizeKnown();
					
					// Infer the output size, if possible
					TypeInfo::DimVector outSize;
					if (sizeKnown)
					{
						const TypeInfo::DimVector& m1Size = type1->getMatSize();
						const TypeInfo::DimVector& m2Size = type2->getMatSize();
						
						size_t m1Length = 1;
						for (size_t i = 0; i < m1Size.size(); ++i)
							m1Length *= m1Size[i];
						size_t m2Length = 1;
						for (size_t i = 0; i < m2Size.size(); ++i)
							m2Length *= m2Size[i];
						
						outSize.push_back(m1Length);
						outSize.push_back(m2Length);
					}					
					
					// Add the resulting type to the output set
					outSet.insert(TypeInfo(
						DataObject::MATRIX_F64,
						true,
						sizeKnown && outSize[0] == 1 && outSize[1] == 1,
						type1->isInteger() && type2->isInteger(),
						sizeKnown,
						outSize,
						NULL,
						TypeSet()
					));
				}		
			}
		}
		
		// Return the possible output types
		return TypeSetString(1, outSet);
	}	
	
	/***************************************************************
	* Function: trueFunc()
	* Purpose : Create and initialize a logical array
	* Initial : Maxime Chevalier-Boisvert on March 4, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* trueFunc(ArrayObj* pArguments)
	{
		// Create and initialize the logical array
		return createLogicalArray(pArguments, true);
	}

	/***************************************************************
	* Function: uniqueFunc()
	* Purpose : Find the unique elements in a matrix
	* Initial : Maxime Chevalier-Boisvert on March 4, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* uniqueFunc(ArrayObj* pArguments)
	{
		// Ensure there is one argument
		if (pArguments->getSize() != 1)
			throw RunError("invalid argument count");
	
		// Get a pointer to the argument
		DataObject* pArgument = pArguments->getObject(0);

		// If the argument is a matrix
		if (pArgument->getType() == DataObject::MATRIX_F64)
		{			
			// Get a typed pointer to the argument
			MatrixF64Obj* pInMatrix = (MatrixF64Obj*)pArgument;
			
			// If the input matrix is empty, return a copy of it
			if (pInMatrix->isEmpty())
				return new ArrayObj(pInMatrix->copy());
			
			// Store the matrix elements into a vector
			std::vector<float64> elements(pInMatrix->getElements(), pInMatrix->getElements() + pInMatrix->getNumElems());
			
			// Sort the elements in ascending order
			std::sort(elements.begin(), elements.end());
			
			// Create a vector to store the unique elements
			std::vector<float64> unique;
			unique.reserve(elements.size());
			
			// Add the first element to the vector
			unique.push_back(elements.front());
			
			// For each element
			for (size_t i = 1; i < elements.size(); ++i)
			{
				// If this element differs from the preceding one, keep it
				if (elements[i] != elements[i-1])
					unique.push_back(elements[i]);
			}
			
			// Determine the output vector size
			size_t numRows = unique.size();
			size_t numCols = 1;
			
			// If the input matrix is a horizontal vector, change the output vector orientation
			if (pInMatrix->isVector() && pInMatrix->getSize()[0] == 1)
				std::swap(numRows, numCols);
			
			// Create a vector to store the output
			MatrixF64Obj* pOutMatrix = new MatrixF64Obj(numRows, numCols);
			
			// Copy the unique elements into the output matrix
			memcpy(pOutMatrix->getElements(), &unique[0], unique.size() * sizeof(float64));
			
			// Return the output matrix
			return new ArrayObj(pOutMatrix);
		}
		else
		{
			// Throw an exception
			throw RunError("unsupported input type");
		}		
	}
	
	/***************************************************************
	* Function: uniqueFuncTypeMapping()
	* Purpose : Type mapping for the "unique" library function
	* Initial : Maxime Chevalier-Boisvert on May 14, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	TypeSetString uniqueFuncTypeMapping(const TypeSetString& argTypes)
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
			// Add the resulting type to the output set
			outSet.insert(TypeInfo(
				type1->getObjType(),
				type1->is2D(),
				type1->isScalar(),
				type1->isInteger(),
				false,
				TypeInfo::DimVector(),
				NULL,
				TypeSet()
			));	
		}
		
		// Return the possible output types
		return TypeSetString(1, outSet);
	}
	
	/***************************************************************
	* Function: zerosFunc()
	* Purpose : Create and initialize a matrix
	* Initial : Maxime Chevalier-Boisvert on January 29, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	ArrayObj* zerosFunc(ArrayObj* pArguments)
	{
		// Create and initialize the matrix
		return createMatrix(pArguments, 0);
	}
	
	// Library function objects
	LibFunction abs			("abs"		, absFunc		, absFuncTypeMapping			);
	LibFunction any			("any"		, anyFunc		, anyFuncTypeMapping			);
	LibFunction blkdiag		("blkdiag"	, blkdiagFunc	, blkdiagFuncTypeMapping		);
	LibFunction bitwsand	("bitand"	, bitwsandFunc	, arrayArithOpTypeMapping<true>	);
	LibFunction cd			("cd"		, cdFunc		, nullTypeMapping				);
	LibFunction ceil		("ceil"		, ceilFunc		, intUnaryOpTypeMapping			);
	LibFunction cell		("cell"		, cellFunc		, createCellArrTypeMapping		);
	LibFunction clock		("clock"	, clockFunc		, clockFuncTypeMapping			);
	LibFunction cos			("cos"		, cosFunc		, unaryOpTypeMapping<false>		);
	LibFunction diag		("diag"		, diagFunc		, diagFuncTypeMapping			);
	LibFunction disp		("disp"		, dispFunc		, nullTypeMapping				);
	LibFunction dot			("dot"		, dotFunc		, dotFuncTypeMapping			);
	LibFunction eval		("eval"		, evalFunc		, nullTypeMapping				);
	LibFunction eps			("eps"		, epsFunc		, realScalarTypeMapping			);
	LibFunction exist		("exist"	, existFunc		, intScalarTypeMapping			);
	LibFunction exp			("exp"		, expFunc		, unaryOpTypeMapping<false>		);
	LibFunction eye			("eye"		, eyeFunc		, createF64MatTypeMapping		);
	LibFunction false_		("false"	, falseFunc		, createLogArrTypeMapping		);
	LibFunction fclose		("fclose"	, fcloseFunc	, intScalarTypeMapping			);
	LibFunction feval		("feval"	, fevalFunc		, nullTypeMapping				);
	LibFunction find		("find"		, findFunc		, findFuncTypeMapping			);
	LibFunction fix			("fix"		, fixFunc		, intUnaryOpTypeMapping			);
	LibFunction floor		("floor"	, floorFunc		, intUnaryOpTypeMapping			);
	LibFunction fopen		("fopen"	, fopenFunc		, intScalarTypeMapping			);
	LibFunction fprintf		("fprintf"	, fprintfFunc	, nullTypeMapping				);
	LibFunction i			("i"		, iFunc			, complexScalarTypeMapping		);
	LibFunction iscell		("iscell"	, iscellFunc	, boolScalarTypeMapping			);
	LibFunction isempty		("isempty"	, isemptyFunc	, boolScalarTypeMapping			);
	LibFunction isequal		("isequal"	, isequalFunc	, boolScalarTypeMapping			);
	LibFunction isnumeric	("isnumeric", isnumericFunc	, boolScalarTypeMapping			);
	LibFunction length		("length"	, lengthFunc	, intScalarTypeMapping			);
	LibFunction load		("load"		, loadFunc		, loadFuncTypeMapping			);
	LibFunction log2		("log2"		, log2Func		, unaryOpTypeMapping<false>		);
	LibFunction ls			("ls"		, lsFunc		, stringValueTypeMapping		);
	LibFunction max			("max"		, maxFunc		, maxFuncTypeMapping			);
	LibFunction mean		("mean"		, meanFunc		, vectorOpTypeMapping<false>	);
	LibFunction min			("min"		, minFunc		, maxFuncTypeMapping			);
	LibFunction mod			("mod"		, modFunc		, arrayArithOpTypeMapping<false>);
	LibFunction not_		("not"		, notFunc		, notFuncTypeMapping			);
	LibFunction num2str		("num2str"	, num2strFunc	, stringValueTypeMapping		);
	LibFunction numel		("numel"	, numelFunc		, intScalarTypeMapping			);
	LibFunction ones		("ones"		, onesFunc		, createF64MatTypeMapping		);
	LibFunction pi			("pi"		, piFunc		, realScalarTypeMapping			);
	LibFunction plot        ("plot"     , plotFunc      , plotFuncTypeMapping           );
	LibFunction pwd			("pwd"		, pwdFunc		, stringValueTypeMapping		);
	LibFunction rand		("rand"		, randFunc		, createF64MatTypeMapping		);
	LibFunction reshape		("reshape"	, reshapeFunc	, reshapeFuncTypeMapping		);
	LibFunction round		("round"	, roundFunc		, intUnaryOpTypeMapping			);
	LibFunction sign		("sign"		, signFunc		, intUnaryOpTypeMapping			);
	LibFunction sin			("sin"		, sinFunc		, unaryOpTypeMapping<false>		);
	LibFunction size		("size"		, sizeFunc		, sizeFuncTypeMapping			);
	LibFunction sort		("sort"		, sortFunc		, sortFuncTypeMapping			);
	LibFunction sprintf		("sprintf"	, sprintfFunc	, stringValueTypeMapping		);
	LibFunction squeeze		("squeeze"	, squeezeFunc	, squeezeFuncTypeMapping		);
	LibFunction sqrt		("sqrt"		, sqrtFunc		, unaryOpTypeMapping<false>		);
	LibFunction strcat		("strcat"	, strcatFunc	, strcatFuncTypeMapping			);
	LibFunction strcmp		("strcmp"	, strcmpFunc	, boolScalarTypeMapping			);
	LibFunction sum			("sum"		, sumFunc		, vectorOpTypeMapping<false>	);
	LibFunction system		("system"	, systemFunc	, systemFuncTypeMapping			);
	LibFunction tic			("tic"		, ticFunc		, nullTypeMapping				);
	LibFunction toc			("toc"		, tocFunc		, realScalarTypeMapping			);
	LibFunction toeplitz	("toeplitz"	, toeplitzFunc	, toeplitzFuncTypeMapping		);
	LibFunction true_		("true"		, trueFunc		, createLogArrTypeMapping		);
	LibFunction unique		("unique"	, uniqueFunc	, uniqueFuncTypeMapping			);
	LibFunction zeros		("zeros"	, zerosFunc		, createF64MatTypeMapping		);
	
	/***************************************************************
	* Function: loadLibrary()
	* Purpose : Load the library functions into the interpreter
	* Initial : Maxime Chevalier-Boisvert on January 28, 2009
	****************************************************************
	Revisions and bug fixes:
	*/
	void loadLibrary()
	{
		// Bind the library functions in the interpreter's environment
		Interpreter::setBinding(abs.getFuncName()		, (DataObject*)&abs			);
		Interpreter::setBinding(any.getFuncName()		, (DataObject*)&any			);
		Interpreter::setBinding(blkdiag.getFuncName()	, (DataObject*)&blkdiag		);
		Interpreter::setBinding(bitwsand.getFuncName()	, (DataObject*)&bitwsand	);
		Interpreter::setBinding(cd.getFuncName()		, (DataObject*)&cd			);
		Interpreter::setBinding(ceil.getFuncName()		, (DataObject*)&ceil		);
		Interpreter::setBinding(cell.getFuncName()		, (DataObject*)&cell		);
		Interpreter::setBinding(clock.getFuncName()		, (DataObject*)&clock		);
		Interpreter::setBinding(cos.getFuncName()		, (DataObject*)&cos			);
		Interpreter::setBinding(diag.getFuncName()		, (DataObject*)&diag		);
		Interpreter::setBinding(disp.getFuncName()		, (DataObject*)&disp		);
		Interpreter::setBinding(dot.getFuncName()		, (DataObject*)&dot			);
		Interpreter::setBinding(eval.getFuncName()		, (DataObject*)&eval		);
		Interpreter::setBinding(eps.getFuncName()		, (DataObject*)&eps			);
		Interpreter::setBinding(exist.getFuncName()		, (DataObject*)&exist		);
		Interpreter::setBinding(exp.getFuncName()		, (DataObject*)&exp			);
		Interpreter::setBinding(eye.getFuncName()		, (DataObject*)&eye			);
		Interpreter::setBinding(false_.getFuncName()	, (DataObject*)&false_		);
		Interpreter::setBinding(fclose.getFuncName()	, (DataObject*)&fclose		);
		Interpreter::setBinding(feval.getFuncName()		, (DataObject*)&feval		);
		Interpreter::setBinding(find.getFuncName()		, (DataObject*)&find		);
		Interpreter::setBinding(fix.getFuncName()		, (DataObject*)&fix			);
		Interpreter::setBinding(floor.getFuncName()		, (DataObject*)&floor		);
		Interpreter::setBinding(fopen.getFuncName()		, (DataObject*)&fopen		);
		Interpreter::setBinding(fprintf.getFuncName()	, (DataObject*)&fprintf		);
		Interpreter::setBinding(i.getFuncName()			, (DataObject*)&i			);
		Interpreter::setBinding(iscell.getFuncName()	, (DataObject*)&iscell		);
		Interpreter::setBinding(isempty.getFuncName()	, (DataObject*)&isempty		);
		Interpreter::setBinding(isequal.getFuncName()	, (DataObject*)&isequal		);
		Interpreter::setBinding(isnumeric.getFuncName()	, (DataObject*)&isnumeric	);
		Interpreter::setBinding(length.getFuncName()	, (DataObject*)&length		);
		Interpreter::setBinding(load.getFuncName()		, (DataObject*)&load		);
		Interpreter::setBinding(log2.getFuncName()		, (DataObject*)&log2		);
		Interpreter::setBinding(ls.getFuncName()		, (DataObject*)&ls			);
		Interpreter::setBinding(max.getFuncName()		, (DataObject*)&max			);
		Interpreter::setBinding(mean.getFuncName()		, (DataObject*)&mean		);
		Interpreter::setBinding(min.getFuncName()		, (DataObject*)&min			);
		Interpreter::setBinding(mod.getFuncName()		, (DataObject*)&mod			);
		Interpreter::setBinding(not_.getFuncName()		, (DataObject*)&not_		);
		Interpreter::setBinding(num2str.getFuncName()	, (DataObject*)&num2str		);
		Interpreter::setBinding(numel.getFuncName()		, (DataObject*)&numel		);
		Interpreter::setBinding(ones.getFuncName()		, (DataObject*)&ones		);
		Interpreter::setBinding(pi.getFuncName()		, (DataObject*)&pi			);
		Interpreter::setBinding(plot.getFuncName()      , (DataObject*)&plot        );
		Interpreter::setBinding(pwd.getFuncName()		, (DataObject*)&pwd			);
		Interpreter::setBinding(rand.getFuncName()		, (DataObject*)&rand		);
		Interpreter::setBinding(reshape.getFuncName()	, (DataObject*)&reshape		);
		Interpreter::setBinding(round.getFuncName()		, (DataObject*)&round		);
		Interpreter::setBinding(sign.getFuncName()		, (DataObject*)&sign		);
		Interpreter::setBinding(sin.getFuncName()		, (DataObject*)&sin			);
		Interpreter::setBinding(size.getFuncName()		, (DataObject*)&size		);
		Interpreter::setBinding(sort.getFuncName()		, (DataObject*)&sort		);
		Interpreter::setBinding(sprintf.getFuncName()	, (DataObject*)&sprintf		);
		Interpreter::setBinding(squeeze.getFuncName()	, (DataObject*)&squeeze		);
		Interpreter::setBinding(sqrt.getFuncName()		, (DataObject*)&sqrt		);
		Interpreter::setBinding(strcat.getFuncName()	, (DataObject*)&strcat		);
		Interpreter::setBinding(strcmp.getFuncName()	, (DataObject*)&strcmp		);
		Interpreter::setBinding(sum.getFuncName()		, (DataObject*)&sum			);
		Interpreter::setBinding(system.getFuncName()	, (DataObject*)&system		);
		Interpreter::setBinding(tic.getFuncName()		, (DataObject*)&tic			);
		Interpreter::setBinding(toc.getFuncName()		, (DataObject*)&toc			);
		Interpreter::setBinding(toeplitz.getFuncName()	, (DataObject*)&toeplitz	);
		Interpreter::setBinding(true_.getFuncName()		, (DataObject*)&true_		);
		Interpreter::setBinding(unique.getFuncName()	, (DataObject*)&unique		);
		Interpreter::setBinding(zeros.getFuncName()		, (DataObject*)&zeros		);
		
		// Declare a type set for a float64 scalar type
		TypeSet f64ScalarArg = typeSetMake(TypeInfo(
			DataObject::MATRIX_F64,
			true,
			true,
			false,
			true,
			TypeInfo::DimVector(2,1),
			NULL,
			TypeSet()
		));
		
		// Register an optimized abs function
		float64 (*pAbsFuncF64)(float64) = std::abs;
		JITCompiler::regLibraryFunc(
			&abs,
			(void*)pAbsFuncF64,
			TypeSetString(1, f64ScalarArg),
			f64ScalarArg,
			true,
			true,
			true		
		);
		
		// Register an optimized exp function
		float64 (*pExpFuncF64)(float64) = ::exp;
		JITCompiler::regLibraryFunc(
			&exp,
			(void*)pExpFuncF64,
			TypeSetString(1, f64ScalarArg),
			f64ScalarArg,
			true,
			true,
			true		
		);
		
		// Register an optimized sin function
		float64 (*pSinFuncF64)(float64) = ::sin;
		JITCompiler::regLibraryFunc(
			&sin,
			(void*)pSinFuncF64,
			TypeSetString(1, f64ScalarArg),
			f64ScalarArg,
			true,
			true,
			true		
		);	
	}
};
