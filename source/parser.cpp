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
#include <cassert>
#include <iostream>
#include "parser.h"
#include "process.h"
#include "utility.h"
#include "stmtsequence.h"
#include "ifelsestmt.h"
#include "switchstmt.h"
#include "loopstmts.h"
#include "returnstmt.h"
#include "assignstmt.h"
#include "exprstmt.h"
#include "paramexpr.h"
#include "unaryopexpr.h"
#include "binaryopexpr.h"
#include "symbolexpr.h"
#include "constexprs.h"
#include "rangeexpr.h"
#include "endexpr.h"
#include "matrixexpr.h"
#include "cellarrayexpr.h"
#include "fnhandleexpr.h"
#include "lambdaexpr.h"
#include "cellindexexpr.h"
#include "configmanager.h"
#include "clientsocket.h"
#include "client.h"

static unsigned maxLoopDepth = 0;

/***************************************************************
* Function: CodeParser::parseSrcFile()
* Purpose : Parse a source file (Matlab code)
* Initial : Maxime Chevalier-Boisvert on October 23, 2008
****************************************************************
Revisions and bug fixes:
*/
CompUnits CodeParser::parseSrcFile(const std::string& filePath)
{
	// If the verbose output flag is set
	if (ConfigManager::s_verboseVar.getBoolValue() == true)
	{
		// Log that we are parsing this file
		std::cout << "Parsing source file: \"" << filePath << "\"" << std::endl;
	}
	
    // Get the absolute path for the file
    std::string absPath = getAbsPath(filePath);
    
    // If the file name is invalid
    if (absPath.empty())
    {
		// Log the error
		std::cout << "ERROR: invalid file name \"" + filePath << "\"" << std::endl;
    	
    	// Abort parsing
    	return CompUnits();
    }
    	
	// Have the front-end parse the source code
	std::string xmlText = Client::parseFile(absPath);

	// Parse the XML IR
	return parseXMLText(xmlText);
}

/***************************************************************
* Function: CodeParser::parseText()
* Purpose : Parse a command String (Matlab code)
* Initial : Nurudeen A. Lameed on May 5, 2009.
****************************************************************
Revisions and bug fixes:
*/
CompUnits CodeParser::parseSrcText(const std::string& commandString)
{
	// returns an a default CompUnits, do not parse an empty command
	if (commandString.empty())
		return CompUnits();
	
	// Declare a string to store the XML output
	std::string output = Client::parseText(commandString);

	return  parseXMLText(output);
}

/***************************************************************
* Function: CodeParser::parseXMLFile()
* Purpose : Parse a an XML IR file
* Initial : Maxime Chevalier-Boisvert on November 18, 2008
****************************************************************
Revisions and bug fixes:
*/
CompUnits CodeParser::parseXMLFile(const std::string& filePath)
{
	// Log that we are parsing this file
	std::cout << "Parsing XML IR file: \"" << filePath << "\"" << std::endl;

	// Create an XML parser object
	XML::Parser parser;

	// Setup a try block to catch any errors
	try
	{
		// Parse the IR code string
		XML::Document xmlIR = parser.parseFile(filePath);

		// If the verbose output flag is set
		if (ConfigManager::s_verboseVar.getBoolValue() == true)
		{
			// Output the parsed XML
			std::cout << std::endl;
			std::cout << "Parsed XML: " << std::endl;
			std::cout << xmlIR.toString() << std::endl;
			std::cout << std::endl;
		}

		// Get a pointer to the XML tree root
		const XML::Element* pTreeRoot = xmlIR.getTree();

		// Parse the XML tree
		return parseXMLRoot(pTreeRoot);
	}

	// If XML parsing error occur
	catch (XML::ParseError error)
	{
		// Log the error
		std::cout << "ERROR: XML parsing failed " + error.toString() << std::endl;

		// Exit this function
		return CompUnits();
	}
}

/***************************************************************
* Function: CodeParser::parseXMLText()
* Purpose : Helper function for parsing source code
* Initial : Maxime Chevalier-Boisvert on October 23, 2008
****************************************************************
Revisions and bug fixes: Nurudeen A. Lameed on May 5, 2009.
*/
CompUnits CodeParser::parseXMLText(const std::string& input)
{
	// Create an XML parser object
	XML::Parser parser;

	// Setup a try block to catch any errors
	try
	{
		// Parse the IR code string
		XML::Document xmlIR = parser.parseString(input);

		// If the verbose output flag is set
		if (ConfigManager::s_verboseVar.getBoolValue() == true)
		{
			//  Output the parsed XML
			std::cout << std::endl;
			std::cout << "Parsed XML: " << std::endl;
			std::cout << xmlIR.toString() << std::endl;
			std::cout << std::endl;
		}

		// Get a pointer to the XML tree root
		const XML::Element* pTreeRoot = xmlIR.getTree();

		// Parse the XML tree
		return parseXMLRoot(pTreeRoot);
	}

	// If XML parsing error occur
	catch (XML::ParseError error)
	{
		// Log the error
		std::cout << "ERROR: XML parsing failed " + error.toString() << std::endl;

		// Exit this function
		return CompUnits();
	}
}

