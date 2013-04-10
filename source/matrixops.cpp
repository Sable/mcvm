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
#include "matrixops.h"

/***************************************************************
* Function: matrixMultOp()
* Purpose : Implement the matrix multiplication operation
* Initial : Maxime Chevalier-Boisvert June 5, 2009
****************************************************************
Revisions and bug fixes:
*/
DataObject* matrixMultOp(const DataObject* pLeftObj, const DataObject* pRightObj)
{
	// If either of the values are 128-bit complex matrices
	if (pLeftObj->getType() == DataObject::MATRIX_C128 || pRightObj->getType() == DataObject::MATRIX_C128)
	{
		// Convert the objects to 128-bit complex matrices, if necessary
		if (pLeftObj->getType() != DataObject::MATRIX_C128)	 pLeftObj = pLeftObj->convert(DataObject::MATRIX_C128);
		if (pRightObj->getType() != DataObject::MATRIX_C128) pRightObj = pRightObj->convert(DataObject::MATRIX_C128);
	
		// Get typed pointers to the values
		MatrixC128Obj* pLMatrix = (MatrixC128Obj*)pLeftObj;
		MatrixC128Obj* pRMatrix = (MatrixC128Obj*)pRightObj;
	
		// If the left matrix is a scalar
		if (pLMatrix->isScalar())
		{
			// Perform scalar multiplication
			return MatrixC128Obj::scalarMult(pRMatrix, pLMatrix->getScalar());
		}
	
		// If the right matrix is a scalar
		if (pRMatrix->isScalar())
		{
			// Perform scalar multiplication
			return MatrixC128Obj::scalarMult(pLMatrix, pRMatrix->getScalar());
		}
	
		// If the matrix dimensions are not compatible
		if (!MatrixC128Obj::multCompatible(pLMatrix, pRMatrix))
		{
			// Throw an exception
			throw RunError("incompatible matrix dimensions in matrix multiplication");
		}
	
		// Perform the multiplication
		return MatrixC128Obj::matrixMult(pLMatrix, pRMatrix);
	}
	
	// Convert the objects to 64-bit float matrices, if necessary
	if (pLeftObj->getType() != DataObject::MATRIX_F64) 	pLeftObj = pLeftObj->convert(DataObject::MATRIX_F64);
	if (pRightObj->getType() != DataObject::MATRIX_F64) pRightObj = pRightObj->convert(DataObject::MATRIX_F64);
	
	// Get typed pointers to the values
	MatrixF64Obj* pLMatrix = (MatrixF64Obj*)pLeftObj;
	MatrixF64Obj* pRMatrix = (MatrixF64Obj*)pRightObj;
	
	// If the left matrix is a scalar
	if (pLMatrix->isScalar())
	{
		// Perform scalar multiplication
		return MatrixF64Obj::scalarMult(pRMatrix, pLMatrix->getScalar());
	}
	
	// If the right matrix is a scalar
	if (pRMatrix->isScalar())
	{
		// Perform scalar multiplication
		return MatrixF64Obj::scalarMult(pLMatrix, pRMatrix->getScalar());
	}
	
	// If the matrix dimensions are not compatible
	if (!MatrixF64Obj::multCompatible(pLMatrix, pRMatrix))
	{
		// Throw an exception
		throw RunError("incompatible matrix dimensions in matrix multiplication");
	}
	
	// Perform the multiplication
	return MatrixF64Obj::matrixMult(pLMatrix, pRMatrix);
}

/***************************************************************
* Function: scalarMultOp()
* Purpose : Implement the scalar multiplication operation
* Initial : Maxime Chevalier-Boisvert on June 5, 2009
****************************************************************
Revisions and bug fixes:
*/
DataObject* scalarMultOp(const DataObject* pLeftObj, float64 scalar)
{
	// If the matrix is a 128-bit complex matrix
	if (pLeftObj->getType() == DataObject::MATRIX_C128)
	{
		// Get a typed pointer to the matrix
		MatrixC128Obj* pMatrix = (MatrixC128Obj*)pLeftObj;
			
		// Perform the operation
		return MatrixC128Obj::scalarMult(pMatrix, scalar);
	}
	
	// Convert the object to 64-bit float matrix
	pLeftObj = pLeftObj->convert(DataObject::MATRIX_F64);
	
	// Get a typed pointer to the matrix
	MatrixF64Obj* pMatrix = (MatrixF64Obj*)pLeftObj;
		
	// Perform the operation
	return MatrixF64Obj::scalarMult(pMatrix, scalar);
}

