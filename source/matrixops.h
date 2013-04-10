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
#ifndef MATRIXOPS_H_
#define MATRIXOPS_H_

// Header files
#include <cmath>
#include "runtimebase.h"
#include "chararrayobj.h"
#include "utility.h"

/***************************************************************
* Class   : AddOp
* Purpose : Function object for the addition operator
* Initial : Maxime Chevalier-Boisvert on February 26, 2009
****************************************************************
Revisions and bug fixes:
*/
template <class ScalarType> class AddOp
{
public:
	
	// Function operator
	static ScalarType op(ScalarType inA, ScalarType inB)
	{
		// Perform the addition
		return inA + inB;
	}
};

/***************************************************************
* Class   : SubOp
* Purpose : Function object for the subtraction operator
* Initial : Maxime Chevalier-Boisvert on February 26, 2009
****************************************************************
Revisions and bug fixes:
*/
template <class ScalarType> class SubOp
{
public:
	
	// Function operator
	static ScalarType op(ScalarType inA, ScalarType inB)
	{
		// Perform the subtraction
		return inA - inB;
	}
};

/***************************************************************
* Class   : MultOp
* Purpose : Function object for the multiplication operator
* Initial : Maxime Chevalier-Boisvert on February 26, 2009
****************************************************************
Revisions and bug fixes:
*/
template <class ScalarType> class MultOp
{
public:
	
	// Function operator
	static ScalarType op(ScalarType inA, ScalarType inB)
	{
		// Perform the multiplication
		return inA * inB;
	}
};

/***************************************************************
* Class   : DivOp
* Purpose : Function object for the division operator
* Initial : Maxime Chevalier-Boisvert on February 26, 2009
****************************************************************
Revisions and bug fixes:
*/
template <class ScalarType> class DivOp
{
public:
	
	// Function operator
	static inline ScalarType op(ScalarType inA, ScalarType inB)
	{
		// If B is 0
		if (inB == ScalarType(0))
		{
			// The result is infinity
			return sign(inA) * (ScalarType)DOUBLE_INFINITY;
		}
			
		// Perform the division
		return inA / inB;
	}
};

/***************************************************************
* Function: DivOp<Complex128>::op()
* Purpose : Specialized division operator for complex numbers
* Initial : Maxime Chevalier-Boisvert on March 11, 2009
****************************************************************
Revisions and bug fixes:
*/
template <> inline Complex128 DivOp<Complex128>::op(Complex128 inA, Complex128 inB)
{
	// If B is 0
	if (inB == Complex128(0))
	{		
		// The result is infinity
		return (inA / std::abs(inA)) * DOUBLE_INFINITY;
	}
		
	// Perform the division
	return inA / inB;
}

/***************************************************************
* Class   : PowOp
* Purpose : Function object for the exponentiation operator
* Initial : Maxime Chevalier-Boisvert on February 26, 2009
****************************************************************
Revisions and bug fixes:
*/
template <class ScalarType> class PowOp
{
public:
	
	// Function operator
	static ScalarType op(ScalarType inA, ScalarType inB)
	{
		// Perform the exponentiation
		return pow(inA, inB);
	}
};

/***************************************************************
* Class   : EqualOp
* Purpose : Function object for the equality operator
* Initial : Maxime Chevalier-Boisvert on February 26, 2009
****************************************************************
Revisions and bug fixes:
*/
template <class ScalarType> class EqualOp
{
public:
	
	// Function operator
	static bool op(ScalarType inA, ScalarType inB)
	{
		// Perform the comparison
		return (inA == inB)? 1:0;
	}
};

/***************************************************************
* Class   : NotEqualOp
* Purpose : Function object for the inequality operator
* Initial : Maxime Chevalier-Boisvert on March 2, 2009
****************************************************************
Revisions and bug fixes:
*/
template <class ScalarType> class NotEqualOp
{
public:
	
	// Function operator
	static bool op(ScalarType inA, ScalarType inB)
	{
		// Perform the comparison
		return (inA != inB)? 1:0;
	}
};

/***************************************************************
* Class   : GreaterThanOp
* Purpose : Function object for the greater-than (>) operator
* Initial : Maxime Chevalier-Boisvert on February 26, 2009
****************************************************************
Revisions and bug fixes:
*/
template <class ScalarType> class GreaterThanOp
{
public:
	
	// Function operator
	static bool op(ScalarType inA, ScalarType inB)
	{
		// Perform the comparison
		return (inA > inB)? 1:0;
	}
};