/***************************************************************
* Function: CodeParser::parseScript()
* Purpose : Parse the XML root element
* Initial : Maxime Chevalier-Boisvert on November 18, 2008
****************************************************************
Revisions and bug fixes:
*/
CompUnits CodeParser::parseXMLRoot(const XML::Element* pTreeRoot)
{
	// Create a list to store parsed functions
	CompUnits functionList;

	// If the root tag is not the compilation units, throw an exception
	if (pTreeRoot->getName() != "CompilationUnits")
	{
		// TODO: implement error list parsing
		std::cout << "XML tree: " << pTreeRoot->toString(true, 0) << std::endl;
		
		throw XML::ParseError("Expected compilation units: \"" + pTreeRoot->getName() + "\"", pTreeRoot->getTextPos());
	}

	// If the verbose output flag is set
	if (ConfigManager::s_verboseVar.getBoolValue() == true)
	{
		// Log the number of compilation units
		std::cout << "Number of compilation units: " << pTreeRoot->getNumChildren() << std::endl;
	}

	// For each compilation unit
	for (size_t i = 0; i < pTreeRoot->getNumChildren(); ++i)
	{
		// Get a pointer to this element
		XML::Element* pUnitElement = pTreeRoot->getChildElement(i);

		// If this is a function list
		if (pUnitElement->getName() == "FunctionList")
		{
			// Get the list of functions
			const std::vector<XML::Node*>& functions = pUnitElement->getChildren();

			// For each function
			for (std::vector<XML::Node*>::const_iterator itr = functions.begin(); itr != functions.end(); ++itr)
			{
				// If this is not an XML element, throw an exception
				if ((*itr)->getType() != XML::Node::ELEMENT)
					throw XML::ParseError("Unexpected XML node type in function list");

				// Get a pointer to this element
				XML::Element* pFuncElement = (XML::Element*)*itr;

				// If this ia a function declaration
				if (pFuncElement->getName() == "Function")
				{
					// Parse the function
					IIRNode* pNewNode = parseFunction(pFuncElement);

					// Add the function to the list
					functionList.push_back(pNewNode);
				}

				// If this is a symbol table
				else if (pFuncElement->getName() == "Symboltable")
				{
					// Ignore for now
				}

				// Otherwise
				else
				{
					// This is an invalid element, throw an exception
					throw XML::ParseError("Invalid element in function list: \"" + pFuncElement->getName() + "\"", pFuncElement->getTextPos());
				}
			}
		}

		// If this is a script
		else if (pUnitElement->getName() == "Script")
		{
			// Parse the script
			IIRNode* pNewNode = parseScript(pUnitElement);

			// Add the function to the list
			functionList.push_back(pNewNode);
		}

		// Otherwise
		else
		{
			// This is an invalid element, throw an exception
			throw XML::ParseError("Invalid element in compilation unit list: \"" + pUnitElement->getName() + "\"", pUnitElement->getTextPos());
		}
	}

	// If the verbose output flag is set
	if (ConfigManager::s_verboseVar.getBoolValue() == true)
	{
		// Output parsed IIR
		std::cout << std::endl;
		std::cout << "Constructed IIR:" << std::endl;
		for (CompUnits::iterator itr = functionList.begin(); itr != functionList.end(); ++itr)
			std::cout << ((*itr) == NULL? "NULL":(*itr)->toString()) << std::endl << std::endl;
		std::cout << std::endl;

		// Log that the parsing was successful
		std::cout << "Parsing successful" << std::endl;
	}

	// Return the parsed function list
	return functionList;
}

/***************************************************************
* Function: CodeParser::parseScript()
* Purpose : Parse the XML code of a script
* Initial : Maxime Chevalier-Boisvert on November 4, 2008
****************************************************************
Revisions and bug fixes:
*/
IIRNode* CodeParser::parseScript(const XML::Element* pElement)
{
	// Declare a pointer for the sequence statement
	StmtSequence* pSequenceStmt = NULL;

	// For each child element
	for (size_t i = 0; i < pElement->getNumChildren(); ++i)
	{
		// Get a pointer to this element
		XML::Element* pFuncElement = pElement->getChildElement(i);

		// If this is a symbol table
		if (pFuncElement->getName() == "Symboltable")
		{
			// Ignore for now
		}

		// If this is the statement list
		else if (pFuncElement->getName() == "StmtList")
		{
			// If the statement list was already encountered, throw an exception
			if (pSequenceStmt != NULL)
				throw XML::ParseError("Duplicate statement list", pElement->getTextPos());

			// Parse the statement list
			pSequenceStmt = parseStmtList(pFuncElement);
		}

		// Otherwise
		else
		{
			// This is an invalid element type, throw an exception
			throw XML::ParseError("Invalid element type in script: \"" + pFuncElement->getName() + "\"", pFuncElement->getTextPos());
		}
	}

	// If the statement list is missing, throw an exception
	if (pSequenceStmt == NULL)
		throw XML::ParseError("Missing statement list", pElement->getTextPos());

	// Create and return a new program function object
	return new ProgFunction(
		"",
		ProgFunction::ParamVector(),
		ProgFunction::ParamVector(),
		ProgFunction::FuncVector(),
		pSequenceStmt,
		true
	);
}

