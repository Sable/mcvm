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
#ifndef MATRIXOBJS_H_
#define MATRIXOBJS_H_

// Header files
#include <iostream>
#include <cassert>
#include <vector>
#include <cstdlib>
#include <cstring>
#include "platform.h"
#include "objects.h"
#include "arrayobj.h"
#include "rangeobj.h"
#include "runtimebase.h"
#include "utility.h"
#include "profiling.h"
#include "dimvector.h"
// Dimension vector type definition
//typedef std::vector<size_t, gc_allocator<size_t> > DimVector;

// Helper functions to highlight indexing conversions
inline size_t toZeroIndex(size_t oneIndex) { return oneIndex - 1; }
inline size_t toOneIndex(size_t zeroIndex) { return zeroIndex + 1; }

/***************************************************************
* Class   : BaseMatrixObj
* Purpose : Base class for all matrix objects
* Initial : Maxime Chevalier-Boisvert on February 15, 2009
****************************************************************
Revisions and bug fixes:
*/
class BaseMatrixObj : public DataObject
{
	// Declare the JIT compiler as a friend class
	// This is so the JIT can access the internal data directly
	friend class JITCompiler;
	
public:
	
	// Method to test if slice indices are valid (positive integers)
	bool validIndices(const ArrayObj* pSlice) const;
	
	// Method to compute the maximum indices for a slice
	DimVector getMaxIndices(const ArrayObj* pSlice, const BaseMatrixObj* pAssignMatrix = NULL) const;
	
	// Method to test if a multidimensional index is valid
	bool boundsCheckND(const DimVector& indices) const;
	
	// Method to expand this matrix
	virtual void expand(const DimVector& indices) = 0;
	
	// Static method to expand a matrix
	static void expandMatrix(BaseMatrixObj* pMatrix, size_t* pIndices, size_t numIndices);
	
	// Method to generate a sub-matrix (multidimensional slice)
	virtual BaseMatrixObj* getSliceND(const ArrayObj* pSlice) const = 0;
	
	// Method to set elements of this matrix from a sub-matrix
	virtual void setSliceND(const ArrayObj* pSlice, const DataObject* pSubMatrix) = 0;
	
	// Method to concatenate this matrix with another matrix
	virtual BaseMatrixObj* concat(const BaseMatrixObj* pOther, size_t catDim) const = 0;
	
	// Static method to test if matrices are compatible for multiplication
	static bool multCompatible(const BaseMatrixObj* pMatrixA, const BaseMatrixObj* pMatrixB);
	
	// Static method to test if matrices are compatible for left division
	static bool leftDivCompatible(const BaseMatrixObj* pMatrixA, const BaseMatrixObj* pMatrixB);
	
	// Method to test if the matrix is a scalar value
	bool isScalar() const { return (m_size.size() == 2 && m_size[0] == 1 && m_size[1] == 1); }
	
	// Method to test if the matrix is a vector
	bool isVector() const { return m_size.size() == 2 && (m_size[0] == 1 || m_size[1] == 1); }
	
	// Method to test if the matrix is square
	bool isSquare() const { return m_size.size() == 2 && m_size[0] == m_size[1]; }	
	
	// Method to test if the matrix is is empty
	bool isEmpty() const { return (m_numElements == 0); }
	
	// Method to test if this matrix is bidimensional
	bool is2D() const { return (m_size.size() == 2); }
		
	// Accessor to get the matrix dimensions
	const DimVector& getSize() const { return m_size; }
	
	// Accessor to get the number of dimensions
	const size_t getNumDims() const { return m_size.size(); }
	
	// Accessor to get the number of matrix elements
	const size_t getNumElems() const { return m_numElements; }
	
	// Static method to get the number of matrix dimensions
	static size_t getDimCount(const BaseMatrixObj* pMatrix) { return pMatrix->m_size.size(); }
	
	// Static method to get a pointer to the size array
	static const size_t* getSizeArray(const BaseMatrixObj* pMatrix) { return &pMatrix->m_size[0]; }
	
protected:
	
	// Helper method to recursively implement slice copying
	template <class ScalarType>
	void getSliceND(const ArrayObj* pSlice, size_t curDim, size_t* pIndices, ScalarType*& pDstElem) const;
	
	// Vector of matrix dimensions
	DimVector m_size;
	
	// Number of matrix elements
	size_t m_numElements;
};