/***************************************************************
* Function: GreaterThanOp<Complex128>::op()
* Purpose : Specialized gt (<) comparison for complex numbers
* Initial : Maxime Chevalier-Boisvert on May 7, 2009
****************************************************************
Revisions and bug fixes:
*/
template <> inline bool GreaterThanOp<Complex128>::op(Complex128 inA, Complex128 inB)
{
	// Perform the comparison
	if (inA.real() > inB.real())
		return true;
	else if (inA.real() < inB.real())
		return false;
	else
		return (inA.imag() > inB.imag());
}

/***************************************************************
* Class   : GreaterThanOp
* Purpose : Function obj for the greater-than-eq (>=) operator
* Initial : Maxime Chevalier-Boisvert on March 2, 2009
****************************************************************
Revisions and bug fixes:
*/
template <class ScalarType> class GreaterThanEqOp
{
public:
	
	// Function operator
	static bool op(ScalarType inA, ScalarType inB)
	{
		// Perform the comparison
		return (inA >= inB)? 1:0;
	}
};

/***************************************************************
* Function: GreaterThanEqOp<Complex128>::op()
* Purpose : Specialized gte (>=) comparison for complex numbers
* Initial : Maxime Chevalier-Boisvert on May 7, 2009
****************************************************************
Revisions and bug fixes:
*/
template <> inline bool GreaterThanEqOp<Complex128>::op(Complex128 inA, Complex128 inB)
{
	// Perform the comparison
	if (inA.real() > inB.real())
		return true;
	else if (inA.real() < inB.real())
		return false;
	else
		return (inA.imag() >= inB.imag());
}

/***************************************************************
* Class   : LessThanOp
* Purpose : Function object for the less-than (<) operator
* Initial : Maxime Chevalier-Boisvert on March 2, 2009
****************************************************************
Revisions and bug fixes:
*/
template <class ScalarType> class LessThanOp
{
public:
	
	// Function operator
	static inline bool op(ScalarType inA, ScalarType inB)
	{
		// Perform the comparison
		return inA < inB;
	}
};

/***************************************************************
* Function: LessThanOp<Complex128>::op()
* Purpose : Specialized lt (<) comparison for complex numbers
* Initial : Maxime Chevalier-Boisvert on April 5, 2009
****************************************************************
Revisions and bug fixes:
*/
template <> inline bool LessThanOp<Complex128>::op(Complex128 inA, Complex128 inB)
{
	// Perform the comparison
	if (inA.real() < inB.real())
		return true;
	else if (inA.real() > inB.real())
		return false;
	else
		return (inA.imag() < inB.imag());
}

/***************************************************************
* Class   : LessThanEqOp
* Purpose : Function object for the less-than-eq (<=) operator
* Initial : Maxime Chevalier-Boisvert on March 2, 2009
****************************************************************
Revisions and bug fixes:
*/
template <class ScalarType> class LessThanEqOp
{
public:
	
	// Function operator
	static inline bool op(ScalarType inA, ScalarType inB)
	{
		// Perform the comparison
		return inA <= inB;
	}
};

/***************************************************************
* Function: LessThanEqOp<Complex128>::op()
* Purpose : Specialized lte (<=) comparison for complex numbers
* Initial : Maxime Chevalier-Boisvert on April 5, 2009
****************************************************************
Revisions and bug fixes:
*/
template <> inline bool LessThanEqOp<Complex128>::op(Complex128 inA, Complex128 inB)
{
	// Perform the comparison
	if (inA.real() < inB.real())
		return true;
	else if (inA.real() > inB.real())
		return false;
	else
		return (inA.imag() <= inB.imag());
}

/***************************************************************
* Class   : OrOp
* Purpose : Function object for the logical OR operator
* Initial : Maxime Chevalier-Boisvert on March 4, 2009
****************************************************************
Revisions and bug fixes:
*/
template <class ScalarType> class OrOp
{
public:
	
	// Function operator
	static bool op(ScalarType inA, ScalarType inB)
	{
		// Perform the logical OR
		return (inA || inB);
	}
};

