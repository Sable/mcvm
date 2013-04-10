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
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <limits.h>
#include "filesystem.h"

/***************************************************************
* Function: getWorkingDir()
* Purpose : Get the current working directory
* Initial : Maxime Chevalier-Boisvert on December 19, 2008
****************************************************************
Revisions and bug fixes:
*/
std::string getWorkingDir()
{
	// Create a buffer to store the path
	char buffer[PATH_MAX + 1];
	
	// Attempt to get the current working directory
	char* result = getcwd(buffer, sizeof(buffer));
	
	// If the operation failed, return the empty string
	if (result == NULL)
		return std::string();

	// Create and return a string from the buffer
	return std::string(buffer);
}

/***************************************************************
* Function: setWorkingDir()
* Purpose : Set the current working directory
* Initial : Maxime Chevalier-Boisvert on December 19, 2008
****************************************************************
Revisions and bug fixes:
*/
bool setWorkingDir(const std::string& dir)
{
	// Attempt to change to the specified directory
	int result = chdir(dir.c_str());
	
	// Return true if the operation was successful
	return (result == 0);
}

/***************************************************************
* Function: getAbsPath()
* Purpose : Get an absolute path for a relative file name
* Initial : Maxime Chevalier-Boisvert on December 24, 2008
****************************************************************
Revisions and bug fixes:
*/
std::string getAbsPath(const std::string& fileName)
{
	// Create a buffer to store the path
	char buffer[PATH_MAX + 1];
	
	// Attempt to get the absolute path
	char* result = realpath(fileName.c_str(), buffer);
	
	// If the operation failed, return the empty string
	if (result == NULL)
		return std::string();

	// Create and return a string from the buffer
	return std::string(buffer);	
}
