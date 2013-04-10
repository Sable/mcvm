/***************************************************************
*
* The programming code contained in this file was written
* by Maxime Chevalier-Boisvert. For information, please
* contact the author.
*
* Contact information:
* e-mail: mcheva@cs.mcgill.ca
* phone : 1-514-935-2809
*
***************************************************************/

// Header files
#include <algorithm>
#include "matrixobjs.h"
#include "matrixops.h"

// Include the blas and lapack header files (C-specific)
#ifdef MCVM_USE_CLAPACK
extern "C"
{
	#include <cblas.h>
	#include <f2c.h>
	#include <clapack.h>
};
#endif

#ifdef MCVM_USE_ACML
extern "C"{
#include <acml.h>
};
#define max(a,b) ((a) >= (b) ? (a) : (b))
#endif

#ifdef MCVM_USE_LAPACKE
extern "C"{
#include <cblas.h>
#include <lapacke.h>
};
#define max(a,b) ((a) >= (b) ? (a) : (b))
#endif

#ifdef MCVM_USE_MKL
extern "C"{
#include <mkl_cblas.h>
#include <mkl_lapacke.h>
};
#define MCVM_USE_LAPACKE
#define max(a,b) ((a) >= (b) ? (a) : (b))
#endif
/***************************************************************
* Function: BaseMatrixObj::validIndices()
* Purpose : Test if slice indices are valid (positive integers)
* Initial : Maxime Chevalier-Boisvert on March 10, 2009
****************************************************************
Revisions and bug fixes:
*/
bool BaseMatrixObj::validIndices(const ArrayObj* pSlice) const
{
	// For each element of the slice
	for (size_t i = 0; i < pSlice->getSize(); ++i)
	{
		// Get the object
		DataObject* pObj = pSlice->getObject(i);
		
		// If the object is a matrix
		if (pObj->getType() == DataObject::MATRIX_F64)
		{
			// Get a typed pointer to the matrix object
			MatrixF64Obj* pMatrix = (MatrixF64Obj*)pObj;
			
			// Get pointers to the start and end values
			const float64* pStartVal = pMatrix->getElements();
			const float64* pEndVal = pStartVal + pMatrix->getNumElems();
			
			// For each value in the matrix
			for (const float64* pValue = pStartVal; pValue != pEndVal; ++pValue)
			{
				// If the value is non-positive, reject it
				if (*pValue <= 0)
					return false;
			}
		}

		// If the object is a logical array
		else if (pObj->getType() == DataObject::LOGICALARRAY)
		{
			// Do nothing, logical arrays are always valid indices
		}
		
		// If the object is a range
		else if (pObj->getType() == DataObject::RANGE)
		{
			// Get a typed pointer to the range object
			RangeObj* pRange = (RangeObj*)pObj;
			
			// If this is the full range
			if (pRange->isFullRange())
			{			
				// Do nothing, the full range is always valid
			}
			else
			{
				// If the range spans negative values, reject it
				if (pRange->getStartVal() <= 0 || pRange->getEndVal() <= 0)
					return false;
			}
		}
		
		// Otherwise, for any other object type
		else
		{
			// This is not a valid index list
			return false;
		}
	}	
	
	// All indices are valid
	return true;
}

