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
#include <cassert>
#include <iostream>
#include <sstream>
#include "configmanager.h"
#include "chararrayobj.h"
#include "runtimebase.h"
#include "interpreter.h"

// Static starting directory config variable
ConfigVar ConfigManager::s_startDirVar("start_dir", ConfigVar::STRING, ".");

// Static verbose output mode config variable
ConfigVar ConfigManager::s_verboseVar("verbose", ConfigVar::BOOL, "false");

// Static library function to set config variables
LibFunction ConfigManager::s_setVarCmd("mcvm_set_var", ConfigManager::setVarCmd);

// Static library function to list config variables
LibFunction ConfigManager::s_listVarsCmd("mcvm_list_vars", ConfigManager::listVarsCmd);

// Static config variable map instance
ConfigManager::VarMap ConfigManager::s_varMap;
	
// Static program name instance
std::string ConfigManager::s_progName = "";
	
// Static target file name instance
std::string ConfigManager::s_fileName = "";

/***************************************************************
* Function: ConfigVar::ConfigVar()
* Purpose : Constructor for config variable class
* Initial : Maxime Chevalier-Boisvert on February 4, 2009
****************************************************************
Revisions and bug fixes:
*/
ConfigVar::ConfigVar(
	const std::string& name,
	Type type,
	const std::string& defaultVal,
	double minVal,
	double maxVal
)
{
	// Ensure the variable name is valid
	assert (!name.empty());
	
	// Ensure that the name contains no spaces
	assert (name.find(" ") == std::string::npos);
	
	// Ensure the min and max values are valid
	assert (minVal < maxVal);
	
	// Store the variable name
	m_varName = name;
	
	// Store the variable type
	m_type = type;
	
	// Store the min and max values
	m_minValue = minVal;
	m_maxValue = maxVal;
	
	// Ensure that we can set the initial default value
	assert (setValue(defaultVal));
}
	 
/***************************************************************
* Function: ConfigVar::setValue()
* Purpose : Set the value of the variable from a string
* Initial : Maxime Chevalier-Boisvert on February 4, 2009
****************************************************************
Revisions and bug fixes:
*/
bool ConfigVar::setValue(const std::string& newValue)
{
	// Set the string value
	m_stringValue = newValue;
	
	// If this is a boolean value
	if (m_type == BOOL)
	{
		// If the value matches a true pattern
		if (
			newValue == "1"	||
			newValue == "TRUE" ||
			newValue == "True" ||
			newValue == "true" ||
			newValue == "ON" ||
			newValue == "On" ||
			newValue == "on")
		{
			// Set the boolean value to true
			m_boolValue = true;
		}
		
		// Otherwise, if the value matches a false pattern
		else if (
			newValue == "0"	||
			newValue == "FALSE" ||
			newValue == "False" ||
			newValue == "false" ||
			newValue == "OFF" ||
			newValue == "Off" ||
			newValue == "off")	
		{
			// Set the boolean value to false
			m_boolValue = false;
		}
		
		// Otherwise, invalid boolean value
		else
		{
			// Log the error
			std::cout << "ERROR: invalid boolean value" << std::endl;
		}
	}
	
	// Otherwise, if this is an integer value
	else if (m_type == INT)
	{
		// Create a string stream object for the string
		std::stringstream strStream(newValue);
		
		// Attempt to extract the integer value
		long int intVal;
		strStream >> intVal;
		
		// If the extraction failed
		if (strStream.fail() || !strStream.eof())
		{
			// Log and return the error
			std::cout << "ERROR: invalid integer value" << std::endl;
			return false;
		}		
		
		// If the value is out of range
		if (intVal < m_minValue || intVal > m_maxValue)
		{
			// Log and return the error
			std::cout << "ERROR: integer value out of range" << std::endl;
			return false;
		}
		
		// Set the integer value
		m_intValue = intVal;
		
		// Set the floating-point value
		m_floatValue = intVal;
		
		// Set the boolean value
		m_boolValue = (m_intValue != 0);
	}

	// Otherwise, if this is a floating-point value
	else if (m_type == FLOAT)
	{
		// Create a string stream object for the string
		std::stringstream strStream(newValue);
		
		// Attempt to extract the floating-point value
		double floatVal;
		strStream >> floatVal;
		
		// If the extraction failed
		if (strStream.fail() || !strStream.eof())
		{
			// Log and return the error
			std::cout << "ERROR: invalid floating-point value" << std::endl;
			return false;
		}		
		
		// If the value is out of range
		if (floatVal < m_minValue || floatVal > m_maxValue)
		{
			// Log and return the error
			std::cout << "ERROR: floating-point value out of range" << std::endl;
			return false;
		}

		// Set the floating-point value
		m_floatValue = floatVal;

		// Get the integer and fractional parts of the value
		double fracPart;
		double intPart;
		fracPart = modf(floatVal, &intPart);
		
		// If the floating-point part is zero
		if (fracPart == 0)
		{
			// Set the integer value
			m_intValue = (long int)floatVal;
		}
		
		// Set the boolean value
		m_boolValue = (m_floatValue != 0);			
	}
	
	// Nothing went wrong
	return true;
}

/***************************************************************
* Function: ConfigVar::registerVar()
* Purpose : Register a config variable
* Initial : Maxime Chevalier-Boisvert on February 4, 2009
****************************************************************
Revisions and bug fixes:
*/
void ConfigManager::registerVar(ConfigVar* pConfigVar)
{
	// Ensure that the pointer is valid
	assert (pConfigVar != NULL);
	
	// Ensure that the variable name is not already registered
	assert (s_varMap.find(pConfigVar->getVarName()) == s_varMap.end());
	
	// Register the config variable
	s_varMap[pConfigVar->getVarName()] = pConfigVar;
}