/***************************************************************
* Function: OrOp<Complex128>::op()
* Purpose : Specialized logical OR operator for complex numbers
* Initial : Maxime Chevalier-Boisvert on June 5, 2009
****************************************************************
Revisions and bug fixes:
*/
template <> inline bool OrOp<Complex128>::op(Complex128 inA, Complex128 inB)
{
	// Perform the logical OR of the absolute values
	return (std::abs(inA) || std::abs(inB));
}

/***************************************************************
* Class   : AndOp
* Purpose : Function object for the logical AND operator
* Initial : Maxime Chevalier-Boisvert on March 4, 2009
****************************************************************
Revisions and bug fixes:
*/
template <class ScalarType> class AndOp
{
public:
	
	// Function operator
	static bool op(ScalarType inA, ScalarType inB)
	{
		// Perform the logical AND
		return (inA && inB);
	}
};

/***************************************************************
* Function: AndOp<Complex128>::op()
* Purpose : Specialized logical AND operator for complex numbers
* Initial : Maxime Chevalier-Boisvert on June 5, 2009
****************************************************************
Revisions and bug fixes:
*/
template <> inline bool AndOp<Complex128>::op(Complex128 inA, Complex128 inB)
{
	// Perform the logical AND of the absolute values
	return (std::abs(inA) && std::abs(inB));
}

/***************************************************************
* Class   : ModOp
* Purpose : Function object for the modulo operator
* Initial : Maxime Chevalier-Boisvert on February 27, 2009
****************************************************************
Revisions and bug fixes:
*/
template <class ScalarType> class ModOp
{
public:
	
	// Function operator
	static ScalarType op(ScalarType inA, ScalarType inB)
	{
		// Perform floating-point modulo
		return fmod(inA, inB);
	}
};

/***************************************************************
* Class   : BitAndOp
* Purpose : Function object for the bitwise AND operator
* Initial : Maxime Chevalier-Boisvert on February 26, 2009
****************************************************************
Revisions and bug fixes:
*/
template <class ScalarType> class BitAndOp
{
public:
	
	// Function operator
	static ScalarType op(ScalarType inA, ScalarType inB)
	{
		// Cast A and B into unsigned long integers
		unsigned long intPartA = (unsigned long)inA;
		unsigned long intPartB = (unsigned long)inB;
		
		// If either of the values are negative
		if (inA < 0 || inB < 0)
			throw RunError("negative values in bitwise AND operation");
		
		// If the difference is nonzero, throw an exception
		if (inA - intPartA != 0 || inB - intPartB != 0)
			throw RunError("non-integer values in bitwise AND operation");
		
		// Compute the result
		unsigned long result = intPartA & intPartB;
		
		// Convert the result back into the desired type
		return (ScalarType)result;
	}
};

/***************************************************************
* Class   : MaxOp
* Purpose : Function object for the maximum operator
* Initial : Maxime Chevalier-Boisvert on March 2, 2009
****************************************************************
Revisions and bug fixes:
*/
template <class ScalarType> class MaxOp
{
public:
	
	// Function operator
	static ScalarType op(ScalarType inA, ScalarType inB)
	{
		// Return the maximum of the two values
		return std::max(inA, inB);
	}
};

/***************************************************************
* Class   : MinOp
* Purpose : Function object for the minimum operator
* Initial : Maxime Chevalier-Boisvert on March 2, 2009
****************************************************************
Revisions and bug fixes:
*/
template <class ScalarType> class MinOp
{
public:
	
	// Function operator
	static ScalarType op(ScalarType inA, ScalarType inB)
	{
		// Return the maximum of the two values
		return std::min(inA, inB);
	}
};

/***************************************************************
* Class   : AbsOp
* Purpose : Function object for the absolute value operator
* Initial : Maxime Chevalier-Boisvert on March 11, 2009
****************************************************************
Revisions and bug fixes:
*/
template <class InType, class OutType = InType> class AbsOp
{
public:
	
	// Function operator
	static OutType op(InType in)
	{
		// Return the absolute value of the input
		return (OutType)std::abs(in);
	}
};

/***************************************************************
* Class   : ExpOp
* Purpose : Function object for the exponential function
* Initial : Maxime Chevalier-Boisvert on March 11, 2009
****************************************************************
Revisions and bug fixes:
*/
template <class ScalarType> class ExpOp
{
public:
	
	// Function operator
	static ScalarType op(ScalarType in)
	{
		// Return e raised to the value of in
		return std::exp(in);
	}
};