/***************************************************************
* Class   : MatrixObj<>
* Purpose : Templated class for matrix and vector data types
* Initial : Maxime Chevalier-Boisvert on January 15, 2009
****************************************************************
Revisions and bug fixes:
*/
template <class ScalarType> class MatrixObj : public BaseMatrixObj
{
	// Declare the JIT compiler as a friend class
	// This is so the JIT can access the internal data directly
	friend class JITCompiler;
	
public:
	
	// Method to get the object type associated with this class
	inline DataObject::Type getClassType();
	
	// Default constructor (empty matrix)
	MatrixObj()
	: m_pElements(NULL)
	{
		// Initialize the matrix size
		m_size.resize(2, 0);
		
		// Initialize the number of elements
		m_numElements = 0;
		
		// Set the object type
		m_type = getClassType();
		
		// Increment the matrix creation count
		PROF_INCR_COUNTER(Profiler::MATRIX_CONSTR_COUNT);
	}

	// Scalar constructor (1x1 matrix)
	MatrixObj(ScalarType elemValue)
	{
		// Initialize the matrix size
		m_size.resize(2, 1);		
		
		// Set the object type
		m_type = getClassType();
		
		// Allocate data for the matrix
		allocMatrix();
		
		// Initialize the matrix
		initMatrix(elemValue);
		
		// Increment the matrix creation count
		PROF_INCR_COUNTER(Profiler::MATRIX_CONSTR_COUNT);
	}
	
	// 2D matrix constructor (m x n matrix)
	MatrixObj(size_t numRows, size_t numCols, ScalarType elemValue = 0)
	{
		// Set the object type
		m_type = getClassType();
		
		// Resize the size vector to store 2 elements
		m_size.resize(2);
		
		// Set the matrix dimensions
		m_size[0] = numRows;
		m_size[1] = numCols;
		
		// Allocate data for the matrix
		allocMatrix();
		
		// Initialize the matrix
		initMatrix(elemValue);
		
		// Increment the matrix creation count
		PROF_INCR_COUNTER(Profiler::MATRIX_CONSTR_COUNT);
	}
	
	// N-dimensional matrix constructor (m x n x p x ... matrix)
	MatrixObj(const DimVector& newSize, ScalarType elemValue = 0)
	{
		// Set the object type
		m_type = getClassType();
		
		// Ensure that a size was specified
		assert (!newSize.empty());
		
		// Set the new matrix size
		m_size = newSize;
		
		// Pop superfluous size 1 dimensions
		while (m_size.back() == 1 && m_size.size() > 2)
			m_size.pop_back();
		
		// If there is only one size element
		if (m_size.size() == 1)
		{
			// Add a second element, making this matrix 2D instead of 1D
			m_size.push_back(m_size.front() == 0? 0:1);
		}
		
		// Initialize the element count
		size_t elemCount = 1;
		
		// Compute the total element count
		for (DimVector::const_iterator itr = m_size.begin(); itr != m_size.end(); ++itr)
			elemCount *= *itr;
		
		// Allocate data for the matrix
		allocMatrix();
		
		// Initialize the matrix
		initMatrix(elemValue);
		
		// Increment the matrix creation count
		PROF_INCR_COUNTER(Profiler::MATRIX_CONSTR_COUNT);
	}	

	// Static method to create a scalar matrix of this type
	static MatrixObj* makeScalar(ScalarType elemValue)
	{
		// Create and return the scalar matrix object
		return new MatrixObj(elemValue);
	}
	
	// Method to copy this data object
	virtual MatrixObj* copy() const
	{
		// Create a new matrix object
		MatrixObj* pNewMatrix = new MatrixObj();
		
		// Copy the matrix dimensions
		pNewMatrix->m_size = m_size;
		
		// Allocate memory for the new matrix
		pNewMatrix->allocMatrix();
		
		// Copy all the matrix elements into the new matrix
		memcpy(pNewMatrix->m_pElements, m_pElements, sizeof(ScalarType) * m_numElements);
		
		// Return the new matrix object
		return pNewMatrix;
	}
	
	// Method to obtain a string representation of this object
	virtual std::string toString() const
	{
		// If the matrix is a scalar
		if (isScalar())
		{
			// Output the text representation of the scalar value
			return ::toString(*m_pElements);
		}
		
		// Create a string to store the output
		std::string output;
		
		// Log that we will print the matrix size
		output += "matrix of size ";
		
		// For each dimension of the matrix
		for (size_t i = 0; i < m_size.size(); ++i)
		{
			// Add this dimension to the output
			output += ::toString(m_size[i]);
			
			// If this is not the last dimension, add a separator
			if (i < m_size.size() - 1)
				output += "x";
		}
		
		// Add a newline to the output
		output += "\n";
		
		// If the matrix has 0 elements
		if (m_numElements == 0)
		{
			// Add the empty matrix symbol to the output
			output += "[]\n";
			
			// Return early
			return output;
		}
		
		// Create and initialize a vector for the indices
		DimVector indices(m_size.size(), 1);
		
		// Initialize the current dimension index
		size_t curDim = 2;
		
		// For each 2D slice of the matrix
		for (;;)
		{	
			// If there is more than one slice
			if (indices.size() > 2)
			{
				// Begin the slice index line
				output += "\nmatrix(:,:";
				
				// Output the slice indices
				for (size_t i = 2; i < indices.size(); ++i)
					output += "," + ::toString(indices[i]);
				
				// End the slice index line
				output += ")\n";
			}
			
			// For each row
			for (indices[0] = 1; indices[0] <= m_size[0]; ++indices[0])
			{
				// For each column
				for (indices[1] = 1; indices[1] <= m_size[1]; ++indices[1])
				{
					// Add this element to the output
					output += "\t" + ::toString(getElemND(indices));
				}
				
				// Add a newline to the output
				output += "\n";				
			}
			
			// If there are no slice indices to increase, stop
			if (curDim >= indices.size())
				break;
			
			// Until we are done moving to the next slice
			while (curDim < indices.size())
			{
				// Move to the next slice along this dimension
				indices[curDim]++;
				
				// If the index along this dimension is still valid
				if (indices[curDim] <= m_size[curDim])
				{
					// Go back to the first slicing dimension
					curDim = 2;
					
					// Stop the process
					break;
				}
				else
				{
					// Move to the next dimension
					curDim++;
					
					// Reset the indices for prior dimensions
					for (size_t i = 2; i < curDim; ++i)
						indices[i] = 1;
				}				
			}
				
			// If we went through all slices, stop
			if (curDim == indices.size())
				break;			
		}		
		
		// Return the output string
		return output;
	}
	
	// Method to convert this matrix to the requested type
	virtual DataObject* convert(DataObject::Type outType) const
	{
		// Switch on the output type type
		switch (outType)
		{
			// Float64 matrix
			case MATRIX_F64:
			{
				// Create a logical array object of the same size
				MatrixObj<float64>* pOutput = new MatrixObj<float64>(m_size);
				
				// Convert each element of this matrix
				for (size_t i = 1; i <= m_numElements; ++i)
					pOutput->setElem1D(i, (float64)getElem1D(i));
				
				// Return the output object
				return pOutput;
			}
			break;

			// Complex matrix
			case MATRIX_C128:
			{
				// Create a logical array object of the same size
				MatrixObj<Complex128>* pOutput = new MatrixObj<Complex128>(m_size);
				
				// Convert each element of this matrix
				for (size_t i = 1; i <= m_numElements; ++i)
					pOutput->setElem1D(i, (Complex128)getElem1D(i));
				
				// Return the output object
				return pOutput;
			}
			break;
			
			// Logical array
			case LOGICALARRAY:
			{
				// Create a logical array object of the same size
				MatrixObj<bool>* pOutput = new MatrixObj<bool>(m_size);
				
				// Convert each element of this matrix
				for (size_t i = 1; i <= m_numElements; ++i)
					pOutput->setElem1D(i, (getElem1D(i) == 0)? false:true);
				
				// Return the output object
				return pOutput;
			}
			break;
		
			// For all other output types
			default:
			{
				// Refer to the default conversion method
				return DataObject::convert(outType);
			}
		}
	}
	
	// Method to expand this matrix
	virtual void expand(const DimVector& indices)
	{
		// Ensure that the index vector is not empty
		assert (indices.empty() == false);
		
		// Store the current (old) matrix size
		DimVector oldSize = m_size;
		
		// Store a pointer to the current (old) matrix elements
		ScalarType* pOldElements = m_pElements;

		// Create a vector for the new matrix size
		DimVector newSize = indices;
		
		// If this matrix is empty or scalar, expand it
		// horizontally for Matlab consistency 
		if ((isEmpty() || isScalar()) && newSize.size() == 1)
			//newSize.insert(newSize.begin(), 1);
			newSize.insert(0, 1);
		
		// Declare a variable for the number of matrix elements
		size_t numElements = 1;
		
		// For each index element
		for (size_t i = 0; i < indices.size(); ++i)
		{
			// Ensure each index value is nonzero
			assert (indices[i] > 0);
			
			// If this dimension is within the previous size
			if (i < m_size.size())
			{
				// Make the size along this dimension the maximum
				// of the current size and the requested index value
				newSize[i] = std::max(newSize[i], m_size[i]);
			}
			
			// Update the number of elements
			numElements *= newSize[i];
		}
		
		// Ensure that the size has at least 2 dimensions
		if (newSize.size() == 1)
			newSize.push_back(1);
		
		// Pop superfluous size 1 dimensions
		while (newSize.back() == 1 && newSize.size() > 2)
			newSize.pop_back();
		
		// Resize the old size vector to match the size of the new vector
		oldSize.resize(newSize.size(), 1);
		
		// Set the new matrix size
		m_size = newSize;
		
		// Allocate data for the new matrix elements
		allocMatrix();
		
		// If the allocation failed, throw an exception
		if (m_pElements == NULL)
			throw RunError("allocation failed during matrix expand operation");
		
		// Create vectors for the stride info
		DimVector srcStride(oldSize.size());
		DimVector dstStride(newSize.size());

		// Compute the source stride values
		srcStride[0] = 1;
		for (size_t i = 1; i < srcStride.size(); ++i)
			srcStride[i] = oldSize[i-1] * srcStride[i-1];
		
		// Compute the destination stride values
		dstStride[0] = 1;
		for (size_t i = 1; i < dstStride.size(); ++i)
			dstStride[i] = newSize[i-1] * dstStride[i-1];
		
		// Recursively perform the matrix expansion
		expand(oldSize, newSize, srcStride, dstStride, pOldElements, m_pElements, newSize.size() - 1);
		
		// Delete the old matrix elements
		delete [] pOldElements;
	}

	// Method to recursively expand this matrix
	void expand(const DimVector& srcSize, const DimVector& dstSize, const DimVector& srcStride, const DimVector& dstStride, const ScalarType* pSrc, ScalarType* pDst, size_t curDim)
	{
		// If we are at the lowest dimension
		if (curDim == 0)
		{
			// Copy the elements of this column
			memcpy(pDst, pSrc, sizeof(ScalarType) * srcSize[0]);
			
			// Fill the rest of the column with zeros
			std::uninitialized_fill(pDst + srcSize[0], pDst + dstSize[0], (ScalarType)0);
		}
		else
		{
			// Initialize the slice pointers
			const ScalarType* pSrcSlice = pSrc;
			ScalarType* pDstSlice = pDst;
			
			// For each slice to be copied
			for (size_t i = 0; i < srcSize[curDim]; ++i)
			{
				// Recursively copy the lower dimensions
				expand(
					srcSize,
					dstSize,
					srcStride,
					dstStride,
					pSrcSlice, 
					pDstSlice,
					curDim - 1
				);
				
				// Update the slice pointers
				pSrcSlice += srcStride[curDim];
				pDstSlice += dstStride[curDim];
			}
			
			// Compute a pointer to the end of this slice
			ScalarType* pDstEnd = pDst + dstSize[curDim] * dstStride[curDim];
			
			// Initialize the rest of the slice to a default value
			initRange(pDstSlice, pDstEnd);
		}		
	}
	
	// Method to generate a sub-matrix (multidimensional slice)
	virtual MatrixObj* getSliceND(const ArrayObj* pSlice) const
	{
		// Ensure that the slice has at most as many dimensions as this matrix
		assert (pSlice->getSize() <= m_size.size());
				
		// Create a size vector for the new matrix and reserve space for each dimension
		DimVector newSize;
		newSize.reserve(pSlice->getSize());
		
		// Initialize the dimension iterator
		DimVector::const_iterator dimItr = m_size.begin();
		
		// For each dimension of the slice
		for (size_t i = 0; i < pSlice->getSize(); ++i)
		{		
			// Get the slice along the current dimension
			const DataObject* pCurSlice = pSlice->getObject(i);
			
			// Extract the size along this dimension
			size_t dimSize = *(dimItr++);
			
			// Declare a variable for the element count along this dimension
			size_t dimCount;
			
			// If the object is a matrix
			if (pCurSlice->isMatrixObj())
			{
				// If the object is a logical array
				if (pCurSlice->getType() == DataObject::LOGICALARRAY)
				{
					// Get a typed pointer to the matrix object
					MatrixObj<bool>* pMatrix = (MatrixObj<bool>*)pCurSlice;
						
					// Get pointers to the start and end values
					const bool* pStartVal = pMatrix->getElements();
					const bool* pEndVal = pStartVal + pMatrix->getNumElems();
					
					// Initialize the element count at 0
					dimCount = 0;
					
					// For each value in the matrix
					for (const bool* pValue = pStartVal; pValue != pEndVal; ++pValue)
					{
						// If this value is true, increment the element count
						if (*pValue) ++dimCount;
					}
				}
				else
				{
					// Get a typed pointer to the matrix object
					const BaseMatrixObj* pMatrix = (BaseMatrixObj*)pCurSlice;
				
					// There are as many elements as indices
					dimCount = pMatrix->getNumElems();
				}
			}
			
			// If the object is a range
			else if (pCurSlice->getType() == DataObject::RANGE)
			{
				// Get a typed pointer to the range object
				const RangeObj* pRange = (RangeObj*)pCurSlice;
				
				// If this is the full range
				if (pRange->isFullRange())
				{
					// Set the count to the size of this dimension
					dimCount = dimSize;
					
					// If this is the last dimension of the multislice
					if (i == pSlice->getSize() - 1)
					{
						// Compute the size of the extended last dimension
						for (; dimItr != m_size.end(); ++dimItr)
							dimCount *= *dimItr;
					}
				}
				else
				{
					// There are as many elements as numbers in the range
					dimCount = pRange->getElemCount();
				}
			}
			
			// Otherwise, for any other object type
			else
			{
				// Break an assertion
				assert (false);
			}

			// Add the size of this dimension to the sub-matrix's size vector
			newSize.push_back(dimCount);
		}
		
		// Ensure that the size has at least two elements
		if (newSize.size() == 1)
			newSize.push_back(1);
		
		// If this matrix is a vector
		if (isVector())
		{
			// If the vector orientation does not match with the sub-matrix size
			if ((m_size[0] != 1 && newSize[1] != 1) || (m_size[1] != 1 && newSize[0] != 1))
			{
				// Change the orientation of the sub-matrix
				std::swap(newSize[0], newSize[1]);
			}
		}
	
		// Create a new matrix object to store the sub-matrix
		MatrixObj* pSubMatrix = new MatrixObj(newSize);
		
		// If the sub-matrix is isEmpty, return early
		if (pSubMatrix->isEmpty())
			return pSubMatrix;

		// Get a pointer to the current element in the new matrix
		ScalarType* pDstElem = pSubMatrix->m_pElements;
		
		// Allocate memory for the indices
		size_t* pIndices = (size_t*)alloca(sizeof(size_t) * pSlice->getSize());
		
		// Copy the slice recursively
		getSliceND(pSlice, pSlice->getSize() - 1, pIndices, pDstElem);
		
		// Increment the matrix slice read count
		PROF_INCR_COUNTER(Profiler::MATRIX_GETSLICE_COUNT);
		
		// Return the sub-matrix
		return pSubMatrix;
	}
	
	// Helper method to recursively implement slice copying
	void getSliceND(const ArrayObj* pSlice, size_t curDim, size_t* pIndices, ScalarType*& pDstElem) const
	{		
		// Get a reference to the slice along the current dimension
		const DataObject* pCurSlice = pSlice->getObject(curDim);
		
		// Compute the index of the next lower dimension
		size_t nextDim = curDim - 1;
		
		// If we are not at the lowest dimension
		if (curDim > 0)
		{
			// If the object is a matrix
			if (pCurSlice->getType() == DataObject::MATRIX_F64)
			{
				// Get a typed pointer to the matrix object
				MatrixObj<float64>* pMatrix = (MatrixObj<float64>*)pCurSlice;
				
				// Get pointers to the start and end values
				const float64* pStartVal = pMatrix->getElements();
				const float64* pEndVal = pStartVal + pMatrix->getNumElems();
				
				// For each value in the matrix
				for (const float64* pValue = pStartVal; pValue != pEndVal; ++pValue)
				{
					// Set the index for this dimension
					pIndices[curDim] = toZeroIndex(size_t(*pValue));
						
					// Recurse for the next lower dimension
					getSliceND(pSlice, nextDim, pIndices, pDstElem);					
				}
			}
			
			// If the object is a logical array
			else if (pCurSlice->getType() == DataObject::LOGICALARRAY)
			{
				// Get a typed pointer to the matrix object
				MatrixObj<bool>* pMatrix = (MatrixObj<bool>*)pCurSlice;
					
				// Get pointers to the start and end values
				const bool* pStartVal = pMatrix->getElements();
				const bool* pEndVal = pStartVal + pMatrix->getNumElems();
				
				// Initialize the current index at 0
				size_t index = 0;
				
				// For each value in the matrix
				for (const bool* pValue = pStartVal; pValue != pEndVal; ++pValue)
				{
					// If this value is true
					if (*pValue)
					{
						// Set the index for this dimension
						pIndices[curDim] = index++;
						
						// Recurse for the next lower dimension
						getSliceND(pSlice, nextDim, pIndices, pDstElem);
					}
					
					// Increment the index
					++index;
				}
			}
			
			// If the object is a range
			else if (pCurSlice->getType() == DataObject::RANGE)
			{
				// Get a typed pointer to the range object
				const RangeObj* pRange = (RangeObj*)pCurSlice;
				
				// If this is the full range
				if (pRange->isFullRange())
				{
					// Set the element count to the size of this dimension
					size_t elemCount = m_size[curDim];
					
					// If this is the last dimension of the multislice
					if (curDim == pSlice->getSize() - 1)
					{
						// Compute the size of the extended last dimension
						for (size_t dim = curDim + 1; dim < m_size.size(); ++dim)
							elemCount *= m_size[dim];
					}
					
					// For each element along this dimension
					for (size_t i = 0; i < elemCount; ++i)
					{
						// Set the index for this dimension
						pIndices[curDim] = i;
						
						// Recurse for the next lower dimension
						getSliceND(pSlice, nextDim, pIndices, pDstElem);
					}					
				}
				else
				{
					// Initialize the current value to the start value
					double value = pRange->getStartVal();
				
					// Get the element count for the range
					size_t elemCount = pRange->getElemCount();
					
					// For each range element
					for (size_t i = 1; i <= elemCount; ++i)
					{
						// Set the index for this dimension
						pIndices[curDim] = toZeroIndex(size_t(value));
							
						// Recurse for the next lower dimension
						getSliceND(pSlice, nextDim, pIndices, pDstElem);
						
						// Update the current value
						value += pRange->getStepVal();
					}					
				}
			}
			
			// Otherwise, for any other object type
			else
			{
				// Break an assertion
				assert (false);
			}
		}

		// Otherwise, we are at the lowest dimension
		else
		{
			// Initialize the base address pointer
			ScalarType* pBaseAddr = m_pElements;
			
			// Initialize the dimensional offset product
			size_t dimOffset = 1;
			
			// For each dimension except the first
			for (size_t i = 1; i < pSlice->getSize(); ++i)
			{
				// Update the dimensional offset product
				dimOffset *= m_size[i - 1];
				
				// Update the base address pointer
				pBaseAddr += pIndices[i] * dimOffset;				
			}	
			
			// If the object is a matrix
			if (pCurSlice->getType() == DataObject::MATRIX_F64)
			{
				// Get a typed pointer to the matrix object
				MatrixObj<float64>* pMatrix = (MatrixObj<float64>*)pCurSlice;

				// Get pointers to the start and end values
				const float64* pStartVal = pMatrix->getElements();
				const float64* pEndVal = pStartVal + pMatrix->getNumElems();
								
				// For each value in the matrix
				for (const float64* pValue = pStartVal; pValue != pEndVal; ++pValue)
				{
					// Convert the index to zero indexing
					size_t zeroIndex = toZeroIndex(size_t(*pValue));
					
					// Compute the source address
					ScalarType* pSrcAddr = pBaseAddr + zeroIndex;
					
					// Copy the element
					*pDstElem = *pSrcAddr;
					
					// Increment the destination element pointer
					++pDstElem;
				}
			}
			
			// If the object is a logical array
			else if (pCurSlice->getType() == DataObject::LOGICALARRAY)
			{
				// Get a typed pointer to the matrix object
				MatrixObj<bool>* pMatrix = (MatrixObj<bool>*)pCurSlice;
					
				// Get pointers to the start and end values
				const bool* pStartVal = pMatrix->getElements();
				const bool* pEndVal = pStartVal + pMatrix->getNumElems();
				
				// Initialize the current index at 0
				size_t index = 0;
				
				// For each value in the matrix
				for (const bool* pValue = pStartVal; pValue != pEndVal; ++pValue)
				{
					// If this value is true
					if (*pValue)
					{
						// Compute the source address
						ScalarType* pSrcAddr = pBaseAddr + index;
						
						// Copy the element
						*pDstElem = *pSrcAddr;
						
						// Increment the destination element pointer
						++pDstElem;
					}
					
					// Increment the index
					++index;
				}
			}
			
			// If the object is a range
			else if (pCurSlice->getType() == DataObject::RANGE)
			{
				// Get a typed pointer to the range object
				const RangeObj* pRange = (RangeObj*)pCurSlice;
				
				// If this is the full range
				if (pRange->isFullRange())
				{
					// Set the element count to the size of this dimension
					size_t elemCount = m_size[curDim];
					
					// If this is the last dimension of the multislice
					if (curDim == pSlice->getSize() - 1)
					{
						// Compute the size of the extended last dimension
						for (size_t dim = curDim + 1; dim < m_size.size(); ++dim)
							elemCount *= m_size[dim];
					}
					
					// For each element along this dimension
					for (size_t i = 0; i < elemCount; ++i)
					{
						// Compute the source address
						ScalarType* pSrcAddr = pBaseAddr + i;
						
						// Copy the element
						*pDstElem = *pSrcAddr;
						
						// Increment the destination element pointer
						++pDstElem;						
					}
				}
				else
				{
					// Initialize the current value to the start value
					double value = pRange->getStartVal();
				
					// Get the element count for the range
					size_t elemCount = pRange->getElemCount();
					
					// For each range element
					for (size_t i = 1; i <= elemCount; ++i)
					{
						// Convert the index to zero indexing
						size_t zeroIndex = toZeroIndex(size_t(value));
						
						// Compute the source address
						ScalarType* pSrcAddr = pBaseAddr + zeroIndex;
						
						// Copy the element
						*pDstElem = *pSrcAddr;
						
						// Increment the destination element pointer
						++pDstElem;
						
						// Update the current range value
						value += pRange->getStepVal();
					}
				}
			}
			
			// Otherwise, for any other object type
			else
			{
				// Break an assertion
				assert (false);
			}		
		}
	}
	
	// Method to set elements of this matrix from a sub-matrix
	virtual void setSliceND(const ArrayObj* pSlice, const DataObject* pSubMatrix)
	{
		// Ensure that the slice has at most as many dimensions as this matrix
		assert (pSlice->getSize() <= m_size.size());
	
		// Declare a pointer for the source matrix
		MatrixObj* pSrcMatrix;
		
		// Convert the source matrix to the local type, if necessary
		if (m_type != pSubMatrix->getType())
			pSrcMatrix = (MatrixObj*)pSubMatrix->convert(m_type);
		else
			pSrcMatrix = (MatrixObj*)pSubMatrix;
		
		// Create a size vector for the sub matrix and reserve space for each dimension
		DimVector subSize;
		subSize.reserve(pSlice->getSize());
		
		// Initialize the dimension iterator
		DimVector::const_iterator dimItr = m_size.begin();
		
		// For each dimension of the slice
		for (size_t i = 0; i < pSlice->getSize(); ++i)
		{		
			// Get the slice along the current dimension
			const DataObject* pCurSlice = pSlice->getObject(i);
			
			// Extract the size along this dimension
			size_t dimSize = *(dimItr++);
			
			// Declare a variable for the element count along this dimension
			size_t dimCount;

			// If the object is a matrix
			if (pCurSlice->isMatrixObj())
			{
				// Get a typed pointer to the matrix object
				const BaseMatrixObj* pMatrix = (BaseMatrixObj*)pCurSlice;
				
				// There are as many elements as indices
				dimCount = pMatrix->getNumElems();
			}
			
			// If the object is a range
			else if (pCurSlice->getType() == DataObject::RANGE)
			{
				// Get a typed pointer to the range object
				const RangeObj* pRange = (RangeObj*)pCurSlice;
				
				// If this is the full range
				if (pRange->isFullRange())
				{
					// Set the count to the size of this dimension
					dimCount = dimSize;
					
					// If this is the last dimension of the multislice
					if (i == pSlice->getSize() - 1)
					{
						// Compute the size of the extended last dimension
						for (; dimItr != m_size.end(); ++dimItr)
							dimCount *= *dimItr;
					}
				}
				else
				{
					// There are as many elements as numbers in the range
					dimCount = pRange->getElemCount();
				}
			}
			
			// Otherwise, for any other object type
			else
			{
				// Break an assertion
				assert (false);
			}

			// Add the size of this dimension to the sub-matrix's size vector
			subSize.push_back(dimCount);
		}
	
		// Compute the number of elements in the slice
		size_t elemCount = subSize.size()? subSize[0]:0;
		for (size_t i = 1; i < subSize.size(); ++i)
			elemCount *= subSize[i];
		
		// If the input matrix is scalar but this one is not
		if (pSrcMatrix->isScalar() && !isScalar())
		{
			// Make the source matrix match the slice dimensions
			pSrcMatrix = new MatrixObj(elemCount, 1, pSrcMatrix->getScalar());
		}
		
		// If the size of the submatrix doesn't match the computed size, throw an exception
		if (pSrcMatrix->getNumElems() != elemCount)	
			throw RunError("incompatible matrix size in matrix assignment");
		
		// If the sub-matrix is empty, return early
		if (pSrcMatrix->isEmpty())
			return;

		// Get a pointer to the current element in the sub matrix
		ScalarType* pSrcElem = pSrcMatrix->m_pElements;
		
		// Allocate memory for the indices
		size_t* pIndices = (size_t*)alloca(sizeof(size_t) * pSlice->getSize());
		
		// Copy the slice recursively
		setSliceND(pSlice, pSlice->getSize() - 1, pIndices, pSrcElem);		
	}
	
	// Helper method to recursively implement slice copying
	void setSliceND(const ArrayObj* pSlice, size_t curDim, size_t* pIndices, ScalarType*& pSrcElem) const
	{
		// Get a reference to the slice along the current dimension
		const DataObject* pCurSlice = pSlice->getObject(curDim);
		
		// Compute the index of the next lower dimension
		size_t nextDim = curDim - 1;
		
		// If we are not at the lowest dimension
		if (curDim > 0)
		{
			// If the object is a matrix
			if (pCurSlice->getType() == DataObject::MATRIX_F64)
			{
				// Get a typed pointer to the matrix object
				MatrixObj<float64>* pMatrix = (MatrixObj<float64>*)pCurSlice;
				
				// Get pointers to the start and end values
				const float64* pStartVal = pMatrix->getElements();
				const float64* pEndVal = pStartVal + pMatrix->getNumElems();
				
				// For each value in the matrix
				for (const float64* pValue = pStartVal; pValue != pEndVal; ++pValue)
				{
					// Set the index for this dimension
					pIndices[curDim] = toZeroIndex(size_t(*pValue));
						
					// Recurse for the next lower dimension
					setSliceND(pSlice, nextDim, pIndices, pSrcElem);					
				}
			}
			
			// If the object is a range
			else if (pCurSlice->getType() == DataObject::RANGE)
			{
				// Get a typed pointer to the range object
				const RangeObj* pRange = (RangeObj*)pCurSlice;
				
				// If this is the full range
				if (pRange->isFullRange())
				{
					// Set the element count to the size of this dimension
					size_t elemCount = m_size[curDim];
					
					// If this is the last dimension of the multislice
					if (curDim == pSlice->getSize() - 1)
					{
						// Compute the size of the extended last dimension
						for (size_t dim = curDim + 1; dim < m_size.size(); ++dim)
							elemCount *= m_size[dim];
					}
					
					// For each element along this dimension
					for (size_t i = 0; i < elemCount; ++i)
					{
						// Set the index for this dimension
						pIndices[curDim] = i;
						
						// Recurse for the next lower dimension
						setSliceND(pSlice, nextDim, pIndices, pSrcElem);
					}					
				}
				else
				{
					// Initialize the current value to the start value
					double value = pRange->getStartVal();
				
					// Get the element count for the range
					size_t elemCount = pRange->getElemCount();
					
					// For each range element
					for (size_t i = 1; i <= elemCount; ++i)
					{
						// Set the index for this dimension
						pIndices[curDim] = toZeroIndex(size_t(value));
							
						// Recurse for the next lower dimension
						setSliceND(pSlice, nextDim, pIndices, pSrcElem);
						
						// Update the current value
						value += pRange->getStepVal();
					}					
				}
			}
			
			// Otherwise, for any other object type
			else
			{
				// Break an assertion
				assert (false);
			}
		}

		// Otherwise, we are at the lowest dimension
		else
		{
			// Initialize the base address pointer
			ScalarType* pBaseAddr = m_pElements;
			
			// Initialize the dimensional offset product
			size_t dimOffset = 1;
			
			// For each dimension except the first
			for (size_t i = 1; i < pSlice->getSize(); ++i)
			{			
				// Update the dimensional offset product
				dimOffset *= m_size[i - 1];
				
				// Update the base address pointer
				pBaseAddr += pIndices[i] * dimOffset;				
			}	
					
			// If the object is a matrix
			if (pCurSlice->getType() == DataObject::MATRIX_F64)
			{
				// Get a typed pointer to the matrix object
				MatrixObj<float64>* pMatrix = (MatrixObj<float64>*)pCurSlice;

				// Get pointers to the start and end values
				const float64* pStartVal = pMatrix->getElements();
				const float64* pEndVal = pStartVal + pMatrix->getNumElems();
								
				// For each value in the matrix
				for (const float64* pValue = pStartVal; pValue != pEndVal; ++pValue)
				{
					// Convert the index to zero indexing
					size_t zeroIndex = toZeroIndex(size_t(*pValue));
					
					// Compute the destination address
					ScalarType* pDstAddr = pBaseAddr + zeroIndex;
					
					// Copy the element
					*pDstAddr = *pSrcElem;
					
					// Increment the source element pointer
					++pSrcElem;
				}
			}
			
			// If the object is a range
			else if (pCurSlice->getType() == DataObject::RANGE)
			{
				// Get a typed pointer to the range object
				const RangeObj* pRange = (RangeObj*)pCurSlice;
				
				// If this is the full range
				if (pRange->isFullRange())
				{
					// Set the element count to the size of this dimension
					size_t elemCount = m_size[curDim];
					
					// If this is the last dimension of the multislice
					if (curDim == pSlice->getSize() - 1)
					{
						// Compute the size of the extended last dimension
						for (size_t dim = curDim + 1; dim < m_size.size(); ++dim)
							elemCount *= m_size[dim];
					}
					
					// For each element along this dimension
					for (size_t i = 0; i < elemCount; ++i)
					{
						// Compute the destination address
						ScalarType* pDstAddr = pBaseAddr + i;
						
						// Copy the element
						*pDstAddr = *pSrcElem;						
						
						// Increment the source element pointer
						++pSrcElem;						
					}
				}
				else
				{
					// Initialize the current value to the start value
					double value = pRange->getStartVal();
				
					// Get the element count for the range
					size_t elemCount = pRange->getElemCount();
					
					// For each range element
					for (size_t i = 1; i <= elemCount; ++i)
					{
						// Convert the index to zero indexing
						size_t zeroIndex = toZeroIndex(size_t(value));
						
						// Compute the destination address
						ScalarType* pDstAddr = pBaseAddr + zeroIndex;
						
						// Copy the element
						*pDstAddr = *pSrcElem;
						
						// Increment the source element pointer
						++pSrcElem;
						
						// Update the current range value
						value += pRange->getStepVal();
					}
				}
			}
			
			// Otherwise, for any other object type
			else
			{
				// Break an assertion
				assert (false);
			}		
		}
	}	
	
	// Method to concatenate this matrix with another matrix
	virtual BaseMatrixObj* concat(const BaseMatrixObj* pOther, size_t catDim) const
	{
		// Declare a pointer for the other matrix
		MatrixObj* pOtherMat;
				
		// If the other matrix does not have the same type as this one
		if (m_type != pOther->getType())
		{
			// If the other matrix is a complex matrix
			if (pOther->getType() == MATRIX_C128)
			{
				// Convert this matrix into a complex matrix and perform the operation
				return MatrixObj<Complex128>::concat(
					(MatrixObj<Complex128>*)convert(MATRIX_C128), 
					(MatrixObj<Complex128>*)pOther,
					catDim
				);
			}

			// Convert the other matrix to the local type
			pOtherMat = (MatrixObj*)pOther->convert(m_type);
		}
		else
		{
			// Cast the pointer to the local type
			pOtherMat = (MatrixObj*)pOther;
		}
		
		// Perform the concatenation
		return concat(this, pOtherMat, catDim);
	}
	
	// Method to get an element unidimensionally
	ScalarType getElem1D(size_t index) const
	{
		// Convert to zero indexing
		index = toZeroIndex(index);
		
		// Ensure that the index is valid
		assert (index < m_numElements);
		
		// Return the desired element
		return m_pElements[index];
	}
	
	// Method to set an element unidimensionally
	void setElem1D(size_t index, ScalarType value)
	{
		// Convert to zero indexing
		index = toZeroIndex(index);
		
		// Ensure that the index is valid
		assert (index < m_numElements);

		// Set the desired element
		m_pElements[index] = value;
	}

	// Method to get an element bidimensionally
	ScalarType getElem2D(size_t rowIndex, size_t colIndex) const
	{
		// Convert to zero indexing
		rowIndex = toZeroIndex(rowIndex);
		colIndex = toZeroIndex(colIndex);
		
		// Ensure the indices are valid
		assert (m_numElements > 0 && rowIndex < m_size[0] && colIndex < m_size[1]);
		
		// Compute the element index
		size_t index = rowIndex + colIndex * m_size[0];
		
		// Ensure the index is valid
		assert (index < m_numElements);
		
		// Return the desired element
		return m_pElements[index];
	}

	// Method to set an element bidimensionally
	void setElem2D(size_t rowIndex, size_t colIndex, ScalarType value)
	{
		// Convert to zero indexing
		rowIndex = toZeroIndex(rowIndex);
		colIndex = toZeroIndex(colIndex);		
		
		// Ensure the indices are valid
		assert (m_numElements > 0 && rowIndex < m_size[0] && colIndex < m_size[1]);
		
		// Compute the element index
		size_t index = rowIndex + colIndex * m_size[0];
		
		// Ensure the index is valid
		assert (index < m_numElements);
		
		// Set the desired element
		m_pElements[index] = value;
	}
	
	// Method get an element multidimensionally
	void setElemND(const DimVector& indices, ScalarType value)
	{
		// Ensure that the index is valid
		assert (boundsCheckND(indices));
		
		// Initialize the index iterator
		DimVector::const_iterator indItr = indices.begin();
		
		// Initialize the global index
		size_t index = *(indItr++);
		
		// Convert the initial index to zero indexing
		index = toZeroIndex(index);
		
		// Initialize the dimension iterator
		DimVector::const_iterator dimItr = m_size.begin();
		
		// Initialize the offset product
		size_t offset = 1;
		
		// For each of the indices
		while (indItr != indices.end())
		{
			// Extract the size of the current dimension
			size_t dimSize = *(dimItr++);
			
			// Update the offset product
			offset *= dimSize;
			
			// Extract the index along the current dimension
			size_t dimIndex = *(indItr++);					
			
			// Convert the dimensional index to zero indexing
			dimIndex = toZeroIndex(dimIndex);
			
			// Update the index sum
			index += dimIndex * offset;
		}

		// Ensure that the global index is valid
		assert (index < m_numElements);
		
		// Set the desired element
		m_pElements[index] = value;
	}

	// Method get an element multidimensionally
	ScalarType getElemND(const DimVector& indices) const
	{
		// Ensure that the index is valid
		assert (boundsCheckND(indices));
		
		// Initialize the index iterator
		DimVector::const_iterator indItr = indices.begin();
		
		// Initialize the global index
		size_t index = *(indItr++);
		
		// Convert the initial index to zero indexing
		index = toZeroIndex(index);
		
		// Initialize the dimension iterator
		DimVector::const_iterator dimItr = m_size.begin();
		
		// Initialize the offset product
		size_t offset = 1;
		
		// For each of the indices
		while (indItr != indices.end())
		{
			// Extract the size of the current dimension
			size_t dimSize = *(dimItr++);
			
			// Update the offset product
			offset *= dimSize;
			
			// Extract the index along the current dimension
			size_t dimIndex = *(indItr++);					
			
			// Convert the dimensional index to zero indexing
			dimIndex = toZeroIndex(dimIndex);
			
			// Update the index sum
			index += dimIndex * offset;
		}

		// Ensure that the global index is valid
		assert (index < m_numElements);
		
		// Return the desired element
		return m_pElements[index];
	}
	
	// Static method to vectorize this matrix
	static MatrixObj* vectorize(const MatrixObj* pMatrix)
	{
		// Copy this matrix
		MatrixObj* pResult = pMatrix->copy();
		
		// Make the matrix size N x 1, where N is the number of elements
		pResult->m_size.resize(2);
		pResult->m_size[0] = pMatrix->m_numElements;
		pResult->m_size[1] = 1;
		
		// Return the result matrix
		return pResult;
	}	
	
	// Static method to obtain the conjugate transpose of a matrix
	static MatrixObj* conjTranspose(const MatrixObj* pMatrix)
	{
		// Ensure that the input matrix is square
		assert (pMatrix->m_size.size() == 2);
		
		// Create a new matrix object to store the result
		MatrixObj* pResult = new MatrixObj(pMatrix->m_size[1], pMatrix->m_size[0]);
		
		// Get the number of rows and columns
		size_t numRows = pMatrix->m_size[0];
		size_t numCols = pMatrix->m_size[1];		

		// For each row of the input matrix
		for (size_t i = 0; i < numRows; ++i)		
		{
			// For each column of the input matrix
			for (size_t j = 0; j < numCols; ++j)		
			{
				// Conjugate and transpose this element
				pResult->m_pElements[i * numCols + j] = std::conj(pMatrix->m_pElements[j * numRows + i]);
			}
		}
		
		// Return the output matrix
		return pResult;
	}
	
	// Static method to obtain the transpose of a matrix
	static MatrixObj* transpose(const MatrixObj* pMatrix)
	{
		// Ensure that the input matrix is square
		assert (pMatrix->m_size.size() == 2);
		
		// Create a new matrix object to store the result
		MatrixObj* pResult = new MatrixObj(pMatrix->m_size[1], pMatrix->m_size[0]);
		
		// Get the number of rows and columns
		size_t numRows = pMatrix->m_size[0];
		size_t numCols = pMatrix->m_size[1];		

		// For each row of the input matrix
		for (size_t i = 0; i < numRows; ++i)		
		{
			// For each column of the input matrix
			for (size_t j = 0; j < numCols; ++j)		
			{
				// Transpose this element
				pResult->m_pElements[i * numCols + j] = pMatrix->m_pElements[j * numRows + i];
			}
		}
		
		// Return the output matrix
		return pResult;
	}
	
	// Static method to perform matrix multiplication
	static MatrixObj* matrixMult(const MatrixObj* pMatrixA, const MatrixObj* pMatrixB)
	{
		// Default version unimplemented, see specialized versions
		assert (false);
	}
	
	// Static method to perform scalar multiplication
	static MatrixObj* scalarMult(const MatrixObj* pMatrix, ScalarType scalar)
	{
		// Default version unimplemented, see specialized versions
		assert (false);
	}

	// Static templated method to perform scalar multiplication with a specific scalar type
	template <class InScalarType> static MatrixObj* scalarMult(const MatrixObj* pMatrix, InScalarType scalar)
	{
		// Perform the scalar multiplication
		return scalarMult(pMatrix, (ScalarType)scalar);
	}
	
	// Static method to perform matrix left division
	static MatrixObj* matrixLeftDiv(const MatrixObj* pMatrixA, const MatrixObj* pMatrixB)
	{
		// Default version unimplemented, see specialized versions
		assert (false);
	}	
	
	// Static method to perform matrix right division
	static MatrixObj* matrixRightDiv(const MatrixObj* pMatrixA, const MatrixObj* pMatrixB)
	{
		// Default version unimplemented, see specialized versions
		assert (false);
	}	
	
	// Static templated method to perform an array (per-element) operation on one matrix
	template <class UnaryOp, class OutType> static MatrixObj<OutType>* arrayOp(const MatrixObj* pMatrix)
	{
		// Create a new matrix object to store the result
		MatrixObj<OutType>* pResult = new MatrixObj<OutType>(pMatrix->m_size);
			
		// Compute a pointer to the last matrix element of the matrix
		const ScalarType* pLastElem = pMatrix->m_pElements + pMatrix->m_numElements;
			
		// For each matrix element of the input and output matrices
		ScalarType* pIn = pMatrix->m_pElements;
		OutType* pOut = pResult->getElements();
		for (; pIn < pLastElem; ++pIn, ++pOut)		
		{
			// Perform the exponentiation
			*pOut = (OutType)UnaryOp::op(*pIn);
		}
			
		// Return a pointer to the result matrix
		return pResult;
	}
	
	// Static templated method to perform an array (per-element) operation between a scalar lhs and a matrix rhs
	template <class BinaryOp, class OutType, class InScalarType> static MatrixObj<OutType>* lhsScalarArrayOp(const MatrixObj* pMatrixR, InScalarType scalarL)
	{
		// Create a new matrix object to store the result
		MatrixObj<OutType>* pResult = new MatrixObj<OutType>(pMatrixR->m_size);
		
		// Compute a pointer to the last matrix element of the right matrix
		const ScalarType* pLastElem = pMatrixR->m_pElements + pMatrixR->m_numElements;
		
		// For each matrix element of the input and output matrices
		ScalarType* pIn = pMatrixR->m_pElements;
		OutType* pOut = pResult->getElements();
		for (; pIn < pLastElem; ++pIn, ++pOut)		
		{
			// Perform the exponentiation
			*pOut = (OutType)BinaryOp::op(scalarL, *pIn);
		}
		
		// Return a pointer to the result matrix
		return pResult;
	}
	
	// Static templated method to perform an array (per-element) operation between a matrix lhs and a scalar rhs
	template <class BinaryOp, class OutType, class InScalarType> static MatrixObj<OutType>* rhsScalarArrayOp(const MatrixObj* pMatrixL, InScalarType scalarR)
	{
		// Create a new matrix object to store the result
		MatrixObj<OutType>* pResult = new MatrixObj<OutType>(pMatrixL->m_size);
		
		// Compute a pointer to the last matrix element of the right matrix
		const ScalarType* pLastElem = pMatrixL->m_pElements + pMatrixL->m_numElements;
		
		// For each matrix element of the input and output matrices
		ScalarType* pIn = pMatrixL->m_pElements;
		OutType* pOut = pResult->getElements();
		for (; pIn < pLastElem; ++pIn, ++pOut)		
		{
			// Perform the exponentiation
			*pOut = (OutType)BinaryOp::op(*pIn, scalarR);
		}
		
		// Return a pointer to the result matrix
		return pResult;
	}	
		
	// Static templated method to perform an array (per-element) operation on two matrices
	template <class BinaryOp, class OutType> static MatrixObj<OutType>* binArrayOp(const MatrixObj* pMatrixA, const MatrixObj* pMatrixB)
	{
		// If the left matrix is a scalar
		if (pMatrixA->isScalar())
		{
			// Perform the operation with a scalar lhs and a matrix rhs
			return lhsScalarArrayOp<BinaryOp, OutType, ScalarType>(pMatrixB, pMatrixA->getScalar());
		}
		
		// If the right matrix is a scalar
		else if (pMatrixB->isScalar())
		{
			// Perform the operation with a matrix lhs and a scalar rhs
			return rhsScalarArrayOp<BinaryOp, OutType, ScalarType>(pMatrixA, pMatrixB->getScalar());
		}
		
		// Otherwise, if both matrices are non-scalar
		else
		{
			// If both matrices do not have the same size, throw an exception
			if (pMatrixA->m_size != pMatrixB->m_size)
				throw RunError("matrix dimensions do not match");
			
			// Create a new matrix object to store the result
			MatrixObj<OutType>* pResult = new MatrixObj<OutType>(pMatrixA->m_size);
			
			// Compute a pointer to the last matrix element
			const ScalarType* pLastElem = pMatrixA->m_pElements + pMatrixA->m_numElements;
			
			// For each matrix element of the input and output matrices
			ScalarType* pInA = pMatrixA->m_pElements;
			ScalarType* pInB = pMatrixB->m_pElements;
			OutType* pOut = pResult->getElements();
			for (; pInA < pLastElem; ++pInA, ++pInB, ++pOut)		
			{
				// Perform the exponentiation
				*pOut = (OutType)BinaryOp::op(*pInA, *pInB);
			}		
			
			// Return a pointer to the result matrix
			return pResult;
		}
	}

	// Static templated method to perform a vector (per-vector) operation on a matrix
	template <class VectorOp, class OutType> static MatrixObj<OutType>* vectorOp(const MatrixObj* pInMatrix, size_t opDim)
	{
		// Ensure the operating dimension is valid
		assert (opDim < pInMatrix->m_size.size());
		
		// If the input matrix is empty, return an empty matrix
		if (pInMatrix->isEmpty())
			return new MatrixObj<OutType>(pInMatrix->getSize());
		
		// Get the operating dimension length
		size_t opDimLength = pInMatrix->m_size[opDim];
		
		// Compute the stride along the operating dimension
		size_t opDimStride = 1;
		for (size_t i = 0; i < opDim; ++i)
			opDimStride *=  pInMatrix->m_size[i];
		
		// Compute the size of the output matrix
		DimVector outSize = pInMatrix->m_size;
		outSize[opDim] = 1;
		
		// Create a new matrix to store the output
		MatrixObj<OutType>* pOutMatrix = new MatrixObj<OutType>(outSize);
			
		// Initialize the output pointer
		OutType* pOut = pOutMatrix->getElements();
		
		// Create and initialize a vector for the indices
		DimVector indices(pInMatrix->m_size.size(), 0);
		
		// Initialize the current dimension index
		size_t curDim = 0;
		
		// For each vector
		for (;;)
		{				
			// Compute the current vector index
			size_t index = 0;
			size_t offset = 1;
			for (size_t i = 0; i < indices.size(); ++i)
			{
				index += indices[i] * offset;
				offset *= pInMatrix->m_size[i];
			}
			
			// Compute the vector start and end addresses
			ScalarType* pVecStart = pInMatrix->m_pElements + index;
			ScalarType* pVecEnd = pVecStart + (opDimLength * opDimStride);
			
			// Restrict the vector end to be within the matrix boundary
			pVecEnd = std::min(pVecEnd, pInMatrix->m_pElements + pInMatrix->m_numElements);
			
			// Ensure that the output pointer is valid
			assert (pOut < pOutMatrix->getElements() + pOutMatrix->getNumElems());
			
			// Perform the operation on this vector
			*pOut = (OutType)VectorOp::op(pVecStart, pVecEnd, opDimStride);
			
			// Increment the output pointer
			++pOut;

			// Until we are done moving to the next slice
			while (curDim < indices.size())
			{
				// If this is the operating dimension
				if (curDim == opDim)
				{
					// Move to the next dimension
					curDim++;
					
					// If the current dimension is no longer valid, stop the process
					if (curDim >= indices.size())
						break;
				}
	
				// Move to the next index along this dimension
				indices[curDim]++;
				
				// If the index along this dimension is still valid
				if (indices[curDim] < pInMatrix->m_size[curDim])
				{
					// Go back to the first dimension
					curDim = 0;
					
					// Stop the process
					break;
				}
				else
				{
					// Move to the next dimension
					curDim++;
					
					// Reset the indices for prior dimensions
					for (size_t i = 0; i < curDim; ++i)
						indices[i] = 0;
				}				
			}
			
			// If there are no more indices, stop
			if (curDim >= indices.size())
				break;
		}
		
		// Return the output matrix
		return pOutMatrix;
	}
	
	// Static method to concatenate two matrices along a specified dimension
	static MatrixObj* concat(const MatrixObj* pMatrixA, const MatrixObj* pMatrixB, size_t catDim)
	{
		// Get the size of both matrices
		const DimVector& sizeA = pMatrixA->getSize();
		const DimVector& sizeB = pMatrixB->getSize();
		
		// Ensure that both matrices have the same number of dimensions
		assert (sizeA.size() == sizeB.size());

		// Ensure that the concatenation dimension is valid 
		assert (catDim <= sizeA.size());
		
		// If A is empty, return a copy of B
		if (pMatrixA->isEmpty())
			return pMatrixB->copy();
		
		// If B is empty, return a copy of A
		if (pMatrixB->isEmpty())
			return pMatrixA->copy();

		// Ensure that all dimensions have the same size, except the concat dimension
		for (size_t i = 0; i < sizeA.size(); ++i)
		{
			if (sizeA[i] != sizeB[i] && i != catDim)
				assert (false);
		}
		
		// Create a vector for the size of the result matrix
		DimVector newSize = sizeA;
		
		// If the vector is not large enough, resize it
		if (catDim >= newSize.size())
			newSize.resize(catDim + 1, 1);
		
		// Update the result size along the concatenation dimension
		newSize[catDim] += (catDim < sizeB.size())? sizeB[catDim]:1;
		
		// Create a matrix to store the result
		MatrixObj* pResult = new MatrixObj(newSize);
		
		// If the result matrix is isEmpty, return early
		if (pResult->isEmpty())
			return pResult;
		
		// Compute the slice size for A
		size_t sliceSizeA = 1;
		for (size_t i = 0; i <= catDim && i < sizeA.size(); ++i)
			sliceSizeA *= sizeA[i];
		
		// Compute the slice size for B
		size_t sliceSizeB = 1;
		for (size_t i = 0; i <= catDim && i < sizeB.size(); ++i)
			sliceSizeB *= sizeB[i];
		
		// Compute the slice size for the result matrix
		size_t sliceSizeR = sliceSizeA + sliceSizeB;
		
		// Compute the number of slices
		size_t numSlices = 1;
		for (size_t i = catDim + 1; i < sizeA.size(); ++i)
			numSlices *= sizeA[i];
		
		// For each slice
		for (size_t i = 0; i < numSlices; ++i)
		{
			// Compute the address of the result slice
			ScalarType* pSliceR = pResult->m_pElements + i * sliceSizeR;
			
			// If matrix A has a nonzero slice size
			if (sliceSizeA > 0)
			{
				// Compute the address of this slice in A
				const ScalarType* pSliceA = pMatrixA->m_pElements + i * sliceSizeA;
				
				// Copy the data into the result matrix
				memcpy(pSliceR, pSliceA, sizeof(ScalarType) * sliceSizeA);
			}
			
			// If matrix B has a nonzero slice size
			if (sliceSizeB > 0)
			{
				// Compute the address of this slice in B
				const ScalarType* pSliceB = pMatrixB->m_pElements + i * sliceSizeB;
				
				// Copy the data into the result matrix
				memcpy(pSliceR + sliceSizeA, pSliceB, sizeof(ScalarType) * sliceSizeB);
			}			
		}

		// Return the result matrix
		return pResult;
	}
	
	// Method to read an element using 1D indexing
	static ScalarType readElem1D(const MatrixObj* pMatrix, int64 index)
	{
		// Convert the index to zero indexing
		size_t zeroIndex = toZeroIndex(size_t(index));
		
		// If the index is out of range, throw an exception
		if (zeroIndex >= pMatrix->m_numElements)
			throw RunError("index out of range in 1D matrix read");
		
		// Get a pointer to the matrix data
		const ScalarType* pData = pMatrix->getElements();
		
		// Read the element at the index
		return pData[zeroIndex];
	}
	
	// Method to read an element using 2D indexing
	static ScalarType readElem2D(const MatrixObj* pMatrix, int64 index1, int64 index2)
	{
		// Convert the indices to zero indexing
		size_t zeroIndex1 = toZeroIndex(size_t(index1));
		size_t zeroIndex2 = toZeroIndex(size_t(index2));
		
		// Compute the element offset
		size_t offset = zeroIndex2 * pMatrix->m_size[0] + zeroIndex1;
		
		// If the indices are out of range, throw an exception
		if (zeroIndex1 >= pMatrix->m_size[0] || offset >= pMatrix->m_numElements)
			throw RunError("index out of range in 2D matrix read");
		
		// Get a pointer to the matrix data
		const ScalarType* pData = pMatrix->getElements();
		
		// Read the element at the offset
		return pData[offset];
	}
	
	// Method to write an element using 1D indexing
	static void writeElem1D(MatrixObj* pMatrix, int64 index, ScalarType value)
	{
		// Convert the index to zero indexing
		size_t zeroIndex = toZeroIndex(size_t(index));
				
		// If the index is out of range
		if (zeroIndex >= pMatrix->m_numElements)
		{
			// If the index is non-positive
			if (index <= 0)
				throw RunError("non-positive index in 1D matrix write");
			
			// Expand the matrix
			pMatrix->expand(DimVector(1, index));
		}
		
		// Get a pointer to the matrix data
		ScalarType* pData = pMatrix->getElements();
		
		// Write the element at the index
		pData[zeroIndex] = value;
	}
	
	// Method to write an element using 2D indexing
	static void writeElem2D(MatrixObj* pMatrix, int64 index1, int64 index2, ScalarType value)
	{
		// Convert the indices to zero indexing
		size_t zeroIndex1 = toZeroIndex(size_t(index1));
		size_t zeroIndex2 = toZeroIndex(size_t(index2));
				
		// Compute the element offset
		size_t offset = zeroIndex2 * pMatrix->m_size[0] + zeroIndex1;
		
		// If the indices are out of range
		if (zeroIndex1 >= pMatrix->m_size[0] || offset >= pMatrix->m_numElements)
		{
			// If one of the indices is non-positive
			if (index1 <= 0 || index2 <= 0)
				throw RunError("non-positive index in 2D matrix write");
			
			// Expand the matrix dimensions
			DimVector newSize;
			newSize.push_back(index1);
			newSize.push_back(index2);
			pMatrix->expand(newSize);
			
			// Recompute the element offset
			offset = zeroIndex2 * pMatrix->m_size[0] + zeroIndex1;
		}
		
		// Get a pointer to the matrix data
		ScalarType* pData = pMatrix->getElements();
		
		// Write the element at the offset
		pData[offset] = value;
	}
	
	// Method to test if the matrix is diagonal
	bool isDiagonal() const
	{
		// If the matrix is not square, it is not diagonal
		if (!isSquare())
			return false;
		
		// Get the number of columns and rows
		size_t numCols = m_size[0];
		size_t numRows = m_size[1];
		
		// For each row
		for (size_t r = 1; r <= numRows; ++r)
		{
			// For each column
			for (size_t c = 1; c <= numRows; ++c)
			{
				// Test that only diagonal elements are nonzero
				if (getElem2D(r, c) != 0 && c != r)
					return false;
			}
		}
		
		// The matrix is diagonal
		return true;
	}
	
	// Accessor to get the first matrix element
	ScalarType getScalar() const { return *m_pElements; }
	
	// Accessor to access the matrix elements of a constant matrix
	const ScalarType* getElements() const { return m_pElements; }
	
	// Accessors to access the matrix elements
	ScalarType* getElements() { return m_pElements; }

	// Static method to get the first element of a matrix
	static ScalarType getScalarVal(const MatrixObj* pMatrix) { return pMatrix->getScalar(); }
	
	// Matrix binary operation function type definitions
	typedef MatrixObj* (*MATRIX_BINOP_FUNC)(const MatrixObj*, const MatrixObj*);
	typedef MatrixObj* (*SCALAR_BINOP_FUNC)(const MatrixObj*, ScalarType);
	typedef MatrixObj* (*F64_SCALAR_BINOP_FUNC)(const MatrixObj*, float64);
	typedef MatrixObj<bool>* (*MATRIX_LOGIC_OP_FUNC)(const MatrixObj*, const MatrixObj*);
	typedef MatrixObj<bool>* (*SCALAR_LOGIC_OP_FUNC)(const MatrixObj*, ScalarType);
	typedef MatrixObj<bool>* (*F64_SCALAR_LOGIC_OP_FUNC)(const MatrixObj*, float64);
	
	// Matrix read/write function type definitions
	typedef ScalarType (*MATRIX_1D_READ_FUNC)(const MatrixObj*, int64);
	typedef ScalarType (*MATRIX_2D_READ_FUNC)(const MatrixObj*, int64, int64);
	typedef void (*MATRIX_1D_WRITE_FUNC)(MatrixObj*, int64, ScalarType);
	typedef void (*MATRIX_2D_WRITE_FUNC)(MatrixObj*, int64, int64, ScalarType);

protected:
		
	// Method to allocate data for the matrix
	void allocMatrix()
	{
		// Compute the number of matrix elements
		m_numElements = m_size[0];
		for (size_t i = 1; i < m_size.size(); ++i)
			m_numElements *= m_size[i];
		
		// Allocate memory for the matrix elements
		// Note that the memory is garbage-collected
		m_pElements = (ScalarType*)GC_MALLOC_ATOMIC_IGNORE_OFF_PAGE(m_numElements * sizeof(ScalarType));
	}
	
	// Method to initialize the matrix
	void initMatrix(ScalarType value)
	{
		// Initialize all matrix elements to this value
		std::uninitialized_fill(m_pElements, m_pElements + m_numElements, value);
	}
	
	// Method to initialize a range, used for matrix expansion
	void initRange(ScalarType* pStart, ScalarType* pEnd)
	{
		// Fill the range with zero values
		std::uninitialized_fill(pStart, pEnd, (ScalarType)0);
	}
	
	// Array of matrix element
	// Note: the elements are stored in column-major order
	ScalarType* m_pElements;
};