/***************************************************************
* Function: CodeParser::parseFunction()
* Purpose : Parse the XML code of a function
* Initial : Maxime Chevalier-Boisvert on November 3, 2008
****************************************************************
Revisions and bug fixes:
*/
ProgFunction* CodeParser::parseFunction(const XML::Element* pElement)
{
	// Get the function name
	std::string funcName = pElement->getStringAttrib("name");

	// Declare a pointer for the sequence statement
	StmtSequence* pSequenceStmt = NULL;

	// Declare vectors for the input and output parameters
	ProgFunction::ParamVector inParams;
	ProgFunction::ParamVector outParams;

	// Declare a vector for the nested function list
	ProgFunction::FuncVector nestedFuncs;

	// For each child element
	for (size_t i = 0; i < pElement->getNumChildren(); ++i)
	{
		// Get a pointer to this element
		XML::Element* pFuncElement = pElement->getChildElement(i);

		// If this is a symbol table
		if (pFuncElement->getName() == "Symboltable")
		{
			// Ignore for now
		}

		// If this is a parameter declaration list
		else if (pFuncElement->getName() == "ParamDeclList")
		{
			// Ignore for now
		}

		// If this is an input parameter list
		else if (pFuncElement->getName() == "InputParamList")
		{
			// For each child element
			for (size_t i = 0; i < pFuncElement->getNumChildren(); ++i)
			{
				// Get the parameter name
				const std::string& paramName = pFuncElement->getChildElement(i)->getStringAttrib("nameId");

				// Add the parameter to the list
				inParams.push_back(SymbolExpr::getSymbol(paramName));
			}
		}

		// If this is an output parameter list
		else if (pFuncElement->getName() == "OutputParamList")
		{
			// For each child element
			for (size_t i = 0; i < pFuncElement->getNumChildren(); ++i)
			{
				// Get the parameter name
				const std::string& paramName = pFuncElement->getChildElement(i)->getStringAttrib("nameId");

				// Add the parameter to the list
				outParams.push_back(SymbolExpr::getSymbol(paramName));
			}
		}

		// If this is a nested function list
		else if (pFuncElement->getName() == "NestedFunctionList")
		{
			// For each child element
			for (size_t i = 0; i < pFuncElement->getNumChildren(); ++i)
			{
				// Parse this function declaration
				ProgFunction* pFunction = parseFunction(pFuncElement->getChildElement(i));

				// Add the nested function to the list
				nestedFuncs.push_back(pFunction);
			}
		}

		// If this is the statement list
		else if (pFuncElement->getName() == "StmtList")
		{
			// If the statement list was already encountered, throw an exception
			if (pSequenceStmt != NULL)
				throw XML::ParseError("Duplicate statement list", pElement->getTextPos());

			// Parse the statement list
			pSequenceStmt = parseStmtList(pFuncElement);
		}

		// Otherwise
		else
		{
			// This is an invalid element type, throw an exception
			throw XML::ParseError("Invalid element type in script: \"" + pFuncElement->getName() + "\"", pFuncElement->getTextPos());
		}
	}

	// If the statement list is missing, throw an exception
	if (pSequenceStmt == NULL)
		throw XML::ParseError("Missing statement list", pElement->getTextPos());

	// Create a new program function object
	ProgFunction* pNewFunc = new ProgFunction(
		funcName,
		inParams,
		outParams,
		nestedFuncs,
		pSequenceStmt
	);

	// For each nested function
	for (ProgFunction::FuncVector::iterator nestItr = nestedFuncs.begin(); nestItr != nestedFuncs.end(); ++nestItr)
	{
		// Set the parent pointer to the newly created function object
		(*nestItr)->setParent(pNewFunc);
	}

	// Return the new program function object
	return pNewFunc;
}

/***************************************************************
* Function: CodeParser::parseStatement()
* Purpose : Parse the XML code of a statement
* Initial : Maxime Chevalier-Boisvert on November 5, 2008
****************************************************************
Revisions and bug fixes:
*/
Statement* CodeParser::parseStatement(const XML::Element* pElement)
{
	// If this is an expression statement
	if (pElement->getName() == "ExprStmt")
	{
		// Parse the expression statement
		return parseExprStmt(pElement);
	}

	// If this is an assignment statement
	else if (pElement->getName() == "AssignStmt")
	{
		// Parse the assignment statement
		return parseAssignStmt(pElement);
	}

	// If this is an if-else statement
	else if (pElement->getName() == "IfStmt")
	{
		// Parse the if-else statement
		return parseIfStmt(pElement);
	}

	// If this is a switch statement
	else if (pElement->getName() == "SwitchStmt")
	{
		// Parse the switch statement
		return parseSwitchStmt(pElement);
	}

	// If this is a for loop statement
	else if (pElement->getName() == "ForStmt")
	{
		// Parse the for loop statement
		return parseForStmt(pElement);
	}

	// If this is a while loop statement
	else if (pElement->getName() == "WhileStmt")
	{
		// Parse the while loop statement
		return parseWhileStmt(pElement);
	}

	// If this a break statement
	else if (pElement->getName() == "BreakStmt")
	{
		// Create and return a break statement object
		return new BreakStmt();
	}

	// If this a continue statement
	else if (pElement->getName() == "ContinueStmt")
	{
		// Create and return a continue statement object
		return new ContinueStmt();
	}

	// If this a return statement
	else if (pElement->getName() == "ReturnStmt")
	{
		// Parse the return statement
		return parseReturnStmt(pElement);
	}

	// Otherwise
	else
	{
		// This is an invalid statement type, throw an exception
		throw XML::ParseError("Invalid statement type: \"" + pElement->getName() + "\"", pElement->getTextPos());
	}
}