/***************************************************************
* Function: BaseMatrixObj::getMaxIndices()
* Purpose : Compute the maximum indices for a slice
* Initial : Maxime Chevalier-Boisvert on February 16, 2009
****************************************************************
Revisions and bug fixes:
*/
DimVector BaseMatrixObj::getMaxIndices(const ArrayObj* pSlice, const BaseMatrixObj* pAssignMatrix) const
{		
	// Create a vector to store the max indices
	DimVector maxInds;
	maxInds.reserve(max(m_size.size(), pSlice->getSize()));
	
	// For each element of the slice
	for (size_t i = 0; i < pSlice->getSize(); ++i)
	{
		// Get the object
		DataObject* pObj = pSlice->getObject(i);

		// Declare a variable for the maximum index found
		size_t maxIndex = 0;
		
		// If the object is a matrix
		if (pObj->getType() == DataObject::MATRIX_F64)
		{
			// Get a typed pointer to the matrix object
			MatrixF64Obj* pMatrix = (MatrixF64Obj*)pObj;
			
			// Get pointers to the start and end values
			const float64* pStartVal = pMatrix->getElements();
			const float64* pEndVal = pStartVal + pMatrix->getNumElems();
			
			// For each value in the matrix
			for (const float64* pValue = pStartVal; pValue != pEndVal; ++pValue)
			{
				// Update the maximum index
				maxIndex = max(maxIndex, size_t(*pValue));
			}
		}

		// If the object is a logical array
		else if (pObj->getType() == DataObject::LOGICALARRAY)
		{
			// Get a typed pointer to the matrix object
			LogicalArrayObj* pMatrix = (LogicalArrayObj*)pObj;
				
			// Get pointers to the start and end values
			const bool* pStartVal = pMatrix->getElements();
			const bool* pEndVal = pStartVal + pMatrix->getNumElems();
			
			// For each value in the matrix
			for (const bool* pValue = pStartVal; pValue != pEndVal; ++pValue)
			{
				// If this value is true, increment the max index
				if (*pValue) ++maxIndex;
			}
		}
		
		// If the object is a range
		else if (pObj->getType() == DataObject::RANGE)
		{
			// Get a typed pointer to the range object
			RangeObj* pRange = (RangeObj*)pObj;
			
			// If this is the full range
			if (pRange->isFullRange())
			{			
				// If this is the last slice element but not the last matrix dimension
				if (i == pSlice->getSize() - 1 && i < m_size.size() - 1)
				{	
					// Compute the size of the last extended dimension
					maxIndex = m_size[i];
					for (size_t dim = i + 1; dim < m_size.size(); ++dim)
						maxIndex *= m_size[dim];
				}
				else
				{
					// Ensure that this is a valid matrix dimension
					assert (i < m_size.size());
									
					// Set the max index to the last element of this dimension
					maxIndex = m_size[i];
				}
				
				// If the max index is currently 0 and an assignment matrix is specified
				if (maxIndex == 0 && pAssignMatrix)
				{
					// Get the size of the assignment matrix
					const DimVector& assignSize = pAssignMatrix->getSize();
					
					// Update the max index based on the assignment matrix size
					if (i < assignSize.size())
					{
						// If this is the last slice element and the assignment matrix is a vector
						if (i == pSlice->getSize() - 1 && pAssignMatrix->isVector())
							maxIndex = pAssignMatrix->getNumElems();
						else
							maxIndex = assignSize[i];
					}
				}
			}
			else
			{
				// Get the maximum index
				maxIndex = max(maxIndex, size_t(pRange->getStartVal()));
				maxIndex = max(maxIndex, size_t(pRange->getEndVal()));
			}
		}
		
		// Otherwise, for any other object type
		else
		{
			// Break an assertion
			assert (false);
		}
		
		// If this is the last slice element but not the last matrix dimension
		if (i == pSlice->getSize() - 1 && i < m_size.size() - 1 && !(i == 0 && m_size[1] < 2))
		{			
			// Compute the number of times this index covers this dimension and the remainder
			size_t numCover = maxIndex / m_size[i] + 1;
			size_t numOver  = maxIndex % m_size[i];
			
			// If the remainder is 0
			if (numOver == 0)
			{
				// Set this element to the last along this dimension
				numOver = m_size[i];
				numCover -= 1;
			}
			
			// Store the max index for this dimension
			maxInds.push_back(numOver);
			
			// For all remaining dimensions
			for (size_t dimIndex = i + 1; dimIndex < m_size.size(); ++dimIndex)
			{		
				// If no previous dimensions are fully covered
				if (numCover == 0)
				{
					// Set the index to 1 for this dimension
					maxInds.push_back(1);
					
					// Move to the next dimension
					continue;					
				}
				
				// If this is the last matrix dimension
				if (dimIndex == m_size.size() - 1)
				{					
					// Add the last index to the vector
					maxInds.push_back(numCover);
				}
				else
				{
					// Store the previous coverage count
					size_t prevNumCover = numCover;
					
					// Update the coverage count and remainder
					numCover = prevNumCover / m_size[dimIndex] + 1;
					numOver  = prevNumCover % m_size[dimIndex];
					
					// If the remainder is 0
					if (numOver == 0)
					{
						// Set this element to the last along this dimension
						numOver = m_size[dimIndex];
						numCover -= 1;
					}
					
					// Store the max index for this dimension
					maxInds.push_back(numOver);
				}
			}
		}
		else
		{
			// Store the max index for this dimension
			maxInds.push_back(maxIndex);
		}
	}		
	
	// Return the max indices
	return maxInds;
}

/***************************************************************
* Function: BaseMatrixObj::boundsCheckND()
* Purpose : Test if a multidimensional index is valid
* Initial : Maxime Chevalier-Boisvert on February 16, 2009
****************************************************************
Revisions and bug fixes:
*/
bool BaseMatrixObj::boundsCheckND(const DimVector& indices) const
{
	// Ensure that the vector is not empty
	assert (indices.size() != 0);
	
	// For each dimension
	for (size_t i = 0; i < indices.size(); ++i)
	{
		// Extract the index 
		size_t index = indices[i];
		
		// If we are inside the matrix dimensions
		if (i < m_size.size())
		{
			// If this is not the last index
			if (i < indices.size())
			{
				// Ensure the index is within the size of this dimension
				if (index > m_size[i])
					return false;
			}
			
			// Otherwise, this is the last index
			else
			{
				// Compute the size of the last "virtual" dimension
				size_t dimSize = m_size[i];
				for (++i; i < m_size.size(); ++i)
					dimSize *= m_size[i];
				
				// Ensure the last index fits in the virtual dimension
				if (index > dimSize)
					return false;
			}
		}
		
		// Otherwise, we are past the matrix dimensions
		else
		{
			// Ensure the index is 0
			if (index > 0)
				return false;				
		}
	}		
	
	// The matrix bounds are respected
	return true;
}