// Template specialization of the class type method for common matrix object types
template <> inline DataObject::Type MatrixObj<float32    >::getClassType() { return DataObject::MATRIX_F32; 	}
template <> inline DataObject::Type MatrixObj<float64    >::getClassType() { return DataObject::MATRIX_F64; 	}
template <> inline DataObject::Type MatrixObj<Complex128 >::getClassType() { return DataObject::MATRIX_C128; 	}
template <> inline DataObject::Type MatrixObj<bool       >::getClassType() { return DataObject::LOGICALARRAY;	}
template <> inline DataObject::Type MatrixObj<char       >::getClassType() { return DataObject::CHARARRAY;  	}
template <> inline DataObject::Type MatrixObj<DataObject*>::getClassType() { return DataObject::CELLARRAY;  	}

// Template specialization of the conversion method
template <> DataObject* MatrixObj<Complex128>::convert(DataObject::Type outType) const;
template <> DataObject* MatrixObj<DataObject*>::convert(DataObject::Type outType) const;

// Template specialization of the matrix multiplication method
template <> MatrixObj<float64>* MatrixObj<float64>::matrixMult(const MatrixObj* pMatrixA, const MatrixObj* pMatrixB);
template <> MatrixObj<Complex128>* MatrixObj<Complex128>::matrixMult(const MatrixObj* pMatrixA, const MatrixObj* pMatrixB);

