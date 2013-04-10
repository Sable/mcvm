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
#ifndef STDLIB_H_
#define STDLIB_H_

// Header files
#include "functions.h"
#include "matrixobjs.h"

// Standard library name space
namespace StdLib
{
	// Library function used to compute absolute values
	extern LibFunction abs;

	// Library function used to test if a matrix contains nonzero elements
	extern LibFunction any;
	
	// Library function used to apply the bitwise AND operation
	extern LibFunction bitwsand;
	
	// Library function used to create a block diagonal matrix
	extern LibFunction blkdiag;
	
	// Library function used to change the working directory
	extern LibFunction cd;
	
	// Library function used apply the ceiling function
	extern LibFunction ceil;
	
	// Library function used create cell arrays
	extern LibFunction cell;
	
	// Library function used to get the current time
	extern LibFunction clock;
	
	// Library function used apply the cosine function
	extern LibFunction cos;
	
	// Library function used for creating diagonal matrices
	extern LibFunction diag;
	
	// Library function used for displaying to the console
	extern LibFunction disp;

	// Library function used for computing dot products of vectors
	extern LibFunction dot;
	
	// Library function used to evaluate text strings as statements
	extern LibFunction eval;

	// Library function used to obtain floating-point epsilon values
	extern LibFunction eps;
	
	// Library function used to determine the existence of an object
	extern LibFunction exist;
	
	// Library function used to apply the exponential function
	extern LibFunction exp;

	// Library function used to generate identity matrices
	extern LibFunction eye;
	
	// Library function used to create matrices of false logical values
	extern LibFunction false_;
	
	// Library function used to close files by file handle
	extern LibFunction fclose;	
	
	// Library function used to evaluate functions
	extern LibFunction feval;

	// Library function used to find nonzero values in matrices
	extern LibFunction find;
	
	// Library function used to round toward zero
	extern LibFunction fix;
	
	// Library function used apply the floor function
	extern LibFunction floor;

	// Library function used to open files and return a file handle
	extern LibFunction fopen;
	
	// Library function used to print text strings into streams
	extern LibFunction fprintf;

	// Library function that returns the imaginary constant i
	extern LibFunction i;
	
	// Library function used to determine if an object is a cell array
	extern LibFunction iscell;
	
	// Library function used to determine if a matrix is empty
	extern LibFunction isempty;
	
	// Library function used to determine if matrices are equal
	extern LibFunction isequal;	
	
	// Library function used to determine if an object is a numeric value
	extern LibFunction isnumeric;		
	
	// Library function used compute the length of matrices
	extern LibFunction length;
	
	// Library function used to load workspace variables from files
	extern LibFunction load;
	
	// Library function used to compute base 2 logarithms
	extern LibFunction log2;

	// Library function used to list directory contents
	extern LibFunction ls;	
	
	// Library function used to find the largest value in a vector
	extern LibFunction max;
	
	// Library function used to compute means
	extern LibFunction mean;

	// Library function used to find the smallest value in a vector
	extern LibFunction min;
	
	// Library function used to compute modulos
	extern LibFunction mod;

	// Library function used to perform logical negation
	extern LibFunction not_;

	// Library function used to convert numerical values to strings
	extern LibFunction num2str;
	
	// Library function used to obtain the number of elements in a matrix
	extern LibFunction numel;
	
	// Library function used to create and initialize matrices
	extern LibFunction ones;

	// Library function that returns the constant pi
	extern LibFunction pi;
	
	// Library function used to get the current working directory
	extern LibFunction pwd;
	
	// Library function used to generate uniform random numbers
	extern LibFunction rand;
	
	// Library function used to change the shape of matrices
	extern LibFunction reshape;

	// Library function used to round floating-point numbers
	extern LibFunction round;

	// Library function used to obtain the sign of numbers
	extern LibFunction sign;
	
	// Library function used to compute the sine of numbers
	extern LibFunction sin;
	
	// Library function used to obtain the size of matrices
	extern LibFunction size;
	
	// Library function used to sort vectors of numbers
	extern LibFunction sort;

	// Library function used to format text strings
	extern LibFunction sprintf;
	
	// Library function used to eliminate singleton dimensions in matrices
	extern LibFunction squeeze;
	
	// Library function used to compute square roots
	extern LibFunction sqrt;

	// Library function used to perform string concatenation
	extern LibFunction strcat;
	
	// Library function used to perform string equality comparison
	extern LibFunction strcmp;
	
	// Library function used to compute sums of array elements
	extern LibFunction sum;

	// Library function used to execute a system command
	extern LibFunction system;
	
	// Library function used to begin timing measurements
	extern LibFunction tic;
	
	// Library function used to stop timing measurements
	extern LibFunction toc;
	
	// Library function used to create a toeplitz matrix
	extern LibFunction toeplitz;	
	
	// Library function used to create matrices of true logical values
	extern LibFunction true_;
	
	// Library function used to find unique elements in a matrix
	extern LibFunction unique;
	
	// Library function used to create and initialize matrices
	extern LibFunction zeros;
	
	// Function to load the library functions
	void loadLibrary();
};

#endif // #ifndef STDLIB_H_ 