/***************************************************************
* Function: BaseMatrixObj::expandMatrix()
* Purpose : Static method to expand a matrix
* Initial : Maxime Chevalier-Boisvert on July 15, 2009
****************************************************************
Revisions and bug fixes:
*/
void BaseMatrixObj::expandMatrix(BaseMatrixObj* pMatrix, size_t* pIndices, size_t numIndices)
{
	// Create a dimension vector
	DimVector newSize(pIndices, pIndices + numIndices);
	
	// Expand the matrix to the new size
	pMatrix->expand(newSize);	
}

/***************************************************************
* Function: BaseMatrixObj::multCompatible()
* Purpose : Test if matrices are compatible for multiplication
* Initial : Maxime Chevalier-Boisvert on February 16, 2009
****************************************************************
Revisions and bug fixes:
*/
bool BaseMatrixObj::multCompatible(const BaseMatrixObj* pMatrixA, const BaseMatrixObj* pMatrixB)
{
	// Ensure that both matrices are bidimensional
	if (pMatrixA->m_size.size() != 2 || pMatrixB->m_size.size() != 2)
		return false;
	
	// Test that both matrices are square with compatible inner dimensions
	return (pMatrixA->m_size[1] == pMatrixB->m_size[0]);
}

/***************************************************************
* Function: BaseMatrixObj::leftDivCompatible()
* Purpose : Test if matrices are compatible for left division
* Initial : Maxime Chevalier-Boisvert on March 3, 2009
****************************************************************
Revisions and bug fixes:
*/
bool BaseMatrixObj::leftDivCompatible(const BaseMatrixObj* pMatrixA, const BaseMatrixObj* pMatrixB)
{
	// Ensure that both matrices are bidimensional
	if (pMatrixA->m_size.size() != 2 || pMatrixB->m_size.size() != 2)
		return false;
	
	// Test that both matrices have the same number of rows and that b has only one column
	return (pMatrixA->m_size[0] == pMatrixB->m_size[0]);
}