// Template specialization of the scalar multiplication method
template <> MatrixObj<float64>* MatrixObj<float64>::scalarMult(const MatrixObj* pMatrix, float64 scalar);
template <> MatrixObj<Complex128>* MatrixObj<Complex128>::scalarMult(const MatrixObj* pMatrix, Complex128 scalar);

// Template specialization of the matrix left-division method
template <> MatrixObj<float64>* MatrixObj<float64>::matrixLeftDiv(const MatrixObj* pMatrixA, const MatrixObj* pMatrixB);
template <> MatrixObj<Complex128>* MatrixObj<Complex128>::matrixLeftDiv(const MatrixObj* pMatrixA, const MatrixObj* pMatrixB);

// Template specialization of the matrix right-division method
template <> MatrixObj<float64>* MatrixObj<float64>::matrixRightDiv(const MatrixObj* pMatrixA, const MatrixObj* pMatrixB);
template <> MatrixObj<Complex128>* MatrixObj<Complex128>::matrixRightDiv(const MatrixObj* pMatrixA, const MatrixObj* pMatrixB);

// Template specialization of the range initialization method
template <> void MatrixObj<DataObject*>::initRange(DataObject** pStart, DataObject** pEnd);

// Matrix object type definitions
typedef MatrixObj<float32> MatrixF32Obj;
typedef MatrixObj<float64> MatrixF64Obj;
typedef MatrixObj<Complex128> MatrixC128Obj;

// Logical array type definition
typedef MatrixObj<bool> LogicalArrayObj;

#endif // #ifndef MATRIXOBJS_H_
