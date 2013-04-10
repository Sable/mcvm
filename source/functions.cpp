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
#include <iostream>
#include "functions.h"
#include "transform_loops.h"
#include "transform_switch.h"
#include "transform_endexpr.h"
#include "transform_logic.h"
#include "transform_split.h"
#include "interpreter.h"
#include "utility.h"

/***************************************************************
* Function: ProgFunction::ProgFunction()
* Purpose : Constructor for program function class
* Initial : Maxime Chevalier-Boisvert on January 23, 2009
****************************************************************
Revisions and bug fixes:
*/
ProgFunction::ProgFunction(
	const std::string& name, 
	const ParamVector& inParams, 
	const ParamVector& outParams,
	const FuncVector& nestedFuncs,
	StmtSequence* stmts, 
	bool isScript,
	bool isClosure
)
: m_inputParams(inParams), 
  m_outputParams(outParams),
  m_nestedFuncs(nestedFuncs),
  m_pParent(NULL),
  m_nextTempId(0)
{ 
	// Indicate that this is a program function
	m_isProgFunction = true;
	
	// Set the function name
	m_funcName = name;
	
	// Store the original function body
	m_pOrigBody = stmts;
	
	// Perform loop simplifications on the original body
	m_pCurrentBody = transformLoops(m_pOrigBody, this);
	
	// Perform switch simplifications on the function body
	m_pCurrentBody = transformSwitch(m_pCurrentBody, this);
	
	// Process range end expressions in the function body
	m_pCurrentBody = processEndExpr(m_pCurrentBody, this);
	
	// If type validation is enabled
	if (Interpreter::s_validateTypes.getBoolValue() == true)
	{
		// Convert the function body to split form
		m_pCurrentBody = transformLogic(m_pCurrentBody, this);
		m_pCurrentBody = splitSequence(m_pCurrentBody, this);
	}		
	
	// Store the script flag value
	m_isScript = isScript;
	
	// Store the closure flag value
	m_isClosure = isClosure;
}

/***************************************************************
* Function: ProgFunction::copy()
* Purpose : Copy this IIR node recursively
* Initial : Maxime Chevalier-Boisvert on November 6, 2008
****************************************************************
Revisions and bug fixes:
*/
ProgFunction* ProgFunction::copy() const
{
	// Create vectors to store the input and output parameter copies
	ParamVector inParams;
	ParamVector outParams;
	
	// Create a vector to store the nested function copies
	FuncVector nestedFuncs;
	
	// Copy the input parameters
	for (ParamVector::const_iterator itr = m_inputParams.begin(); itr != m_inputParams.end(); ++itr)
		inParams.push_back((SymbolExpr*)(*itr)->copy());

	// Copy the output parameters
	for (ParamVector::const_iterator itr = m_outputParams.begin(); itr != m_outputParams.end(); ++itr)
		outParams.push_back((SymbolExpr*)(*itr)->copy());
	
	// Copy the nested functions
	for (FuncVector::const_iterator itr = m_nestedFuncs.begin(); itr != m_nestedFuncs.end(); ++itr)
		nestedFuncs.push_back((ProgFunction*)(*itr)->copy());
	
	// Create a copy of this node
	ProgFunction* pNewFunc = new ProgFunction(
		m_funcName,
		inParams,
		outParams,
		nestedFuncs,
		(StmtSequence*)m_pOrigBody->copy(),
		m_isScript,
		m_isClosure
	);
	
	// Set the parent pointer
	pNewFunc->setParent(m_pParent);
	
	// Set the next available temp id
	pNewFunc->m_nextTempId = m_nextTempId;
	
	// Return the new function object
	return pNewFunc;
}

/***************************************************************
* Function: ProgFunction::toString()
* Purpose : Generate a text representation of this IIR node
* Initial : Maxime Chevalier-Boisvert on November 6, 2008
****************************************************************
Revisions and bug fixes:
*/
std::string ProgFunction::toString() const
{
	// Declare a string to the output
	std::string output;

	// Open the output parameter list
	output += "function [";
	
	// For each output parameter
	for (ParamVector::const_iterator itr = m_outputParams.begin(); itr != m_outputParams.end(); ++itr)
	{
		// Add this parameter to the string
		output += (*itr)->toString();
		
		// If this is not the last parameter, add a comma
		if (itr != --m_outputParams.end())
			output += ", ";
	}
	
	// Add the function name and open the input parameter list
	output += "] = " + m_funcName + "(";
	
	// For each input parameter
	for (ParamVector::const_iterator itr = m_inputParams.begin(); itr != m_inputParams.end(); ++itr)
	{
		// Add this parameter to the string
		output += (*itr)->toString();
		
		// If this is not the last parameter, add a comma
		if (itr != --m_inputParams.end())
			output += ", ";
	}
	
	// Close the input parameter list
	output += ")\n";
	
	// Indent and add the body statements
	output += indentText(m_pCurrentBody->toString());
	
	// End the function
	output += "end";
	
	// Return the output
	return output;
}

/***************************************************************
* Function: ProgFunction::createTemp()
* Purpose : Create a temporary variable in this function
* Initial : Maxime Chevalier-Boisvert on March 1, 2009
****************************************************************
Revisions and bug fixes:
*/
SymbolExpr* ProgFunction::createTemp()
{
	// Assign an id number to the new variable
	size_t tempId = m_nextTempId++;
	
	// Concatenate the variable name
	std::string varName = TEMP_VAR_PREFIX + ::toString(tempId);
	
	// Return a symbol object for this variable name
	return SymbolExpr::getSymbol(varName);
}

/***************************************************************
* Function: ProgFunction::getSymbolUses()
* Purpose : Get all symbols used/read in this function
* Initial : Maxime Chevalier-Boisvert on June 29, 2009
****************************************************************
Revisions and bug fixes:
*/
Expression::SymbolSet ProgFunction::getSymbolUses() const
{
	// Declare a set for the symbols used
	Expression::SymbolSet useSet;
	
	// Get the uses for the function body
	Expression::SymbolSet bodyUses = m_pOrigBody->getSymbolUses();
	
	// Add the body uses to the set
	useSet.insert(bodyUses.begin(), bodyUses.end());

	// Add the output parameters to the set
	useSet.insert(m_outputParams.begin(), m_outputParams.end());
	
	// For each nested function
	for (FuncVector::const_iterator itr = m_nestedFuncs.begin(); itr != m_nestedFuncs.end(); ++itr)
	{
		// Get a pointer to the function
		ProgFunction* pFunc = *itr;

		// Get the uses of this function
		Expression::SymbolSet uses = pFunc->getSymbolUses();
		
		// Add the definitions to the main set
		useSet.insert(uses.begin(), uses.end());
	}	
	
	// Return the set of used symbols
	return useSet;	
}

/***************************************************************
* Function: ProgFunction::getSymbolDefs()
* Purpose : Get all symbols written/defined in this function
* Initial : Maxime Chevalier-Boisvert on June 29, 2009
****************************************************************
Revisions and bug fixes:
*/
Expression::SymbolSet ProgFunction::getSymbolDefs() const
{
	// Declare a set for the symbols defined
	Expression::SymbolSet defSet;
	
	// Get the defs for the function body
	Expression::SymbolSet bodyDefs = m_pOrigBody->getSymbolDefs();
	
	// Add the body uses to the set
	defSet.insert(bodyDefs.begin(), bodyDefs.end());
	
	// Add the input parameters to the set
	defSet.insert(m_inputParams.begin(), m_inputParams.end());
	
	// Return the set of defined symbols
	return defSet;		
}
	
