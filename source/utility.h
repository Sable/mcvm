// =========================================================================== //
//                                                                             //
// Copyright 2007-2009 Maxime Chevalier-Boisvert and McGill University.        //
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
#ifndef UTILITY_H_
#define UTILITY_H_

// Header files
#include <cmath>
#include <sstream>
#include <limits>
#include <string>
#include <vector>
#include <cstdio>

// Floating-point infinity constants
const float FLOAT_INFINITY = std::numeric_limits<float>::infinity();
const double DOUBLE_INFINITY = std::numeric_limits<double>::infinity();

// Trigonometric constants
const double PI			= 3.14159265358979323846;
const double HALF_PI	= 0.5 * PI;
const double TWO_PI		= 2.0 * PI;
const double FOUR_PI	= 4.0 * PI;

/***************************************************************
* Function: isPowerOf2()
* Purpose : Test that an unsigned integer is a power of two
* Initial : Maxime Chevalier-Boisvert on September 14, 2007
****************************************************************
Revisions and bug fixes:
*/
inline bool isPowerOf2(unsigned long int N)
{
	// Test that only 1 bit is on
    return ((N - 1) & N) == 0;
}

/***************************************************************
* Function: log2()
* Purpose : Compute base-2 logarithms
* Initial : Maxime Chevalier-Boisvert on February 18, 2009
****************************************************************
Revisions and bug fixes:
*/
inline double log2(double x)
{
	// Use the law of logarithms to compute a conversion factor
	static const double CONV_FACTOR = 1.0/log(2.0);
	
	// Perform the computation
	return log(x) * CONV_FACTOR;
}

/***************************************************************
* Function: charToHex()
* Purpose : Produce a hexadecimal representation of a char
* Initial : Maxime Chevalier-Boisvert on August 8, 2007
****************************************************************
Revisions and bug fixes:
*/
inline std::string charToHex(char C)
{
	// Declare a buffer for the hexadecimal character
	char Hex[8];
			
	// Cast the character to an integer
	int Char = C;
			
	// Produce a hexadecimal representation of the character
	sprintf(Hex, "%02X", Char);
	
	// Return the string
	return Hex;
}

/***************************************************************
* Function: toString()
* Purpose : Convert a basic type to a string
* Initial : Maxime Chevalier-Boisvert on April 1, 2007
****************************************************************
Revisions and bug fixes:
*/
template <class T> std::string toString(T V)
{
    // create a stringstream object
    std::stringstream Stream;
    
    // Input the value in the stream
    Stream << V;
    
    // Return the string buffer
    return Stream.str();
}

/***************************************************************
* Function: square()
* Purpose : Compute the square of a number
* Initial : Maxime Chevalier-Boisvert on April 1, 2007
****************************************************************
Revisions and bug fixes:
*/
template <class T> T square(T value)
{
	// Compute the square
	return value * value;
}

/***************************************************************
* Function: sign
* Purpose : Compute the sign of a value
* Initial : Maxime Chevalier-Boisvert on May 7, 2007
****************************************************************
Revisions and bug fixes:
*/
template <class T> int sign(T value)
{
    // Compute the sign
    if (value > 0)
        return 1;
    else if (value < 0)
        return -1;
    else 
        return 0;
}

/***************************************************************
* Function: notANumber()
* Purpose : Test if a floating-point value is NaN
* Initial : Maxime Chevalier-Boisvert on May 7, 2007
****************************************************************
Revisions and bug fixes:
*/
template <class F> bool notANumber(F value)
{
	// Test for self-inequality
	return (value != value);
}

/***************************************************************
* Function: isInteger()
* Purpose : Test if a floating-point value is an integer
* Initial : Maxime Chevalier-Boisvert on May 6, 2009
****************************************************************
Revisions and bug fixes:
*/
template <class F> bool isInteger(F value)
{
	// Extract the integer and floating-point parts
	F floatPart, intPart;
	floatPart = modf(value , &intPart);

	// Test that the floating-point part is 0
	return (floatPart == 0);	
}

/***************************************************************
* Function: round()
* Purpose : Round a floating-point number
* Initial : Maxime Chevalier-Boisvert on February 2, 2009
****************************************************************
Revisions and bug fixes:
*/
template <class F> F round(F value)
{
	// Get the integer and fractional parts of the number
	F intPart;
	F fracPart = modf(value , &intPart);

	// Perform the rounding
	if (fracPart > 0.5f)
		return intPart + 1;
	else
		return intPart;	
}

/***************************************************************
* Function: tokenMatch()
* Purpose : Try to match a token with a portion of a string
* Initial : Maxime Chevalier-Boisvert on October 19, 2008
****************************************************************
Revisions and bug fixes:
*/
inline bool tokenMatch(const std::string& string, size_t charIndex, const std::string& token)
{
	// Get the length of the token string
	size_t tokenLength = token.length();
	
	// Perform the substring comparison
	return (string.compare(charIndex, tokenLength, token, 0, tokenLength) == 0);
}

/***************************************************************
* Class   : StrHashFunc
* Purpose : Hash function object for C++ strings
* Initial : Maxime Chevalier-Boisvert on July 26, 2007
****************************************************************
Revisions and bug fixes:
*/
class StrHashFunc
{
public:

	// Evaluation operator
	size_t operator() (const std::string& String) const
	{
		// Use the string hash method
		return HashString(String.c_str());
	}
	
	// Static method to compute the hash code for a string
	inline static size_t HashString(const char* pString)
	{
		// Declare a variable for the hash value
		size_t Hash = 0;
		
		// Consider each character in the hash
		for (const char* pChar = pString; *pChar != '\0'; ++pChar)
			Hash = (Hash << 6) + (Hash << 16) - Hash + *pChar;

		// Return the hash value
		return Hash;
	}
};

/***************************************************************
* Class   : IntHashFunc
* Purpose : Hash function object for pointer/integer values
* Initial : Maxime Chevalier-Boisvert on March 15, 2009
****************************************************************
Revisions and bug fixes:
*/
template <class T> class IntHashFunc
{
public:

	// Evaluation operator
	size_t operator() (const T& value) const
	{
		// Cast the value into a size_t
		return size_t(value);
	}
};

/***************************************************************
* Function: isWhiteSpace()
* Purpose : Test if a text character is whitespace
* Initial : Maxime Chevalier-Boisvert on October 19, 2008
****************************************************************
Revisions and bug fixes:
*/
inline bool isWhiteSpace(char c)
{
	// Perform the test
	return (c == ' ' || c == '\t' || c == '\r' || c == '\n');
}

// Function to indent a block of text
std::string indentText(const std::string& input, const std::string& indent = "\t");

// Function to convert a character to lowercase
char lowerCase(char c);

// Function to convert a string to lowercase
std::string lowerCase(const std::string& string);

// Function to compare strings without case sensitivity
bool compareNoCase(const std::string& str1, const std::string& str2);

// Function to remove leading and trailing characters
std::string trimString(const std::string& input, const std::string& characters);

// Function to tokenize a string into its tokens
std::vector<std::string> tokenize(const std::string& input, const std::string& delimiters, bool includeDelims, bool trimTokens);

// Function to read a text file into a string
bool readTextFile(const std::string& fileName, std::string& output);

#endif // #ifndef UTILITY_H_ 
