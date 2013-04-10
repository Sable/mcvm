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
#ifndef CONFIGMANAGER_H_
#define CONFIGMANAGER_H_

// Header files
#include <map>
#include <string>
#include "utility.h"
#include "functions.h"

/***************************************************************
* Class   : ConfigVar
* Purpose : Manage configuration parameters
* Initial : Maxime Chevalier-Boisvert on February 4, 2009
****************************************************************
Revisions and bug fixes:
*/
class ConfigVar
{
public:
	
	// Configuration variable types
	enum Type
	{
		BOOL,
		INT,
		FLOAT,
		STRING
	};
	
	// Constructor
	ConfigVar(
		const std::string& name,
		Type type,
		const std::string& defaultVal,
		double minVal = -DOUBLE_INFINITY,
		double maxVal = DOUBLE_INFINITY
	);	
	
	// Method to set the value of the variable from a string
	bool setValue(const std::string& newValue);
	
	// Accessor to get the variable name
	const std::string& getVarName() const { return m_varName; }
	
	// Accessor to get the boolean value
	bool getBoolValue() const { return m_boolValue; }
	
	// Accessor to get the integer value
	long int getIntValue() const { return m_intValue; }
	
	// Accessor to get the floating-point value
	double getFloatValue() const { return m_floatValue; }
	
	// Accessor to get the string value
	const std::string& getStringValue() const { return m_stringValue; } 

	// Operator to get the boolean value
	operator bool () { return getBoolValue(); }
	
private:
	
	// Variable name
	std::string m_varName;
	
	// Type of this variable
	Type m_type;
	
	// Minimum and maximum allowed values
	double m_minValue;
	double m_maxValue;
	
	// Boolean value
	bool m_boolValue;
	
	// Integer value
	long int m_intValue;
	
	// Index value
	double m_floatValue;
	
	// String value
	std::string m_stringValue;
};

/***************************************************************
* Class   : ConfigManager
* Purpose : Manage configuration parameters
* Initial : Maxime Chevalier-Boisvert on February 4, 2009
****************************************************************
Revisions and bug fixes:
*/
class ConfigManager
{
public:
		
	// Method to register a config variable
	static void registerVar(ConfigVar* pConfigVar);
	
	// Method to load a config file
	static bool loadCfgFile(const std::string& filePath);
	
	// Method to parse command-line arguments
	static bool parseCmdArgs(int argCount, char** argVal);
	
	// Method to set the value of a config variable
	static bool setVariable(const std::string& varName, const std::string& value);
	
	// Accessor to get the program name
	static const std::string& getProgName() { return s_progName; }
	
	// Accessor to get the target file name
	static const std::string& getFileName() { return s_fileName; }

	// Method to initialize the config manager
	static void initialize();

	// Starting directory config variable
	static ConfigVar s_startDirVar;
	
	// Verbose output mode config variable
	static ConfigVar s_verboseVar;
	
	// Library function to set config variables
	static ArrayObj* setVarCmd(ArrayObj* pArguments);
	static LibFunction s_setVarCmd;
	
	// Library function to list config variables
	static ArrayObj* listVarsCmd(ArrayObj* pArguments);
	static LibFunction s_listVarsCmd;
	
private:
	
	// Variable map type definition
	typedef std::map<std::string, ConfigVar*> VarMap;
	
	// Config variable map
	static VarMap s_varMap;
	
	// Program name
	static std::string s_progName;
	
	// Target file name
	static std::string s_fileName;
};

#endif // #ifndef CONFIGMANAGER_H_
