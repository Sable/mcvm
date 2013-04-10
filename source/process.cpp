// =========================================================================== //
//                                                                             //
// Copyright 2008 Maxime Chevalier-Boisvert and McGill University.             //
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
#include <cstdio>
#include <cstdlib>
#include <iostream>

/***************************************************************
* Function: openPipe()
* Purpose : Open a pipe to an external process
* Initial : Maxime Chevalier-Boisvert on October 23, 2008
****************************************************************
Revisions and bug fixes:
*/
bool openPipe(const std::string& command, std::string* pOutput)
{
	// Attemot to open the pipe in text reading mode
	FILE* pPipeFile = popen(command.c_str(), "r");
	
	// Ensure the pipe was successfully opened
	if (!pPipeFile)
		return false;
	
	// While the end of file flag was not reached
	while (!feof(pPipeFile) && !ferror(pPipeFile))
	{
		// Create a buffer to read into
		char buffer[1024];
		
		// Read data from the pipe into the buffer
		size_t numRead = fread(&buffer[0], 1, sizeof(buffer) - 1, pPipeFile);
		
		// Add a null string terminator to the buffer
		buffer[numRead] = '\0';
		
		// Add the string to the output
		(*pOutput) += buffer;
	}	
		
	// If an error occurred
	if (ferror(pPipeFile))
	{
		// Close the pipe and signal the error
		pclose(pPipeFile);
		return false;
	}
	
	// Close the pipe file
	pclose(pPipeFile);
	
	// Operation successful
	return true;
}