/***************************************************************
* Function: ConfigVar::loadCfgFile()
* Purpose : Load a config file
* Initial : Maxime Chevalier-Boisvert on February 4, 2009
****************************************************************
Revisions and bug fixes:
*/
bool ConfigManager::loadCfgFile(const std::string& filePath)
{
	// TODO	
	
	// Nothing went wrong
	return true;
}

/***************************************************************
* Function: ConfigVar::parseCmdArgs()
* Purpose : Parse command-line arguments
* Initial : Maxime Chevalier-Boisvert on February 4, 2009
****************************************************************
Revisions and bug fixes:
*/
bool ConfigManager::parseCmdArgs(int argCount, char** argVal)
{
	// Ensure that there is at least 1 argument
	assert (argCount > 0);
	
	// Extract the program name
	s_progName = argVal[0];
	
	// Declare a variable for the argument index
	int argIndex = 1;
	
	// For each pair of command-line arguments
	for (; argIndex < argCount - 1; argIndex += 2)
	{
		// Get the variable name and value
		std::string varName = argVal[argIndex];
		std::string value = argVal[argIndex + 1];
		
		// If the variable name format is invalid
		if (varName.empty() || varName[0] != '-')
		{
			// Log the error and abort
			std::cout << "ERROR: invalid argument format \"" << varName << "\"" << std::endl;
			return false;
		}
		
		// Extract the variable name without the dash
		varName = varName.substr(1, varName.length() - 1);
		
		// Attempt to set the variable value
		setVariable(varName, value);
	}
	
	// If one argument remains
	if (argCount > 1 && argIndex < argCount)
	{	
		// Set the target file name
		s_fileName = argVal[argCount - 1];
	}
	
	// Nothing went wrong
	return true;
}

/***************************************************************
* Function: ConfigVar::setVariable()
* Purpose : Set the value of a config variable
* Initial : Maxime Chevalier-Boisvert on February 4, 2009
****************************************************************
Revisions and bug fixes:
*/
bool ConfigManager::setVariable(const std::string& varName, const std::string& value)
{
	// Attempt to find the variable
	VarMap::iterator varItr = s_varMap.find(varName);
	
	// If the variable is not found
	if (varItr == s_varMap.end())
	{
		// Log the error and return
		std::cout << "ERROR: variable not found \"" << varName << "\"" << std::endl;
		return false;		
	}
	
	// Set the value for this variable
	varItr->second->setValue(value);
	
	// Nothing went wrong
	return true;
}

/***************************************************************
* Function: initialize()
* Purpose : initialize the config manager
* Initial : Maxime Chevalier-Boisvert on February 5, 2009
****************************************************************
Revisions and bug fixes:
*/
void ConfigManager::initialize()
{
	// Register the local config variables
	registerVar(&s_startDirVar);
	registerVar(&s_verboseVar);
	
	// Register the local library functions
	Interpreter::setBinding(s_setVarCmd.getFuncName(), (DataObject*)&s_setVarCmd);
	Interpreter::setBinding(s_listVarsCmd.getFuncName(), (DataObject*)&s_listVarsCmd);
}

/***************************************************************
* Function: ConfigManager::setVarCmd()
* Purpose : Library function to set config variables
* Initial : Maxime Chevalier-Boisvert on February 5, 2009
****************************************************************
Revisions and bug fixes:
*/
ArrayObj* ConfigManager::setVarCmd(ArrayObj* pArguments)
{
	// Ensure there are 2 arguments
	if (pArguments->getSize() != 2)
		throw RunError("expected 2 arguments");
	
	// Ensure that both arguments are strings
	if (pArguments->getObject(0)->getType() != DataObject::CHARARRAY ||
		pArguments->getObject(1)->getType() != DataObject::CHARARRAY)
		throw RunError("expected string arguments");

	// Get pointers to the arguments
	CharArrayObj* pVarName = (CharArrayObj*)pArguments->getObject(0);
	CharArrayObj* pValue = (CharArrayObj*)pArguments->getObject(1);
	
	// Attempt to set the variable
	bool result = ConfigManager::setVariable(pVarName->getString(), pValue->getString());
	
	// If the operation failed, throw an exception
	if (result == false)
		throw RunError("failed to set config variable");
	
	// Return nothing
	return new ArrayObj();
}

/***************************************************************
* Function: ConfigManager::listVarsCmd()
* Purpose : Library function to list config variables
* Initial : Maxime Chevalier-Boisvert on July 12, 2009
****************************************************************
Revisions and bug fixes:
*/
ArrayObj* ConfigManager::listVarsCmd(ArrayObj* pArguments)
{
	// Ensure there are 0 arguments
	if (pArguments->getSize() != 0)
		throw RunError("expected 0 arguments");
	
	// Print that this is a config variable listing
	std::cout << "Config variable listing: " << std::endl;
	
	// For each config variable
	for (VarMap::const_iterator itr = s_varMap.begin(); itr != s_varMap.end(); ++itr)
	{
		// Get a pointer to the config variable
		ConfigVar* pConfigVar = itr->second;
		
		// Log the variable and its value
		std::cout << pConfigVar->getVarName() << " = " << pConfigVar->getStringValue() << std::endl;
	}	
	
	// Return nothing
	return new ArrayObj();
}