/***************************************************************
* Function: CodeParser::parseExpression()
* Purpose : Parse the XML code of an expression
* Initial : Maxime Chevalier-Boisvert on November 5, 2008
****************************************************************
Revisions and bug fixes:
*/
Expression* CodeParser::parseExpression(const XML::Element* pElement)
{
	// If this is a parameterized expression
	if (pElement->getName() == "ParameterizedExpr")
	{
		// Parse the parameterized expression
		return parseParamExpr(pElement);
	}

	// If this is a cell-indexing expression
	else if (pElement->getName() == "CellIndexExpr")
	{
		// Parse the cell indexing expression
		return parseCellIndexExpr(pElement);
	}

	// If this is a symbol name expression
	else if (pElement->getName() == "NameExpr")
	{
		// Extract the name identifier and return a new symbol expression
		return SymbolExpr::getSymbol(pElement->getChildElement(0)->getStringAttrib("nameId"));
	}

	// If this is a unary negation expression
	else if (pElement->getName() == "NotExpr")
	{
		// Parse and return the unary expression
		return new UnaryOpExpr(
			UnaryOpExpr::NOT,
			parseExpression(pElement->getChildElement(0))
		);
	}

	// If this is a unary minus expression
	else if (pElement->getName() == "UMinusExpr")
	{
		// Parse and return the unary expression
		return new UnaryOpExpr(
			UnaryOpExpr::MINUS,
			parseExpression(pElement->getChildElement(0))
		);
	}

	// If this is a unary plus expression
	else if (pElement->getName() == "UPlusExpr")
	{
		// Parse and return the unary expression
		return new UnaryOpExpr(
			UnaryOpExpr::PLUS,
			parseExpression(pElement->getChildElement(0))
		);
	}

	// If this is a binary plus/addition expression
	else if (pElement->getName() == "PlusExpr")
	{
		// Parse and return the binary expression
		return new BinaryOpExpr(
			BinaryOpExpr::PLUS,
			parseExpression(pElement->getChildElement(0)),
			parseExpression(pElement->getChildElement(1))
		);
	}

	// If this is a binary minus/subtraction expression
	else if (pElement->getName() == "MinusExpr")
	{
		// Parse and return the binary expression
		return new BinaryOpExpr(
			BinaryOpExpr::MINUS,
			parseExpression(pElement->getChildElement(0)),
			parseExpression(pElement->getChildElement(1))
		);
	}

	// If this is a binary equality comparison expression
	else if (pElement->getName() == "EQExpr")
	{
		// Parse and return the binary expression
		return new BinaryOpExpr(
			BinaryOpExpr::EQUAL,
			parseExpression(pElement->getChildElement(0)),
			parseExpression(pElement->getChildElement(1))
		);
	}

	// If this is a binary inequality comparison expression
	else if (pElement->getName() == "NEExpr")
	{
		// Parse and return the binary expression
		return new BinaryOpExpr(
			BinaryOpExpr::NOT_EQUAL,
			parseExpression(pElement->getChildElement(0)),
			parseExpression(pElement->getChildElement(1))
		);
	}

	// If this is a binary less-than (<) comparison expression
	else if (pElement->getName() == "LTExpr")
	{
		// Parse and return the binary expression
		return new BinaryOpExpr(
			BinaryOpExpr::LESS_THAN,
			parseExpression(pElement->getChildElement(0)),
			parseExpression(pElement->getChildElement(1))
		);
	}

	// If this is a binary less-than or equal (<=) comparison expression
	else if (pElement->getName() == "LEExpr")
	{
		// Parse and return the binary expression
		return new BinaryOpExpr(
			BinaryOpExpr::LESS_THAN_EQ,
			parseExpression(pElement->getChildElement(0)),
			parseExpression(pElement->getChildElement(1))
		);
	}

	// If this is a binary greater-than (>) comparison expression
	else if (pElement->getName() == "GTExpr")
	{
		// Parse and return the binary expression
		return new BinaryOpExpr(
			BinaryOpExpr::GREATER_THAN,
			parseExpression(pElement->getChildElement(0)),
			parseExpression(pElement->getChildElement(1))
		);
	}

	// If this is a binary greater-than or equal (>=) comparison expression
	else if (pElement->getName() == "GEExpr")
	{
		// Parse and return the binary expression
		return new BinaryOpExpr(
			BinaryOpExpr::GREATER_THAN_EQ,
			parseExpression(pElement->getChildElement(0)),
			parseExpression(pElement->getChildElement(1))
		);
	}

	// If this is a binary logical OR expression
	else if (pElement->getName() == "ShortCircuitOrExpr")
	{
		// Parse and return the binary expression
		return new BinaryOpExpr(
			BinaryOpExpr::OR,
			parseExpression(pElement->getChildElement(0)),
			parseExpression(pElement->getChildElement(1))
		);
	}

	// If this is a binary logical AND expression
	else if (pElement->getName() == "ShortCircuitAndExpr")
	{
		// Parse and return the binary expression
		return new BinaryOpExpr(
			BinaryOpExpr::AND,
			parseExpression(pElement->getChildElement(0)),
			parseExpression(pElement->getChildElement(1))
		);
	}

	// If this is a binary logical OR expression
	else if (pElement->getName() == "OrExpr")
	{
		// Parse and return the binary expression
		return new BinaryOpExpr(
			BinaryOpExpr::ARRAY_OR,
			parseExpression(pElement->getChildElement(0)),
			parseExpression(pElement->getChildElement(1))
		);
	}

	// If this is a binary logical AND expression
	else if (pElement->getName() == "AndExpr")
	{
		// Parse and return the binary expression
		return new BinaryOpExpr(
			BinaryOpExpr::ARRAY_AND,
			parseExpression(pElement->getChildElement(0)),
			parseExpression(pElement->getChildElement(1))
		);
	}

	// If this is a binary times/multiplication expression
	else if (pElement->getName() == "MTimesExpr")
	{
		// Parse and return the binary expression
		return new BinaryOpExpr(
			BinaryOpExpr::MULT,
			parseExpression(pElement->getChildElement(0)),
			parseExpression(pElement->getChildElement(1))
		);
	}

	// If this is a binary times/multiplication expression
	else if (pElement->getName() == "ETimesExpr")
	{
		// Parse and return the binary expression
		return new BinaryOpExpr(
			BinaryOpExpr::ARRAY_MULT,
			parseExpression(pElement->getChildElement(0)),
			parseExpression(pElement->getChildElement(1))
		);
	}

	// If this is a binary division expression
	else if (pElement->getName() == "MDivExpr")
	{
		// Parse and return the binary expression
		return new BinaryOpExpr(
			BinaryOpExpr::DIV,
			parseExpression(pElement->getChildElement(0)),
			parseExpression(pElement->getChildElement(1))
		);
	}

	// If this is a binary division expression
	else if (pElement->getName() == "EDivExpr")
	{
		// Parse and return the binary expression
		return new BinaryOpExpr(
			BinaryOpExpr::ARRAY_DIV,
			parseExpression(pElement->getChildElement(0)),
			parseExpression(pElement->getChildElement(1))
		);
	}

	// If this is a binary left division expression
	else if (pElement->getName() == "MLDivExpr")
	{
		// Parse and return the binary expression
		return new BinaryOpExpr(
			BinaryOpExpr::LEFT_DIV,
			parseExpression(pElement->getChildElement(0)),
			parseExpression(pElement->getChildElement(1))
		);
	}

	// If this is a binary power expression
	else if (pElement->getName() == "MPowExpr")
	{
		// Parse and return the binary expression
		return new BinaryOpExpr(
			BinaryOpExpr::POWER,
			parseExpression(pElement->getChildElement(0)),
			parseExpression(pElement->getChildElement(1))
		);
	}

	// If this is an array power expression
	else if (pElement->getName() == "EPowExpr")
	{
		// Parse and return the binary expression
		return new BinaryOpExpr(
			BinaryOpExpr::ARRAY_POWER,
			parseExpression(pElement->getChildElement(0)),
			parseExpression(pElement->getChildElement(1))
		);
	}

	// If this is a unary transposition expression
	else if (pElement->getName() == "MTransposeExpr")
	{
		// Parse and return the unary expression
		return new UnaryOpExpr(
			UnaryOpExpr::TRANSP,
			parseExpression(pElement->getChildElement(0))
		);
	}

	// If this is a array transposition expression
	else if (pElement->getName() == "ArrayTransposeExpr")
	{
		// Parse and return the unary expression
		return new UnaryOpExpr(
			UnaryOpExpr::ARRAY_TRANSP,
			parseExpression(pElement->getChildElement(0))
		);
	}

	// If this is a colon expression
	else if (pElement->getName() == "ColonExpr")
	{
		// Parse the range expression
		return new RangeExpr(NULL, NULL, NULL);
	}

	// If this is a range expression
	else if (pElement->getName() == "RangeExpr")
	{
		// Parse the range expression
		return parseRangeExpr(pElement);
	}

	// If this is a range end expression
	else if (pElement->getName() == "EndExpr")
	{
		// Create and return an end expression object
		return new EndExpr();
	}

	// If this is a matrix expression
	else if (pElement->getName() == "MatrixExpr")
	{
		// Parse the matrix expression
		return parseMatrixExpr(pElement);
	}

	// If this is a cell array expression
	else if (pElement->getName() == "CellArrayExpr")
	{
		// Parse the cell array expression
		return parseCellArrayExpr(pElement);
	}

	// If this is a function handle expression
	else if (pElement->getName() == "FunctionHandleExpr")
	{
		// Parse the function handle expression
		return parseFnHandleExpr(pElement);
	}

	// If this is a lambda expression
	else if (pElement->getName() == "LambdaExpr")
	{
		// Parse the lambda expression
		return parseLambdaExpr(pElement);
	}

	// If this is an integer constant expression
	else if (pElement->getName() == "IntLiteralExpr")
	{
		// Parse and return the integer constant expression
		return new IntConstExpr(
			pElement->getIntAttrib("value")
		);
	}

	// If this is a floating-point literal expression
	else if (pElement->getName() == "FPLiteralExpr")
	{
		// Parse and return the floating-point constant expression
		return new FPConstExpr(
			pElement->getFloatAttrib("value")
		);
	}

	// If this is a string literal expression
	else if (pElement->getName() == "StringLiteralExpr")
	{
		// Parse and return the string constant expression
		return new StrConstExpr(
			pElement->getStringAttrib("value")
		);
	}

	// Otherwise
	else
	{
		// This is an invalid statement type, throw an exception
		throw XML::ParseError("Unsupported expression type: \"" + pElement->getName() + "\"", pElement->getTextPos());
	}
}

