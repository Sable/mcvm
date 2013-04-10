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

// Include guards
#ifndef CHARARRAYOBJ_H_
#define CHARARRAYOBJ_H_

// Header files
#include "objects.h"
#include "matrixobjs.h"

// Method to obtain a string representation of a character array
template <> std::string MatrixObj<char>::toString() const;

/***************************************************************
* Class   : CharArrayObj
* Purpose : Class to represent character arrays (strings)
* Initial : Maxime Chevalier-Boisvert on February 12, 2009
****************************************************************
Revisions and bug fixes:
*/
class CharArrayObj : public MatrixObj<char>
{
public:

	// Default constructor
	CharArrayObj()
	{
	}
	
	// String constant constructor
	CharArrayObj(const std::string& string)
	{
		// Get the string length
		size_t strLen = string.length(); 
		
		// Set the matrix size to 1xN, where N is the string length
		m_size.resize(2);
		m_size[0] = 1;
		m_size[1] = strLen;
		
		// Allocate data for the matrix
		allocMatrix();
		
		// Get a C-string pointer for the string
		const char* pCStr = string.c_str();
		
		// Copy the string data into the matrix
		memcpy(m_pElements, pCStr, strLen * sizeof(char));		
	}
	
	// Method to get a 1D string from char array contents
	std::string getString()
	{
		// Create a string object and fill it with the character data
		std::string outStr(m_pElements, m_pElements + m_numElements);

		// Return the output string
		return outStr;
	}
};

#endif // #ifndef CHARARRAYOBJ_H_ 