/***************************************************************
* Class   : SignOp
* Purpose : Function object for the sign operator
* Initial : Maxime Chevalier-Boisvert on March 1, 2009
****************************************************************
Revisions and bug fixes:
*/
template <class ScalarType> class SignOp
{
public:
	
	// Function operator
	static ScalarType op(ScalarType in)
	{
		// Return 1, 0, or -1 if the value is positive, null or negative, respectively
		if (in > 0)
			return 1;
		else if (in == 0)
			return 0;
		else
			return -1;
	}
};

/***************************************************************
* Class   : SqrtOp
* Purpose : Function object for the square root operator
* Initial : Maxime Chevalier-Boisvert on March 11, 2009
****************************************************************
Revisions and bug fixes:
*/
template <class ScalarType> class SqrtOp
{
public:
	
	// Function operator
	static ScalarType op(ScalarType in)
	{
		// Return the square root of the input value
		return std::sqrt(in);
	}
};

/***************************************************************
* Class   : NotOp
* Purpose : Function object for the negation operator
* Initial : Maxime Chevalier-Boisvert on March 2, 2009
****************************************************************
Revisions and bug fixes:
*/
template <class ScalarType> class NotOp
{
public:
	
	// Function operator
	static inline bool op(ScalarType in)
	{
		// Negate the value
		return (in == ScalarType(0));
	}
};

/***************************************************************
* Class   : SumOp
* Purpose : Function object for the sum operator
* Initial : Maxime Chevalier-Boisvert on March 4, 2009
****************************************************************
Revisions and bug fixes:
*/
template <class ScalarType> class SumOp
{
public:
	
	// Function operator
	static ScalarType op(ScalarType* pVecStart, ScalarType* pVecEnd, size_t vecStride)
	{
		// Initialize the sum to 0
		ScalarType sum = 0;
		
		// Sum all the vector elements
		for (ScalarType* pIn = pVecStart; pIn < pVecEnd; pIn += vecStride)
			sum += *pIn;		
		
		// Return the sum
		return sum;
	}
};

/***************************************************************
* Class   : AnyOp
* Purpose : Function object for the any operator
* Initial : Maxime Chevalier-Boisvert on March 4, 2009
****************************************************************
Revisions and bug fixes:
*/
template <class ScalarType> class AnyOp
{
public:
	
	// Function operator
	static bool op(ScalarType* pVecStart, ScalarType* pVecEnd, size_t vecStride)
	{		
		// If any element is nonzero, return true
		for (ScalarType* pIn = pVecStart; pIn < pVecEnd; pIn += vecStride)
			if (*pIn) return true;
		
		// No nonzero elements found, return false
		return false;
	}
};

/***************************************************************
* Function: lhsScalarArithOp<>()
* Purpose : Templated array arithmetic operation with scalar lhs
* Initial : Maxime Chevalier-Boisvert on June 2, 2009
****************************************************************
Revisions and bug fixes:
*/
template <template <class ScalarType> class ArithOp, class ScalarType> DataObject* lhsScalarArithOp(const DataObject* pMatrixR, ScalarType scalarL)
{
	// If the matrix is a 128-bit complex matrix
	if (pMatrixR->getType() == DataObject::MATRIX_C128)
	{
		// Get a typed pointer to the matrix
		MatrixC128Obj* pMatrix = (MatrixC128Obj*)pMatrixR;
			
		// Perform the operation
		return MatrixC128Obj::lhsScalarArrayOp<ArithOp<Complex128>, Complex128>(pMatrix, scalarL);
	}
	
	// Convert the object to 64-bit float matrix
	pMatrixR = pMatrixR->convert(DataObject::MATRIX_F64);
	
	// Get a typed pointer to the matrix
	MatrixF64Obj* pMatrix = (MatrixF64Obj*)pMatrixR;
		
	// Perform the operation
	return MatrixF64Obj::lhsScalarArrayOp<ArithOp<float64>, float64>(pMatrix, scalarL);
}