/***************************************************************
* Function: CodeParser::parseExprStmt()
* Purpose : Parse the XML code of an assignment statement
* Initial : Maxime Chevalier-Boisvert on February 19, 2009
****************************************************************
Revisions and bug fixes:
*/
Statement* CodeParser::parseExprStmt(const XML::Element* pElement)
{
	// Parse the underlying expression
	Expression* pExpr = parseExpression(pElement->getChildElement(0));

	// Get the output suppression flag
	bool suppressOut = pElement->getBoolAttrib("outputSuppressed");

	// Create and return the expression statement object
	return new ExprStmt(pExpr, suppressOut);
}

/***************************************************************
* Function: CodeParser::parseAssignStmt()
* Purpose : Parse the XML code of an assignment statement
* Initial : Maxime Chevalier-Boisvert on November 5, 2008
****************************************************************
Revisions and bug fixes:
*/
Statement* CodeParser::parseAssignStmt(const XML::Element* pElement)
{
	// Get the element corresponding to the left-expression
	XML::Element* pLeftElem = pElement->getChildElement(0);

	// Create a vector to store the left expressions
	AssignStmt::ExprVector leftExprs;

	// If the left expression is a matrix expression
	if (pLeftElem->getName() == "MatrixExpr")
	{
		// If the number of rows is not 1, throw an exception
		if (pLeftElem->getNumChildren() != 1)
			throw XML::ParseError("invalid matrix expression on assignment lhs");

		// Get the row element
		XML::Element* pRowElem = pLeftElem->getChildElement(0);

		// For each left expression element
		for (size_t i = 0; i < pRowElem->getNumChildren(); ++i)
		{
			// Get the child element for this expression
			XML::Element* pExprElem = pRowElem->getChildElement(i);

			// Parse this left-expression
			leftExprs.push_back(parseExpression(pExprElem));
		}
	}
	else
	{
		// Parse the left expression directly
		leftExprs.push_back(parseExpression(pLeftElem));
	}

	// Parse the right expression
	Expression* pRightExpr = parseExpression(pElement->getChildElement(1));

	// Get the output suppression flag
	bool suppressOut = pElement->getBoolAttrib("outputSuppressed");

	// Create and return the new assignment statement object
	return new AssignStmt(leftExprs, pRightExpr, suppressOut);
}