/***************************************************************
* Function: MatrixObj<Complex128>::convert()
* Purpose : Type conversion for complex matrices
* Initial : Maxime Chevalier-Boisvert on March 11, 2009
****************************************************************
Revisions and bug fixes:
*/
template <> DataObject* MatrixObj<Complex128>::convert(DataObject::Type outType) const
{
	// Switch on the output type type
	switch (outType)
	{
		// Logical array
		case LOGICALARRAY:
		{
			// Create a logical array object of the same size
			MatrixObj<bool>* pOutput = new MatrixObj<bool>(m_size);
			
			// Convert each element of this matrix
			for (size_t i = 1; i <= m_numElements; ++i)
			{
				// Get the boolean value of this element
				bool boolVal = (getElem1D(i) != Complex128(0));
				
				// Set the value in the output
				pOutput->setElem1D(i, boolVal);
			}
			
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

/***************************************************************
* Function: MatrixObj<DayaObject*>::convert()
* Purpose : Type conversion for cell array objects
* Initial : Maxime Chevalier-Boisvert on March 3, 2009
****************************************************************
Revisions and bug fixes:
*/
template <> DataObject* MatrixObj<DataObject*>::convert(DataObject::Type outType) const
{
	// Conversion of cell arrays is not allowed
	return DataObject::convert(outType);
}

/***************************************************************
* Function: static MatrixObj<float64>::matrixMult()
* Purpose : Matrix multiplication of 64-bit float matrices
* Initial : Maxime Chevalier-Boisvert on March 3, 2009
****************************************************************
Revisions and bug fixes:
*/
template <> MatrixObj<float64>* MatrixObj<float64>::matrixMult(const MatrixObj* pMatrixA, const MatrixObj* pMatrixB)
{
	// Ensure that both matrices are square with compatible inner dimensions
	assert (multCompatible(pMatrixA, pMatrixB));
		
	// Create a new matrix object to store the result
	MatrixObj* pResult = new MatrixObj(pMatrixA->m_size[0], pMatrixB->m_size[1]);
	
	// If either of the input matrices are isEmpty, return early
	if (pMatrixA->isEmpty() || pMatrixB->isEmpty())
		return pResult;
#ifndef MCVM_USE_ACML	
	// Call the BLAS function to perform the multiplication
	// This computes: alpha*A*B + beta*C, A(m,k), B(k,n), C(m,n)
	cblas_dgemm(
		CblasColMajor,				// Column major storage
		CblasNoTrans,				// No transposition of A
		CblasNoTrans,				// No transposition of B
		pMatrixA->m_size[0],
		pMatrixB->m_size[1],
		pMatrixA->m_size[1],
		1.0,						// alpha = 1.0
		pMatrixA->m_pElements,
		pMatrixA->m_size[0],		// Stride of A
		pMatrixB->m_pElements,
		pMatrixB->m_size[0],		// Stride of B
		0.0,						// beta = 0.0
		pResult->m_pElements,		// C is the result
		pResult->m_size[0]			// Stride of result 
	);
#else
	dgemm('n','n',pMatrixA->m_size[0],pMatrixB->m_size[1],pMatrixA->m_size[1],1.0,pMatrixA->m_pElements,pMatrixA->m_size[0],pMatrixB->m_pElements,pMatrixB->m_size[0],0.0,pResult->m_pElements,pResult->m_size[0]);
#endif

	// Increment the matrix multiplication count
	PROF_INCR_COUNTER(Profiler::MATRIX_MULT_COUNT);
	
	// Return a pointer to the result matrix
	return pResult;
}

/***************************************************************
* Function: static MatrixObj<Complex128>::matrixMult()
* Purpose : Matrix multiplication of 128-bit complex matrices
* Initial : Maxime Chevalier-Boisvert on March 11, 2009
****************************************************************
Revisions and bug fixes:
*/
template <> MatrixObj<Complex128>* MatrixObj<Complex128>::matrixMult(const MatrixObj* pMatrixA, const MatrixObj* pMatrixB)
{
	// Ensure that both matrices are square with compatible inner dimensions
	assert (multCompatible(pMatrixA, pMatrixB));
	
	// Create a new matrix object to store the result
	MatrixObj* pResult = new MatrixObj(pMatrixA->m_size[0], pMatrixB->m_size[1]);
	
	// If either of the input matrices are isEmpty, return early
	if (pMatrixA->isEmpty() || pMatrixB->isEmpty())
		return pResult;
	
	// Create complex objects for the alpha and beta parameters
#ifndef MCVM_USE_ACML	
	// Call the BLAS function to perform the multiplication
	// This computes: alpha*A*B + beta*C, A(m,k), B(k,n), C(m,n)
	Complex128 alpha = 1.0;
	Complex128 beta = 0.0;
	cblas_zgemm(
		CblasColMajor,				// Column major storage
		CblasNoTrans,				// No transposition of A
		CblasNoTrans,				// No transposition of B
		pMatrixA->m_size[0],
		pMatrixB->m_size[1],
		pMatrixA->m_size[1],
		&alpha,						// alpha = 1.0
		pMatrixA->m_pElements,
		pMatrixA->m_size[0],		// Stride of A
		pMatrixB->m_pElements,
		pMatrixB->m_size[0],		// Stride of B
		&beta,						// beta = 0.0
		pResult->m_pElements,		// C is the result
		pResult->m_size[0]			// Stride of result 
	);
#else
	doublecomplex alpha;
	alpha.real = 1.0;
	alpha.imag = 0.0;
	doublecomplex beta;
	beta.real = 1.0;
	beta.imag = 0.0;
	doublecomplex *ptrA = (doublecomplex *)(pMatrixA->m_pElements);
	doublecomplex *ptrB = (doublecomplex *)(pMatrixB->m_pElements); 
	doublecomplex *ptrC = (doublecomplex *)(pResult->m_pElements);
	zgemm('n','n',pMatrixA->m_size[0],pMatrixB->m_size[1],pMatrixA->m_size[1],&alpha,ptrA,pMatrixA->m_size[0],ptrB,pMatrixB->m_size[0],&beta,ptrC,pResult->m_size[0]);
#endif
	// Increment the matrix multiplication count
	PROF_INCR_COUNTER(Profiler::MATRIX_MULT_COUNT);
	
	// Return a pointer to the result matrix
	return pResult;
}

/***************************************************************
* Function: static MatrixObj<float64>::scalarMult()
* Purpose : Scalar multiplication of 64-bit float matrices
* Initial : Maxime Chevalier-Boisvert on March 3, 2009
****************************************************************
Revisions and bug fixes:
*/
template <> MatrixObj<float64>* MatrixObj<float64>::scalarMult(const MatrixObj* pMatrix, float64 scalar)
{
	// Create a new matrix object to store the result
	MatrixObj* pResult = new MatrixObj(pMatrix->m_size);
	
	// If the input matrix is isEmpty, return early
	if (pMatrix->isEmpty())
		return pResult;
#ifndef MCVM_USE_ACML	
	// Call the BLAS function to perform the multiplication
	// This computes: y = alpha * x + y
	cblas_daxpy(
		pMatrix->m_numElements,
		scalar,						// alpha is the scalar
		pMatrix->m_pElements,
		1,
		pResult->m_pElements,		// C is the result
		1
	);
#else
	daxpy(pMatrix->m_numElements,scalar,pMatrix->m_pElements,1,pResult->m_pElements,1);
#endif
	
	// Return a pointer to the result matrix
	return pResult;
}

/***************************************************************
* Function: static MatrixObj<Complex128>::scalarMult()
* Purpose : Scalar multiplication of 128-bit complex matrices
* Initial : Maxime Chevalier-Boisvert on March 11, 2009
****************************************************************
Revisions and bug fixes:
*/
template <> MatrixObj<Complex128>* MatrixObj<Complex128>::scalarMult(const MatrixObj* pMatrix, Complex128 scalar)
{
	// Create a new matrix object to store the result
	MatrixObj* pResult = new MatrixObj(pMatrix->m_size);
	
	// If the input matrix is isEmpty, return early
	if (pMatrix->isEmpty())
		return pResult;
#ifndef MCVM_USE_ACML	
	// Call the BLAS function to perform the multiplication
	// This computes: y = alpha * x + y
	cblas_zaxpy(
		pMatrix->m_numElements,
		&scalar,					// alpha is the scalar
		pMatrix->m_pElements,
		1,
		pResult->m_pElements,		// C is the result
		1
	);
#else
	zaxpy(pMatrix->m_numElements,(doublecomplex *)&scalar,(doublecomplex*)pMatrix->m_pElements,1,(doublecomplex*)pResult->m_pElements,1);
#endif
	// Return a pointer to the result matrix
	return pResult;
}

/***************************************************************
* Function: static MatrixObj<float64>::matrixLeftDiv()
* Purpose : Matrix left division of 64-bit float matrices
* Initial : Maxime Chevalier-Boisvert on March 3, 2009
****************************************************************
Revisions and bug fixes:
*/
template <> MatrixObj<float64>* MatrixObj<float64>::matrixLeftDiv(const MatrixObj* pMatrixA, const MatrixObj* pMatrixB)
{
	// Ensure that the matrices have compatible dimensions
	assert (leftDivCompatible(pMatrixA, pMatrixB));
	
	// If the matrix A is square
	if (pMatrixA->isSquare())
	{
		std::cout << "found a square matrix -- will use dgesv to solve the system" << std::endl;
		
		// Make a copy of B to store the output
		MatrixF64Obj* pOutMatrix = pMatrixB->copy();
#ifdef MCVM_USE_CLAPACK	
		integer n 		= pMatrixA->m_size[0];		// N - number of rows/cols of A
		integer nrhs 	= pMatrixB->m_size[1];		// number of cols of B	
		doublereal* a	= pMatrixA->m_pElements;	// Matrix A
		integer lda		= pMatrixA->m_size[0];		// Stride of A
		integer* ipiv	= new integer[n];			// (output) pivot indices
		doublereal* b	= pOutMatrix->m_pElements;	// Matrix B
		integer ldb		= pMatrixB->m_size[0];		// Stride of B
		integer info;								// (output) convergence info
		
		// Call the DGESV function to solve the system A * X = B
		dgesv_(
			&n,
			&nrhs,
			a,
			&lda,
			ipiv,
			b,
			&ldb,
			&info
		);
		
		// Delete the pivot indices array
		delete [] ipiv;
#else
		int n = pMatrixA->m_size[0];
		int *ipiv = new int[n];
		int info;
#ifdef MCVM_USE_ACML
		dgesv(pMatrixA->m_size[0],pMatrixB->m_size[1],pMatrixA->m_pElements,pMatrixA->m_size[0],ipiv,pOutMatrix->m_pElements,pMatrixB->m_size[0],&info);
#endif
#ifdef MCVM_USE_LAPACKE
		info = LAPACKE_dgesv(LAPACK_COL_MAJOR, pMatrixA->m_size[0],pMatrixB->m_size[1],pMatrixA->m_pElements,pMatrixA->m_size[0],ipiv,pOutMatrix->m_pElements,pMatrixB->m_size[0]);
#endif
		delete[] ipiv;
#endif
/*
		// If the result could not be computed, throw an exception
		if (info != 0)
			throw RunError("illegal value in input matrix");
		
		// Return the output matrix
		return pOutMatrix;
	}	
*/		
		
		// If the result was correctly computed, return it
		if (info == 0)
			return pOutMatrix;

	}
	// If the matrix A is not square
	else {
		std::cout << "matrix is not square " << std::endl;
		/* this is the presumed matlab way of dealing with non square matrizes:
                function M = div(A,B)
                   [m,n] = size(A);
                   k = min(m,n);
                   [Q,R,P] = qr(A);
                   QB = (Q'*B);
                   T = (R(1:k,1:k)\(QB(1:k,:)));
                   M = P(:,1:k)*T;
                end
		 */

#ifdef MCVM_USE_CLAPACK
		//**** get m,n,k and the qr factorization via dgeqp3
		integer m = pMatrixA->m_size[0];    //rows of the matrix A
		integer n = pMatrixA->m_size[1];    //columns of the matrix A
		integer k = m>n?n:m;
		MatrixObj *pMatrixQR = pMatrixA->copy();
		doublereal *a = pMatrixQR->m_pElements; // the elements of matrix A
		integer lda = pMatrixA->m_size[0];  // lda/Stride/leading dimension of A	
		integer *jpvt = new integer[n];     // the permutation matrix, input is 0, meaning all columns can freely by pivoted ('free columns')
		doublereal *tau = new doublereal[k];  //tau - scalar factors of the elementary reflectors (output)
		doublereal *work;
		integer lwork = -1; //queries the work size
		doublereal worksize;
		integer info;

		//query the work size
		dgeqp3_(
				&m,
				&n,
				a,
				&lda,
				jpvt,
				tau,
				&worksize, //the worksize will be stored in the first entry of the work array
				&lwork,
				&info
		);

		//allocate work size array		
		lwork = static_cast<integer>(worksize);
		work = new doublereal[lwork];

		//actually compute the qr factorization
		dgeqp3_(
				&m,
				&n,
				a,
				&lda,
				jpvt,
				tau,
				work,
				&lwork,
				&info
		);
#else
		int m = pMatrixA->m_size[0];    //rows of the matrix A
		int n = pMatrixA->m_size[1];    //columns of the matrix A
		int k = m>n?n:m;
		MatrixObj *pMatrixQR = pMatrixA->copy();
		double *a = pMatrixQR->m_pElements; // the elements of matrix A
		int lda = pMatrixA->m_size[0];  // lda/Stride/leading dimension of A	
		int *jpvt = new int[n];     // the permutation matrix, input is 0, meaning all columns can freely by pivoted ('free columns')
		double *tau = new double[k];  //tau - scalar factors of the elementary reflectors (output)
		int info;
#ifdef MCVM_USE_ACML
		//ACML doesn't require double calls
		dgeqp3(m, n, a, lda, jpvt, tau, &info);
#endif
#ifdef MCVM_USE_LAPACKE
		info = LAPACKE_dgeqp3(LAPACK_COL_MAJOR,m, n, a, lda, jpvt, tau);
#endif

#endif
		
		//****compute QB = (Q'*B) via dormqr
		MatrixObj *pMatrixQB = pMatrixB->copy(); //need a place to store qb, and input of B
#ifdef MCVM_USE_CLAPACK
		char side  = 'L';
		char trans = 'T';
		integer mb = pMatrixB->m_size[0]; //rows of B
		integer nb = pMatrixB->m_size[1]; //columns of B
		//k number of elementary reflectors min(m,n)
		//a stays a - the qr array
		lda = mb;
		//tau stays tau
		doublereal *c = pMatrixQB->m_pElements; //input B, output QB
		integer ldc = mb;
		integer lworkQuery = -1; //queries the work size
		//work stays work
		//info stays info

		std::cout << "compute worksize for q'*b" << std::endl;

		//find work size for Q'*B
		dormqr_(
				&side,
				&trans,
				&mb,
				&nb,
				&k,
				a,
				&lda,
				tau,
				c,
				&ldc,
				work,
				&lworkQuery,
				&info
		);

		//get work - will only delete/create new 'work' array if the new required worksize is larger than the allocated array
		integer newLwork = static_cast<integer>(work[0]);
		if (newLwork > lwork){
			std::cout << "will create new work array: old "<<lwork<<" vs new: "<<newLwork<<std::endl;
			delete [] work;
			lwork = newLwork;
			work = new doublereal[lwork];
		} else {
			std::cout << "will reuse work array: old "<<lwork<<" vs new: "<<newLwork<<std::endl;
		}
			

		//compute Q'*B
		dormqr_(
				&side,
				&trans,
				&mb,
				&nb,
				&k,
				a,
				&lda,
				tau,
				c,
				&ldc,
				work,
				&lwork,
				&info
		);

                //don't need the work array anymore
                delete [] work;

		//**** find T = (R(1:k,1:k)\(QB(1:k,:))) via dtrtrs, i.e. solve R(1:k,1:k)*T = QB(1:k,:) via dtrtrs -- the result T is stored in QB
		char uplo = 'U';  //R is upper triangular
		trans= 'N';  //don't take the transpose of R
		char diag = 'N';  //R is non unit triangular, that means the diagonal entries are not necessarily 1
		integer n2 = k; //order of matrix A
		integer nrhs = nb; //number of columns of QB = number of columns of B
		//a - the upper triangular matrix pMatrixQR->m_pElements;
		lda = mb; //the stride/leading dimension of R -- number of rows of QR = num of rows of B
		doublereal *b = c; //elements of QB(1:k,:)
		integer ldb = mb; //the stride/leading dimension of QB -- number of rows of QB
		//info stays info

		//actually compute T = QB = (R(1:k,1:k)\(QB(1:k,:)))
		dtrtrs_(
				&uplo,
				&trans,
				&diag,
				&n2,
				&nrhs,
				a,
				&lda,
				b,
				&ldb,
				&info
				);


		//****find M = P(:,1:k)*T = P(:,1:k)*QB by copying while permuting the rows of QB to a result matrix
                //create result matrix
                MatrixObj* pResult = new MatrixObj(n, nb); //cols A x (cols B = cols QB)

                for (int i = 0; i < n; i++){
                  //copy row i of QB to row jpvt[i] of Result
                  if (jpvt[i] <= nb){ //matrizes have different sizes - only copy if result is in an actual row
                    for (int j = 0; j < nb; j++){ //because matrizes are stored column major, we can't use memcpy
                      pResult->m_pElements[j*n+jpvt[i]-1] = pMatrixQB->m_pElements[j*nb+i];
                    }
                  }
                }

                //delete arrays
                delete [] jpvt;
                delete [] pMatrixQB;

		return pResult;
#else
		char side  = 'L';
		char trans = 'T';
		int mb = pMatrixB->m_size[0]; //rows of B
		int nb = pMatrixB->m_size[1]; //columns of B
		//k number of elementary reflectors min(m,n)
		//a stays a - the qr array
		lda = mb;
		//tau stays tau
		double *c = pMatrixQB->m_pElements; //input B, output QB
		int ldc = mb;
		int lworkQuery = -1; //queries the work size
		//work stays work
		//info stays info

		std::cout << "compute worksize for q'*b" << std::endl;
#ifdef MCVM_USE_ACML
		//ACML doesn't need stupid two calls. Does auto allocation.
		dormqr(side, trans, mb, nb, k, a, lda, tau, c, ldc, &info);
#endif
#ifdef MCVM_USE_LAPACKE
		info = LAPACKE_dormqr(LAPACK_COL_MAJOR,side,trans,mb,nb,k,a,lda,tau,c,ldc);
#endif

		//**** find T = (R(1:k,1:k)\(QB(1:k,:))) via dtrtrs, i.e. solve R(1:k,1:k)*T = QB(1:k,:) via dtrtrs -- the result T is stored in QB
		char uplo = 'U';  //R is upper triangular
		trans= 'N';  //don't take the transpose of R
		char diag = 'N';  //R is non unit triangular, that means the diagonal entries are not necessarily 1
		int n2 = k; //order of matrix A
		int nrhs = nb; //number of columns of QB = number of columns of B
		//a - the upper triangular matrix pMatrixQR->m_pElements;
		lda = mb; //the stride/leading dimension of R -- number of rows of QR = num of rows of B
		double *b = c; //elements of QB(1:k,:)
		int ldb = mb; //the stride/leading dimension of QB -- number of rows of QB
		//info stays info
#ifdef MCVM_USE_ACML
		//actually compute T = QB = (R(1:k,1:k)\(QB(1:k,:)))
		dtrtrs(uplo, trans, diag, n2, nrhs, a, lda, b, ldb, &info);
#endif
#ifdef MCVM_USE_LAPACKE
		info = LAPACKE_dtrtrs(LAPACK_COL_MAJOR,uplo,trans,diag,n2,nrhs,a,lda,b,ldb);
#endif

		//****find M = P(:,1:k)*T = P(:,1:k)*QB by copying while permuting the rows of QB to a result matrix
                //create result matrix
                MatrixObj* pResult = new MatrixObj(n, nb); //cols A x (cols B = cols QB)

                for (int i = 0; i < n; i++){
                  //copy row i of QB to row jpvt[i] of Result
                  if (jpvt[i] <= nb){ //matrizes have different sizes - only copy if result is in an actual row
                    for (int j = 0; j < nb; j++){ //because matrizes are stored column major, we can't use memcpy
                      pResult->m_pElements[j*n+jpvt[i]-1] = pMatrixQB->m_pElements[j*nb+i];
                    }
                  }
                }

                //delete arrays
                delete [] jpvt;
                delete [] pMatrixQB;

		return pResult;

#endif
	}	
}

/***************************************************************
* Function: static MatrixObj<Complex128>::matrixLeftDiv()
* Purpose : Matrix left division of 128-bit complex matrices
* Initial : Maxime Chevalier-Boisvert on March 11, 2009
****************************************************************
Revisions and bug fixes:
*/
template <> MatrixObj<Complex128>* MatrixObj<Complex128>::matrixLeftDiv(const MatrixObj* pMatrixA, const MatrixObj* pMatrixB)
{
	// Ensure that the matrices have compatible dimensions
	assert (leftDivCompatible(pMatrixA, pMatrixB));
	
	// If the matrix A is square
	if (pMatrixA->isSquare())
	{
#ifdef MCVM_USE_CLAPACK
		// Make a copy of B to store the output
		MatrixC128Obj* pOutMatrix = pMatrixB->copy();
		
		integer n 			= pMatrixA->m_size[0];						// N - number of rows/cols of A
		integer nrhs 		= pMatrixB->m_size[1];						// number of cols of B	
		doublecomplex* a	= (doublecomplex*)pMatrixA->m_pElements;	// Matrix A
		integer lda			= pMatrixA->m_size[0];						// Stride of A
		integer* ipiv		= new integer[n];							// (output) pivot indices
		doublecomplex* b	= (doublecomplex*)pOutMatrix->m_pElements;	// Matrix B
		integer ldb			= pMatrixB->m_size[0];						// Stride of B
		integer info;													// (output) convergence info
		
		// Call the ZGESV function to solve the system A * X = B
		zgesv_(
			&n,
			&nrhs,
			a,
			&lda,
			ipiv,
			b,
			&ldb,
			&info
		);
		
		// Delete the pivot indices array
		delete [] ipiv;
#else
		MatrixC128Obj* pOutMatrix = pMatrixB->copy();
		
		int n 			= pMatrixA->m_size[0];						// N - number of rows/cols of A
		int nrhs 		= pMatrixB->m_size[1];						// number of cols of B	
		int lda			= pMatrixA->m_size[0];						// Stride of A
		int* ipiv		= new int[n];							// (output) pivot indices
		int ldb			= pMatrixB->m_size[0];						// Stride of B
		int info;													// (output) convergence info
#ifdef MCVM_USE_ACML	
		doublecomplex* a	= (doublecomplex*)pMatrixA->m_pElements;	// Matrix A
		doublecomplex* b	= (doublecomplex*)pOutMatrix->m_pElements;	// Matrix B
		zgesv_(
			&n,
			&nrhs,
			a,
			&lda,
			ipiv,
			b,
			&ldb,
			&info
		);
#endif
#ifdef MCVM_USE_LAPACKE
		lapack_complex_double *a = (lapack_complex_double*)pMatrixA->m_pElements;
		lapack_complex_double *b = (lapack_complex_double*)pOutMatrix->m_pElements;
		info = LAPACKE_zgesv(LAPACK_COL_MAJOR,n,nrhs,a,lda,ipiv,b,ldb);
#endif
		
		// Delete the pivot indices array
		delete [] ipiv;

#endif
		// If the result could not be computed, throw an exception
		if (info != 0)
			throw RunError("illegal value in input matrix");
		
		// Return the output matrix
		return pOutMatrix;
	}
	
	// Otherwise, A is an M-by-N matrix
	else
	{		
		// Throw an exception
		throw RunError("M-by-N matrix support for left division currently unimplemented");
	}
}

/***************************************************************
* Function: static MatrixObj<float64>::matrixRightDiv()
* Purpose : Matrix right division of 64-bit float matrices
* Initial : Maxime Chevalier-Boisvert on June 17, 2009
****************************************************************
Revisions and bug fixes:
*/
template <> MatrixObj<float64>* MatrixObj<float64>::matrixRightDiv(const MatrixObj* pMatrixA, const MatrixObj* pMatrixB)
{
	// If the right matrix is a scalar
	if (pMatrixB->isScalar())
	{
		// Perform the operation with a matrix lhs and a scalar rhs
		return rhsScalarArrayOp<DivOp<float64>, float64, float64>(pMatrixA, pMatrixB->getScalar());
	}
	
	// TODO: find more direct solution
	
	// Return the equivalence (A' \ B')'
	return transpose(matrixLeftDiv(transpose(pMatrixA), transpose(pMatrixB)));
}

/***************************************************************
* Function: static MatrixObj<Complex128>::matrixRightDiv()
* Purpose : Matrix right division of complex matrices
* Initial : Maxime Chevalier-Boisvert on June 17, 2009
****************************************************************
Revisions and bug fixes:
*/
template <> MatrixObj<Complex128>* MatrixObj<Complex128>::matrixRightDiv(const MatrixObj* pMatrixA, const MatrixObj* pMatrixB)
{
	// If the right matrix is a scalar
	if (pMatrixB->isScalar())
	{
		// Perform the operation with a matrix lhs and a scalar rhs
		return rhsScalarArrayOp<DivOp<Complex128>, Complex128, Complex128>(pMatrixA, pMatrixB->getScalar());
	}
	
	// TODO: find more direct solution
	
	// Return the equivalence (A' \ B')'
	return transpose(matrixLeftDiv(transpose(pMatrixA), transpose(pMatrixB)));
}

/***************************************************************
* Function: static MatrixObj<DataObject*>::initRange()
* Purpose : Range initialization for cell array objects
* Initial : Maxime Chevalier-Boisvert on March 15, 2009
****************************************************************
Revisions and bug fixes:
*/
template <> void MatrixObj<DataObject*>::initRange(DataObject** pStart, DataObject** pEnd)
{
	// Fill the range with empty cell array objects
	std::uninitialized_fill(pStart, pEnd, new MatrixObj<DataObject*>());
}