/***************************************************************
* Function: rhsScalarArithOp<>()
* Purpose : Templated array arithmetic operation with scalar rhs
* Initial : Maxime Chevalier-Boisvert on June 2, 2009
****************************************************************
Revisions and bug fixes:
*/
template <template <class ScalarType> class ArithOp, class ScalarType> DataObject* rhsScalarArithOp(const DataObject* pMatrixL, ScalarType scalarR)
{
	// If the matrix is a 128-bit complex matrix
	if (pMatrixL->getType() == DataObject::MATRIX_C128)
	{		
		// Get a typed pointer to the matrix
		MatrixC128Obj* pMatrix = (MatrixC128Obj*)pMatrixL;
			
		// Perform the operation
		return MatrixC128Obj::rhsScalarArrayOp<ArithOp<Complex128>, Complex128>(pMatrix, scalarR);
	}
	
	// Convert the object to 64-bit float matrix
	pMatrixL = pMatrixL->convert(DataObject::MATRIX_F64);
	
	// Get a typed pointer to the matrix
	MatrixF64Obj* pMatrix = (MatrixF64Obj*)pMatrixL;
		
	// Perform the operation
	return MatrixF64Obj::rhsScalarArrayOp<ArithOp<float64>, float64>(pMatrix, scalarR);
}

/***************************************************************
* Function: arrayArithOp<>()
* Purpose : Implement a templated array arithmetic operation
* Initial : Maxime Chevalier-Boisvert on March 26, 2009
****************************************************************
Revisions and bug fixes:
*/
template <template <class ScalarType> class ArithOp> DataObject* arrayArithOp(const DataObject* pLeftVal, const DataObject* pRightVal)
{
	// If either of the values are 128-bit complex matrices
	if (pLeftVal->getType() == DataObject::MATRIX_C128 || pRightVal->getType() == DataObject::MATRIX_C128)
	{
		// Convert the objects to 128-bit complex matrices, if necessary
		if (pLeftVal->getType() != DataObject::MATRIX_C128)	 pLeftVal = pLeftVal->convert(DataObject::MATRIX_C128);
		if (pRightVal->getType() != DataObject::MATRIX_C128) pRightVal = pRightVal->convert(DataObject::MATRIX_C128);				
		
		// Get typed pointers to the values
		MatrixC128Obj* pLMatrix = (MatrixC128Obj*)pLeftVal;
		MatrixC128Obj* pRMatrix = (MatrixC128Obj*)pRightVal;
		
		// Perform the operation
		return MatrixC128Obj::binArrayOp<ArithOp<Complex128>, Complex128>(pLMatrix, pRMatrix);
	}
	
	// Convert the objects to 64-bit float matrices, if necessary
	if (pLeftVal->getType() != DataObject::MATRIX_F64) 	pLeftVal = pLeftVal->convert(DataObject::MATRIX_F64);
	if (pRightVal->getType() != DataObject::MATRIX_F64) pRightVal = pRightVal->convert(DataObject::MATRIX_F64);
	
	// Get typed pointers to the values
	MatrixF64Obj* pLMatrix = (MatrixF64Obj*)pLeftVal;
	MatrixF64Obj* pRMatrix = (MatrixF64Obj*)pRightVal;
	
	// Perform the operation
	return MatrixF64Obj::binArrayOp<ArithOp<float64>, float64>(pLMatrix, pRMatrix);
}