/***************************************************************
* Function: CodeParser::parseIfStmt()
* Purpose : Parse the XML code of an if statement
* Initial : Maxime Chevalier-Boisvert on November 5, 2008
****************************************************************
Revisions and bug fixes:
*/
Statement* CodeParser::parseIfStmt(const XML::Element* pElement)
{
	// Declare a vector to store the if blocks
	std::vector<XML::Element*> ifBlocks;

	// Declare a pointer for the else block
	XML::Element* pElseBlock = NULL;

	// For each child element
	for (size_t i = 0; i < pElement->getNumChildren(); ++i)
	{
		// Get a pointer to this element
		XML::Element* pIfElement = pElement->getChildElement(i);

		// If this is an if block
		if (pIfElement->getName() == "IfBlock")
		{
			// Add it to the list
			ifBlocks.push_back(pIfElement);
		}

		// If this is an else block
		else if (pIfElement->getName() == "ElseBlock")
		{
			// If there is more than one else block, throw an exception
			if (pElseBlock)
				throw XML::ParseError("Duplicate else block", pIfElement->getTextPos());

			// Store a pointer to the else block
			pElseBlock = pIfElement;
		}

		// Otherwise
		else
		{
			// This is an invalid element type, throw an exception
			throw XML::ParseError("Invalid element in if statement: \"" + pIfElement->getName() + "\"", pIfElement->getTextPos());
		}
	}

	// If are are no if blocks, throw an exception
	if (ifBlocks.size() == 0)
		throw XML::ParseError("Missing if block", pElement->getTextPos());

	// Get a pointer to the last if block
	XML::Element* pLastIfBlock = ifBlocks.back();

	// remove the last if block from the list
	ifBlocks.pop_back();

	// Create the first if-else statement from the last if and else blocks
	IfElseStmt* pIfStmt = new IfElseStmt(
			parseExpression(pLastIfBlock->getChildElement(0)),
			parseStmtList(pLastIfBlock->getChildElement(1)),
			pElseBlock? parseStmtList(pElseBlock->getChildElement(0)):(new StmtSequence())
	);

	// For each if block, in reverse order
	for (std::vector<XML::Element*>::reverse_iterator itr = ifBlocks.rbegin(); itr != ifBlocks.rend(); ++itr)
	{
		// Get a pointer to this if block element
		XML::Element* pIfBlock = *itr;

		// Add this if block to the recursive structure
		pIfStmt = new IfElseStmt(
			parseExpression(pIfBlock->getChildElement(0)),
			parseStmtList(pIfBlock->getChildElement(1)),
			new StmtSequence(pIfStmt)
		);
	}

	// Return a pointer to the if-else statement
	return pIfStmt;
}

/***************************************************************
* Function: CodeParser::parseSwitchStmt()
* Purpose : Parse the XML code of a switch statement
* Initial : Maxime Chevalier-Boisvert on February 23, 2009
****************************************************************
Revisions and bug fixes:
*/
Statement* CodeParser::parseSwitchStmt(const XML::Element* pElement)
{
	// Parse the switch expression
	Expression* pSwitchExpr = parseExpression(pElement->getChildElement(0));

	// Declare a case list for the switch cases
	SwitchStmt::CaseList caseList;

	// Declare a statement sequence pointer for the default case
	StmtSequence* pDefaultCase = NULL;

	// For each remaining child element
	for (size_t i = 1; i < pElement->getNumChildren(); ++i)
	{
		// Get a pointer to this child element
		XML::Element* pChildElem = pElement->getChildElement(i);

		// If this is a switch case block
		if (pChildElem->getName() == "SwitchCaseBlock")
		{
			// Parse the switch case contents and add the case to the list
			caseList.push_back(
				SwitchStmt::SwitchCase(
					parseExpression(pChildElem->getChildElement(0)),
					parseStmtList(pChildElem->getChildElement(1))
				)
			);
		}

		// If this is a default case block
		else if (pChildElem->getName() == "DefaultCaseBlock")
		{
			// If a default case was already parsed
			if (pDefaultCase != NULL)
			{
				// Throw an exception
				throw XML::ParseError("Duplicate default case in switch statement", pChildElem->getTextPos());
			}

			// Parse the statement list
			pDefaultCase = parseStmtList(pChildElem->getChildElement(0));
		}

		// Otherwise
		else
		{
			// This is an invalid element type, throw an exception
			throw XML::ParseError("Invalid element in switch statement: \"" + pChildElem->getName() + "\"", pChildElem->getTextPos());
		}
	}

	// If there is no default case, create an empty one
	if (pDefaultCase == NULL)
		pDefaultCase = new StmtSequence();

	// Create and return the switch statement object
	return new SwitchStmt(
		pSwitchExpr,
		caseList,
		pDefaultCase
	);
}

