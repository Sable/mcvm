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
#ifndef RUNTIMEBASE_H_
#define RUNTIMEBASE_H_

// Header files
#include <string>
#include <vector>
#include "iir.h"
#include "objects.h"
#include "platform.h"

/***************************************************************
* Class   : RunError
* Purpose : Exception class to represent run-time error
* Initial : Maxime Chevalier-Boisvert on June 15, 2008
****************************************************************
Revisions and bug fixes:
*/
class RunError
{
public:
	
	// Constructor
	RunError(const std::string& text, const IIRNode* pNode = NULL);
	
	// Method to add information
	void addInfo(const std::string& text, const IIRNode* pNode = NULL);	
	
	// Method to obtain a textual error description
	std::string toString() const;
	
	// Static method to throw a runtime error exception
	static void throwError(const char* pText, const IIRNode* pNode);
	
private:
	
	// Error information method
	struct ErrorInfo
	{
		// Error description text
		std::string text;
		
		// Offending expression
		const IIRNode* pNode;
	};
	
	// Error stack type definition
	typedef std::vector<ErrorInfo> ErrorStack;
	
	// Error information stack
	ErrorStack m_errorStack;
};

/***************************************************************
* Class   : ReturnExcept
* Purpose : Exception class used to implement return statements
* Initial : Maxime Chevalier-Boisvert on February 26, 2009
****************************************************************
Revisions and bug fixes:
*/
class ReturnExcept
{
};

/***************************************************************
* Class   : BreakExcept
* Purpose : Exception class used to implement break statements
* Initial : Maxime Chevalier-Boisvert on February 26, 2009
****************************************************************
Revisions and bug fixes:
*/
class BreakExcept
{
};

/***************************************************************
* Class   : ContinueExcept
* Purpose : Exception class used for continue statements
* Initial : Maxime Chevalier-Boisvert on February 26, 2009
****************************************************************
Revisions and bug fixes:
*/
class ContinueExcept
{
};

// Function to evaluate the boolean value of an object
bool getBoolValue(const DataObject* pObject);

// Function to evaluate the integer value of an object
int32 getInt32Value(const DataObject* pObject);

// Function to evaluate the integer value of an object
int64 getInt64Value(const DataObject* pObject);

// Function to evaluate the floating-point value of an object
float64 getFloat64Value(const DataObject* pObject);

// Function to evaluate an object as an index
size_t getIndexValue(const DataObject* pObject);

// Function to create a blank/uninitialized object
DataObject* createBlankObj(DataObject::Type type);

#endif // #ifndef RUNTIMEBASE_H_ 