/***************************************************************
* Function: lhsScalarLogicOp<>()
* Purpose : Templated comparison operation with scalar lhs
* Initial : Maxime Chevalier-Boisvert June 2, 2009
****************************************************************
Revisions and bug fixes:
*/
template <template <class ScalarType> class CompOp, class ScalarType> DataObject* lhsScalarLogicOp(const DataObject* pMatrixR, ScalarType scalarL)
{
	// If the object is a logical array
	if (pMatrixR->getType() == DataObject::LOGICALARRAY)
	{
		// Get a typed pointer to the matrix
		LogicalArrayObj* pMatrix = (LogicalArrayObj*)pMatrixR;
		
		// Perform the comparison on the matrix elements
		return LogicalArrayObj::lhsScalarArrayOp<CompOp<bool>, bool>(pMatrix, scalarL);
	}
	
	// If the object is a floating-point matrix
	else if (pMatrixR->getType() == DataObject::MATRIX_F64)
	{
		// Get a typed pointer to the matrix
		MatrixF64Obj* pMatrix = (MatrixF64Obj*)pMatrixR;
		
		// Perform the comparison on the matrix elements
		return MatrixF64Obj::lhsScalarArrayOp<CompOp<float64>, bool>(pMatrix, scalarL);
	}
	
	// If the object is a character array
	else if (pMatrixR->getType() == DataObject::CHARARRAY)
	{
		// Get a typed pointer to the matrix
		CharArrayObj* pMatrix = (CharArrayObj*)pMatrixR;
		
		// Perform the comparison on the matrix elements
		return CharArrayObj::lhsScalarArrayOp<CompOp<char>, bool>(pMatrix, scalarL);
	}
	
	// If the object is a complex matrix
	else if (pMatrixR->getType() == DataObject::MATRIX_C128)
	{		
		// Get a typed pointer to the matrix
		MatrixC128Obj* pMatrix = (MatrixC128Obj*)pMatrixR;
		
		// Perform the comparison on the matrix elements
		return MatrixC128Obj::lhsScalarArrayOp<CompOp<Complex128>, bool>(pMatrix, scalarL);
	}
	
	// Otherwise
	else
	{
		// Convert the object to a 64-bit float matrix
		pMatrixR = pMatrixR->convert(DataObject::MATRIX_F64);
		
		// Get a typed pointer to the matrix
		MatrixF64Obj* pMatrix = (MatrixF64Obj*)pMatrixR;
		
		// Perform the equality comparison on the matrix elements
		return MatrixF64Obj::lhsScalarArrayOp<CompOp<float64>, bool>(pMatrix, scalarL);
	}
}

/***************************************************************
* Function: rhsScalarLogicOp<>()
* Purpose : Templated comparison operation with scalar rhs
* Initial : Maxime Chevalier-Boisvert June 2, 2009
****************************************************************
Revisions and bug fixes:
*/
template <template <class ScalarType> class CompOp, class ScalarType> DataObject* rhsScalarLogicOp(const DataObject* pMatrixL, ScalarType scalarR)
{
	// If the object is a logical array
	if (pMatrixL->getType() == DataObject::LOGICALARRAY)
	{
		// Get a typed pointer to the matrix
		LogicalArrayObj* pMatrix = (LogicalArrayObj*)pMatrixL;
		
		// Perform the comparison on the matrix elements
		return LogicalArrayObj::rhsScalarArrayOp<CompOp<bool>, bool>(pMatrix, scalarR);
	}
	
	// If the object is a floating-point matrix
	else if (pMatrixL->getType() == DataObject::MATRIX_F64)
	{
		// Get a typed pointer to the matrix
		MatrixF64Obj* pMatrix = (MatrixF64Obj*)pMatrixL;
		
		// Perform the comparison on the matrix elements
		return MatrixF64Obj::rhsScalarArrayOp<CompOp<float64>, bool>(pMatrix, scalarR);
	}
	
	// If the object is a character array
	else if (pMatrixL->getType() == DataObject::CHARARRAY)
	{
		// Get a typed pointer to the matrix
		CharArrayObj* pMatrix = (CharArrayObj*)pMatrixL;
		
		// Perform the comparison on the matrix elements
		return CharArrayObj::rhsScalarArrayOp<CompOp<char>, bool>(pMatrix, scalarR);
	}
	
	// If the object is a complex matrix
	else if (pMatrixL->getType() == DataObject::MATRIX_C128)
	{		
		// Get a typed pointer to the matrix
		MatrixC128Obj* pMatrix = (MatrixC128Obj*)pMatrixL;
		
		// Perform the comparison on the matrix elements
		return MatrixC128Obj::rhsScalarArrayOp<CompOp<Complex128>, bool>(pMatrix, scalarR);
	}
	
	// Otherwise
	else
	{
		// Convert the object to a 64-bit float matrix
		pMatrixL = pMatrixL->convert(DataObject::MATRIX_F64);
		
		// Get a typed pointer to the matrix
		MatrixF64Obj* pMatrix = (MatrixF64Obj*)pMatrixL;
		
		// Perform the equality comparison on the matrix elements
		return MatrixF64Obj::rhsScalarArrayOp<CompOp<float64>, bool>(pMatrix, scalarR);
	}
}

