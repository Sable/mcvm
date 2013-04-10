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
#ifndef RETURNSTMT_H_
#define RETURNSTMT_H_

// Header files
#include "statements.h"

/***************************************************************
* Class   : ReturnStmt
* Purpose : Represent a return statement
* Initial : Maxime Chevalier-Boisvert on November 12, 2008
****************************************************************
Revisions and bug fixes:
*/
class ReturnStmt : public Statement
{
public:

	// Constructor
	ReturnStmt(bool suppressOut = true)
	{ m_stmtType = RETURN; m_suppressOut = suppressOut; }
	
	// Method to recursively copy this node
	ReturnStmt* copy() const { return new ReturnStmt(m_suppressOut); }
	
	// Method to obtain a string representation of this node
	std::string toString() const { return "return"; }
};

#endif // #ifndef RETURNSTMT_H_ 