/***************************************************************
* Function: CodeParser::parseForStmt()
* Purpose : Parse the XML code of a for loop statement
* Initial : Maxime Chevalier-Boisvert on November 8, 2008
****************************************************************
Revisions and bug fixes:
*/
Statement* CodeParser::parseForStmt(const XML::Element* pElement)
{
  unsigned loopDepth = ++maxLoopDepth;

	// Parse the assignment statement
	Statement* pAssignStmt = parseStatement(pElement->getChildElement(0));

	// Ensure that this statement really is an assignment statement
	if (pAssignStmt->getStmtType() != Statement::ASSIGN)
		throw XML::ParseError("Invalid statement type", pElement->getChildElement(0)->getTextPos());

	// Parse the loop body
	StmtSequence* pLoopBody = parseStmtList(pElement->getChildElement(1));

    unsigned annotations = 0;
    // reset maxLoopDepth
    if (loopDepth == maxLoopDepth) {
      // isInnermost loop is true;
      annotations |= Statement::INNERMOST;
    }
    if (loopDepth == 1) {
      // isOutermost loop is true;
      annotations |= Statement::OUTERMOST;
      // reset the counters
      maxLoopDepth = 0;
    }

	// return the new for statement object
    return new ForStmt((AssignStmt*)pAssignStmt, pLoopBody, annotations);
}

/***************************************************************
* Function: CodeParser::parseWhileStmt()
* Purpose : Parse the XML code of a while loop statement
* Initial : Maxime Chevalier-Boisvert on November 8, 2008
****************************************************************
Revisions and bug fixes:
*/
Statement* CodeParser::parseWhileStmt(const XML::Element* pElement)
{

  unsigned loopDepth = ++maxLoopDepth;

  // Parse the condition expression
  Expression* pCondExpr = parseExpression(pElement->getChildElement(0));

  // Parse the loop body
  StmtSequence* pLoopBody = parseStmtList(pElement->getChildElement(1));

  unsigned annotations = 0;
  // reset maxLoopDepth && currentDepth
  if (loopDepth == maxLoopDepth) {
    // isInnermost loop is true;
    annotations |= Statement::INNERMOST;
  }
  if (loopDepth == 1) {
    // isOutermost loop is true;
    annotations |= Statement::OUTERMOST;
    // reset the loop depth counter
    maxLoopDepth = 0;
  }

  // return the new for statement object
  return new WhileStmt(pCondExpr, pLoopBody, annotations);
}

/***************************************************************
* Function: CodeParser::parseReturnStmt()
* Purpose : Parse the XML code of a return statement
* Initial : Maxime Chevalier-Boisvert on February 19, 2009
****************************************************************
Revisions and bug fixes:
*/
Statement* CodeParser::parseReturnStmt(const XML::Element* pElement)
{
	// Get the output suppression flag
	bool suppressOut = pElement->getBoolAttrib("outputSuppressed");

	// Create and return the return statement object
	return new ReturnStmt(suppressOut);
}

/***************************************************************
* Function: CodeParser::parseStmtList()
* Purpose : Parse the XML code of a statement list
* Initial : Maxime Chevalier-Boisvert on November 7, 2008
****************************************************************
Revisions and bug fixes:
*/
StmtSequence* CodeParser::parseStmtList(const XML::Element* pElement)
{
	// Create a statement vector to store the statements
	StmtSequence::StmtVector stmtVector;

	// For each child element
	for (size_t i = 0; i < pElement->getNumChildren(); ++i)
	{
		// Get this child element
		XML::Element* pChildElement = pElement->getChildElement(i);

		// If this is a variable declaration
		if (pChildElement->getName() == "VariableDecl")
		{
			// Ignore for now
			continue;
		}

		// Parse this statement
		Statement* pStmt = parseStatement(pChildElement);

		// Add the statement to the vector
		stmtVector.push_back(pStmt);
	}

	// Return a new sequence statement
	return new StmtSequence(stmtVector);
}

/***************************************************************
* Function: CodeParser::parseParamExpr()
* Purpose : Parse the XML code of a parameterized expression
* Initial : Maxime Chevalier-Boisvert on November 7, 2008
****************************************************************
Revisions and bug fixes:
*/
Expression* CodeParser::parseParamExpr(const XML::Element* pElement)
{
	// Parse the symbol expression
	Expression* pSymExpr = parseExpression(pElement->getChildElement(0));

	// If the symbol expression is not a symbol, throw an exception
	if (pSymExpr->getExprType() != Expression::SYMBOL)
		throw XML::ParseError("Expected symbol expression", pElement->getTextPos());

	// Compute the number of arguments
	size_t numArgs = pElement->getNumChildren() - 1;

	// Declare a vector for the arguments
	ParamExpr::ExprVector arguments;

	// For each child element
	for (size_t i = 0; i < numArgs; ++i)
	{
		// Parse this argument
		arguments.push_back(parseExpression(pElement->getChildElement(i + 1)));
	}

	// Create and return the new parameterized expression
	return new ParamExpr(
		(SymbolExpr*)pSymExpr,
		arguments
	);
}

/***************************************************************
* Function: CodeParser::parseCellIndexExpr()
* Purpose : Parse the XML code of a cell indexing expression
* Initial : Maxime Chevalier-Boisvert on February 15, 2009
****************************************************************
Revisions and bug fixes:
*/
Expression* CodeParser::parseCellIndexExpr(const XML::Element* pElement)
{
	// Parse the symbol expression
	Expression* pSymExpr = parseExpression(pElement->getChildElement(0));

	// If the symbol expression is not a symbol, throw an exception
	if (pSymExpr->getExprType() != Expression::SYMBOL)
		throw XML::ParseError("Expected symbol expression", pElement->getTextPos());

	// Compute the number of arguments
	size_t numArgs = pElement->getNumChildren() - 1;

	// Declare a vector for the arguments
	ParamExpr::ExprVector arguments;

	// For each child element
	for (size_t i = 0; i < numArgs; ++i)
	{
		// Parse this argument
		arguments.push_back(parseExpression(pElement->getChildElement(i + 1)));
	}

	// Create and return the new cell indexing expression
	return new CellIndexExpr(
		(SymbolExpr*)pSymExpr,
		arguments
	);
}