/***************************************************************
* Function: matrixRightDivOp()
* Purpose : Implement the matrix right division operation
* Initial : Maxime Chevalier-Boisvert June 9, 2009
****************************************************************
Revisions and bug fixes:
*/
DataObject* matrixRightDivOp(const DataObject* pLeftObj, const DataObject* pRightObj)
{
	// If either of the values are 128-bit complex matrices
	if (pLeftObj->getType() == DataObject::MATRIX_C128 || pRightObj->getType() == DataObject::MATRIX_C128)
	{
		// Convert the objects to 128-bit complex matrices, if necessary
		if (pLeftObj->getType() != DataObject::MATRIX_C128)	 pLeftObj = pLeftObj->convert(DataObject::MATRIX_C128);
		if (pRightObj->getType() != DataObject::MATRIX_C128) pRightObj = pRightObj->convert(DataObject::MATRIX_C128);

		// Get typed pointers to the values
		MatrixC128Obj* pLMatrix = (MatrixC128Obj*)pLeftObj;
		MatrixC128Obj* pRMatrix = (MatrixC128Obj*)pRightObj;

		// If both matrices are scalars
		if (pLMatrix->isScalar() && pRMatrix->isScalar())
		{
			// Get the scalar values
			Complex128 lScalar = pLMatrix->getScalar();
			Complex128 rScalar = pRMatrix->getScalar();

			// If the divisor is 0
			if (rScalar == Complex128(0))
			{
				// Return +/- infinity
				return new MatrixC128Obj((lScalar / std::abs(lScalar)) * DOUBLE_INFINITY);
			}

			// Perform the division
			return new MatrixC128Obj(lScalar / rScalar);
		}

		// If the right matrix is a scalar
		if (pRMatrix->isScalar())
		{
			// Perform the division
			return MatrixC128Obj::rhsScalarArrayOp<DivOp<Complex128>, Complex128, Complex128>(pLMatrix, pRMatrix->getScalar());
		}
		
		// Perform matrix right division
		return MatrixC128Obj::matrixRightDiv(pLMatrix, pRMatrix);
	}

	// Convert the objects to 64-bit float matrices, if necessary
	if (pLeftObj->getType() != DataObject::MATRIX_F64) 	pLeftObj = pLeftObj->convert(DataObject::MATRIX_F64);
	if (pRightObj->getType() != DataObject::MATRIX_F64) pRightObj = pRightObj->convert(DataObject::MATRIX_F64);

	// Get typed pointers to the values
	MatrixF64Obj* pLMatrix = (MatrixF64Obj*)pLeftObj;
	MatrixF64Obj* pRMatrix = (MatrixF64Obj*)pRightObj;

	// If both matrices are scalars
	if (pLMatrix->isScalar() && pRMatrix->isScalar())
	{
		// Get the scalar values
		float64 lFloat = pLMatrix->getScalar();
		float64 rFloat = pRMatrix->getScalar();

		// If the divisor is 0
		if (rFloat == 0)
		{
			// Return +/- infinity
			return new MatrixF64Obj(sign(lFloat) * DOUBLE_INFINITY);
		}

		// Perform the division
		return new MatrixF64Obj(lFloat / rFloat);
	}

	// If the right matrix is a scalar
	if (pRMatrix->isScalar())
	{
		// Perform the division
		return MatrixF64Obj::rhsScalarArrayOp<DivOp<float64>, float64, float64>(pLMatrix, pRMatrix->getScalar());
	}

	// Perform matrix right division
	return MatrixF64Obj::matrixRightDiv(pLMatrix, pRMatrix);
}
