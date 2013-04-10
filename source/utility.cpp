// =========================================================================== //
//                                                                             //
// Copyright 2004-2009 Maxime Chevalier-Boisvert and McGill University.        //
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
#include <cstring>
#include "utility.h"

/***************************************************************
* Function: indentText()
* Purpose : Indent a block of text
* Initial : Maxime Chevalier-Boisvert on October 29, 2008
****************************************************************
Revisions and bug fixes:
*/
std::string indentText(const std::string& input, const std::string& indent)
{
	// Declare a string for the output
	std::string output;
	
	// Initialize the line start index
	size_t lineStart = 0;
	
	// For each character of the input
	for (size_t index = 0; index < input.length(); ++index)
	{
		// If this is a newline character
		if (input[index] == '\n')
		{
			// Indent this line in the output
			output += indent + input.substr(lineStart, index + 1 - lineStart);
			
			// Update the line start index
			lineStart = index + 1;
		}
	}	
	
	// If the input did not end with a newline
	if (lineStart != input.length())
	{
		// Indent the last line in the output
		output += indent + input.substr(lineStart, input.length() - lineStart);
	}
	
	// Return the output
	return output;
}

/***************************************************************
* Function: lowerCase(char)
* Purpose : Convert a character to lowercase
* Initial : Maxime Chevalier-Boisvert on April 12, 2007
****************************************************************
Revisions and bug fixes:
*/
char lowerCase(char c)
{
    // Convert the character to lower case
    return (c >= 'A' && c <= 'Z')? (c + ('a' - 'A')):c;
}

/***************************************************************
* Function: lowerCase(string)
* Purpose : Convert a string to lowercase
* Initial : Maxime Chevalier-Boisvert on April 12, 2007
****************************************************************
Revisions and bug fixes:
*/
std::string lowerCase(const std::string& string)
{
    // Declare a string for the output
    std::string Output;
    
    // Convert each character to lowercase
    for (std::string::const_iterator Itr = string.begin(); Itr != string.end(); ++Itr)
        Output += lowerCase(*Itr);
    
    // Return the output string
    return Output;
}

/***************************************************************
* Function: compareNoCase()
* Purpose : Compare strings without case sensitivity
* Initial : Maxime Chevalier-Boisvert on April 12, 2007
****************************************************************
Revisions and bug fixes:
*/
bool compareNoCase(const std::string& str1, const std::string& str2)
{
    // Compare the strings in lower-case form
    return (lowerCase(str1) == lowerCase(str2));
}

/***************************************************************
* Function: trimString()
* Purpose : Remove leading and trailing characters
* Initial : Maxime Chevalier-Boisvert on Oct 31, 2004
****************************************************************
Revisions and bug fixes:
*/
std::string trimString(const std::string& input, const std::string& characters)
{
	// If the input is an empty string, return it back
	if (input.length() == 0)
		return input;

	// Declare start and stop indices
	int StartIndex;
	int StopIndex;

	// Move the start index forward as long as we meet spaces
	for (StartIndex = 0; StartIndex < (int)input.length() && characters.find(input[StartIndex]) != std::string::npos; ++StartIndex);

	// Move the stop index backwards as long as we meet spaces
	for (StopIndex = (int)input.length() - 1; StopIndex >= 0 && characters.find(input[StopIndex]) != std::string::npos; --StopIndex);

	// If we went past the start of the string, return an empty string
	if (StopIndex == -1)
		return std::string("");

	// Return the trimmed string
	return input.substr(StartIndex, StopIndex - StartIndex + 1);
}

/***************************************************************
* Function: tokenize()
* Purpose : tokenize a string into its tokens
* Initial : Maxime Chevalier-Boisvert on Oct 31, 2004
****************************************************************
Revisions and bug fixes:
*/
std::vector<std::string> tokenize(const std::string& input, const std::string& delimiters, bool includeDelims, bool trimTokens)
{
	// Create a vector to store the tokens
	std::vector<std::string> Tokens;

	// Declare variables for the index and length of tokens
	size_t TokenIndex  = 0;
	size_t TokenLength = 0;

	// Declare a string to store individual tokens
	std::string Token;

	// Declare a variable to store the current character index
	size_t CharIndex = 0;

	// While we are not past the end of the string
	while (CharIndex < input.length())
	{
		// Set the token index to this character and reset the token length
		TokenIndex  = CharIndex;
		TokenLength = 0;

		// While the current character is not a delimiter
		while (delimiters.find(input[CharIndex]) == std::string::npos && CharIndex < input.length())
		{
			// Increment the token length and the character index
			++TokenLength;
			++CharIndex;
		}

		// If the token has at least 1 character
		if (TokenLength > 0)
		{
			// Extract the token
			Token = input.substr(TokenIndex, TokenLength);

			// Trim the spaces if specified
			if (trimTokens)
				Token = trimString(Token, " ");

			// Add the token to our vector
			Tokens.push_back(Token);
		}
		
		// While the current character is a delimiter
		while (strchr(delimiters.c_str(), input[CharIndex]) && CharIndex < input.length())
		{
			// If we should include delimiters
			if (includeDelims)
			{
				// Extract the delimiter
				Token = input.substr(CharIndex, 1);
				
				// Add the token to our vector
				Tokens.push_back(Token);
			}

			// Increment the character index
			++CharIndex;
		}
	}
	
	// Return the tokens
	return Tokens;
}

/***************************************************************
* Function: readTextFile()
* Purpose : Read a text file into a string
* Initial : Maxime Chevalier-Boisvert on March 4, 2009
****************************************************************
Revisions and bug fixes:
*/
bool readTextFile(const std::string& fileName, std::string& output)
{
	// Attempt to open the file in binary reading mode
	FILE* pFile = fopen(fileName.c_str(), "rb");

	// Make sure that the XML file was successfully opened
	if (!pFile)
		return false;

	// Seek to the end of the file
	fseek(pFile, 0, SEEK_END);

	// Get the file size
	size_t fileSize = ftell(pFile);

	// Seek back to the start of the file
	fseek(pFile, 0, SEEK_SET);

	// Allocate a buffer to read the file
	char* pBuffer = new char[fileSize + 1];

	// Read the file into the buffer
	size_t sizeRead = fread(pBuffer, 1, fileSize, pFile);

	// Close the input file
	fclose(pFile);

	// Make sure the file was entirely read
	if (sizeRead != fileSize)
		return false;

	// Append a null character
	pBuffer[fileSize] = '\0';

	// Store the text in the output string
	output = pBuffer;

	// Delete the temporary buffer
	delete [] pBuffer;
	
	// File read successfully
	return true;
}