/***************************************************************
* Function: CodeParser::parseRangeExpr()
* Purpose : Parse the XML code of a range expression
* Initial : Maxime Chevalier-Boisvert on November 8, 2008
****************************************************************
Revisions and bug fixes:
*/
Expression* CodeParser::parseRangeExpr(const XML::Element* pElement)
{
	// Get the number of values specified in the range
	size_t numValues = pElement->getNumChildren();

	// Declare a vector for the values
	std::vector<Expression*> values;

	// For each child element
	for (size_t i = 0; i < numValues; ++i)
	{
		// Parse this expression
		values.push_back(parseExpression(pElement->getChildElement(i)));
	}

	// If 2 values were specified
	if (values.size() == 2)
	{
		// Create and return the new range expression with a step size of 1
		return new RangeExpr(values[0], values[1], new IntConstExpr(1));
	}

	// Otherwise, if 3 values were specified
	if (values.size() == 3)
	{
		// Create and return the new range expression and specify the step size
		return new RangeExpr(values[0], values[2], values[1]);
	}

	// Otherwise
	else
	{
		// Throw an exception
		throw XML::ParseError("Invalid number of values specified in range", pElement->getTextPos());
	}
}

/***************************************************************
* Function: CodeParser::parseMatrixExpr()
* Purpose : Parse the XML code of a matrix expression
* Initial : Maxime Chevalier-Boisvert on November 8, 2008
****************************************************************
Revisions and bug fixes:
*/
Expression* CodeParser::parseMatrixExpr(const XML::Element* pElement)
{
	// Declare a vector to store the rows
	MatrixExpr::RowVector rowVector;

	// For each child element
	for (size_t i = 0; i < pElement->getNumChildren(); ++i)
	{
		// Declare a row to store elements
		MatrixExpr::Row row;

		// Get a reference to this child element
		XML::Element* pRowElem = pElement->getChildElement(i);

		// If this is not a row element, throw an exception
		if (pRowElem->getName() != "Row")
			throw XML::ParseError("Invalid element found in matrix expression", pRowElem->getTextPos());\

		// For each expression in this row
		for (size_t j = 0; j < pRowElem->getNumChildren(); ++j)
		{
			// Parse the expression and add it to the row
			row.push_back(parseExpression(pRowElem->getChildElement(j)));
		}

		// Add the row to the vector
		rowVector.push_back(row);
	}

	// Create and return the matrix expression
	return new MatrixExpr(rowVector);
}

/***************************************************************
* Function: CodeParser::parseCellArrayExpr()
* Purpose : Parse the XML code of a cell array expression
* Initial : Maxime Chevalier-Boisvert on February 23, 2009
****************************************************************
Revisions and bug fixes:
*/
Expression* CodeParser::parseCellArrayExpr(const XML::Element* pElement)
{
	// Declare a vector to store the rows
	CellArrayExpr::RowVector rowVector;

	// For each child element
	for (size_t i = 0; i < pElement->getNumChildren(); ++i)
	{
		// Declare a row to store elements
		CellArrayExpr::Row row;

		// Get a reference to this child element
		XML::Element* pRowElem = pElement->getChildElement(i);

		// If this is not a row element, throw an exception
		if (pRowElem->getName() != "Row")
			throw XML::ParseError("Invalid element found in cell array expression", pRowElem->getTextPos());

		// For each expression in this row
		for (size_t j = 0; j < pRowElem->getNumChildren(); ++j)
		{
			// Parse the expression and add it to the row
			row.push_back(parseExpression(pRowElem->getChildElement(j)));
		}

		// Add the row to the vector
		rowVector.push_back(row);
	}

	// Create and return the cell array expression
	return new CellArrayExpr(rowVector);
}

/***************************************************************
* Function: CodeParser::parseFnHandleExpr()
* Purpose : Parse the XML code of a function handle expression
* Initial : Maxime Chevalier-Boisvert on February 25, 2009
****************************************************************
Revisions and bug fixes:
*/
Expression* CodeParser::parseFnHandleExpr(const XML::Element* pElement)
{
	// Get the symbol name
	std::string symName = pElement->getChildElement(0)->getStringAttrib("nameId");

	// Create and return the new function handle expression
	return new FnHandleExpr(SymbolExpr::getSymbol(symName));
}

/***************************************************************
* Function: CodeParser::parseLambdaExpr()
* Purpose : Parse the XML code of a lambda expression
* Initial : Maxime Chevalier-Boisvert on February 25, 2009
****************************************************************
Revisions and bug fixes:
*/
Expression* CodeParser::parseLambdaExpr(const XML::Element* pElement)
{
	// Create a vector for the input parameters
	LambdaExpr::ParamVector inParams;

	// Declare a pointer for the body expression
	Expression* pBodyExpr = NULL;

	// For each child element
	for (size_t i = 0; i < pElement->getNumChildren(); ++i)
	{
		// Get a pointer to this child element
		XML::Element* pChildElem = pElement->getChildElement(i);

		// If this is a symbol
		if (pChildElem->getName() == "Name")
		{
			// Parse the symbol and add it to the input parameters
			inParams.push_back(SymbolExpr::getSymbol(pChildElem->getStringAttrib("nameId")));
		}

		// Otherwise, it must be the body expression
		else
		{
			// If the body expression was already parsed
			if (pBodyExpr != NULL)
			{
				// Throw an exception
				throw XML::ParseError("Duplicate body expression", pChildElem->getTextPos());
			}

			// Parse the body expression
			pBodyExpr = parseExpression(pChildElem);
		}
	}

	// If no body expression was found
	if (pBodyExpr == NULL)
	{
		// Throw an exception
		throw XML::ParseError("No body expression found", pElement->getTextPos());
	}

	// Create and return the new lambda expression
	return new LambdaExpr(
		inParams,
		pBodyExpr
	);
}
