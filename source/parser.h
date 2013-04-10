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

// Include guards
#ifndef PARSER_H_
#define PARSER_H_

// Header files
#include <string>
#include <vector>
#include "iir.h"
#include "xml.h"
#include "functions.h"
#include "statements.h"
#include "expressions.h"
#include "stmtsequence.h"
#include "filesystem.h"


// Compilation unit list type definition
typedef std::vector<IIRNode*, gc_allocator<IIRNode*> > CompUnits;

/***************************************************************
* Class   : CodeParser
* Purpose : Parse input source code
* Initial : Maxime Chevalier-Boisvert on October 22, 2008
****************************************************************
Revisions and bug fixes:
*/
class CodeParser
{
public:

	// Method to parse a source file (Matlab code)
	static CompUnits parseSrcFile(const std::string& filePath);

	// Method to parse a source code string (Matlab code)
	static CompUnits parseSrcText(const std::string& commandString);

	// Method to parse an XML file (XML IR)
	static CompUnits parseXMLFile(const std::string& filePath);

	// Method to parse XML text (XML IR) 
	static CompUnits parseXMLText(const std::string& input);
	
private:

	// Method to parse the XML root element
	static CompUnits parseXMLRoot(const XML::Element* pTreeRoot);

	// Method to parse the XML code of a script
	static IIRNode* parseScript(const XML::Element* pElement);

	// Method to parse the XML code of a function
	static ProgFunction* parseFunction(const XML::Element* pElement);

	// Method to parse the XML code of a statement
	static Statement* parseStatement(const XML::Element* pElement);

	// Method to parse the XML code of an expression
	static Expression* parseExpression(const XML::Element* pElement);

	// Method to parse the XML code of an expression statement
	static Statement* parseExprStmt(const XML::Element* pElement);

	// Method to parse the XML code of an assignment statement
	static Statement* parseAssignStmt(const XML::Element* pElement);

	// Method to parse the XML code of an if statement
	static Statement* parseIfStmt(const XML::Element* pElement);

	// Method to parse the XML code of a switch statement
	static Statement* parseSwitchStmt(const XML::Element* pElement);

	// Method to parse the XML code of a for loop statement
	static Statement* parseForStmt(const XML::Element* pElement);

	// Method to parse the XML code of a while loop statement
	static Statement* parseWhileStmt(const XML::Element* pElement);

	// Method to parse the XML code of a return statement
	static Statement* parseReturnStmt(const XML::Element* pElement);

	// Method to parse the XML code of a statement list
	static StmtSequence* parseStmtList(const XML::Element* pElement);

	// Method to parse the XML code of a parameterized expression
	static Expression* parseParamExpr(const XML::Element* pElement);

	// Method to parse the XML code of a cell indexing expression
	static Expression* parseCellIndexExpr(const XML::Element* pElement);

	// Method to parse the XML code of a range expression
	static Expression* parseRangeExpr(const XML::Element* pElement);

	// Method to parse the XML code of a matrix expression
	static Expression* parseMatrixExpr(const XML::Element* pElement);

	// Method to parse the XML code of a cell array expression
	static Expression* parseCellArrayExpr(const XML::Element* pElement);

	// Method to parse the XML code of a function handle expression
	static Expression* parseFnHandleExpr(const XML::Element* pElement);

	// Method to parse the XML code of a lambda expression
	static Expression* parseLambdaExpr(const XML::Element* pElement);
};

#endif // #ifndef PARSER_H_