/***************************************************************
* Function: matrixLogicOp<>()
* Purpose : Implement a templated matrix comparison operation
* Initial : Maxime Chevalier-Boisvert April 5, 2009
****************************************************************
Revisions and bug fixes:
*/
template <template <class ScalarType> class CompOp> DataObject* matrixLogicOp(const DataObject* pLeftObj, const DataObject* pRightObj)
{
	// If both values are logical arrays
	if (pLeftObj->getType() == DataObject::LOGICALARRAY && pRightObj->getType() == DataObject::LOGICALARRAY)
	{
		// Get typed pointers to the values
		LogicalArrayObj* pLMatrix = (LogicalArrayObj*)pLeftObj;
		LogicalArrayObj* pRMatrix = (LogicalArrayObj*)pRightObj;
		
		// Perform the comparison on the matrix elements
		return LogicalArrayObj::binArrayOp<CompOp<bool>, bool>(pLMatrix, pRMatrix);
	}
	
	// If both values are floating-point matrices
	else if (pLeftObj->getType() == DataObject::MATRIX_F64 && pRightObj->getType() == DataObject::MATRIX_F64)
	{
		// Get typed pointers to the values
		MatrixF64Obj* pLMatrix = (MatrixF64Obj*)pLeftObj;
		MatrixF64Obj* pRMatrix = (MatrixF64Obj*)pRightObj;
		
		// Perform the comparison on the matrix elements
		return MatrixF64Obj::binArrayOp<CompOp<float64>, bool>(pLMatrix, pRMatrix);
	}
	
	// If both values are character matrices
	else if (pLeftObj->getType() == DataObject::CHARARRAY && pRightObj->getType() == DataObject::CHARARRAY)
	{
		// Get typed pointers to the values
		CharArrayObj* pLMatrix = (CharArrayObj*)pLeftObj;
		CharArrayObj* pRMatrix = (CharArrayObj*)pRightObj;
		
		// Perform the comparison on the matrix elements
		return CharArrayObj::binArrayOp<CompOp<char>, bool>(pLMatrix, pRMatrix);
	}
	
	// If either of the values is a complex matrix
	else if (pLeftObj->getType() == DataObject::MATRIX_C128 || pRightObj->getType() == DataObject::MATRIX_C128)
	{
		// Convert both values to coplex matrices, if necessary
		if (pLeftObj->getType() != DataObject::MATRIX_C128)	 pLeftObj = pLeftObj->convert(DataObject::MATRIX_C128);
		if (pRightObj->getType() != DataObject::MATRIX_C128) pRightObj = pRightObj->convert(DataObject::MATRIX_C128);
		
		// Get typed pointers to the values
		MatrixC128Obj* pLMatrix = (MatrixC128Obj*)pLeftObj;
		MatrixC128Obj* pRMatrix = (MatrixC128Obj*)pRightObj;
		
		// Perform the comparison on the matrix elements
		return MatrixC128Obj::binArrayOp<CompOp<Complex128>, bool>(pLMatrix, pRMatrix);
	}
	
	// Otherwise
	else
	{
		// Convert the objects to 64-bit float matrices, if necessary
		if (pLeftObj->getType() != DataObject::MATRIX_F64) 	pLeftObj = pLeftObj->convert(DataObject::MATRIX_F64);
		if (pRightObj->getType() != DataObject::MATRIX_F64) pRightObj = pRightObj->convert(DataObject::MATRIX_F64);
		
		// Get typed pointers to the values
		MatrixF64Obj* pLMatrix = (MatrixF64Obj*)pLeftObj;
		MatrixF64Obj* pRMatrix = (MatrixF64Obj*)pRightObj;
		
		// Perform the equality comparison on the matrix elements
		return MatrixF64Obj::binArrayOp<CompOp<float64>, bool>(pLMatrix, pRMatrix);
	}	
}

// Function to implement the matrix multiplication operation
DataObject* matrixMultOp(const DataObject* pLeftObj, const DataObject* pRightObj);

// Function to implement the scalar multiplication operation
DataObject* scalarMultOp(const DataObject* pLeftObj, float64 scalar);

// Function to implement the matrix right division operation
DataObject* matrixRightDivOp(const DataObject* pLeftObj, const DataObject* pRightObj);

// Matrix binary operation function pointer type definitions
typedef DataObject* (*SCALAR_BINOP_FUNC)(const DataObject*, float64);
typedef DataObject* (*MATRIX_BINOP_FUNC)(const DataObject*, const DataObject*);

#endif // #ifndef MATRIXOPS_H_ 
