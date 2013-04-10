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
#ifndef STMTSEQUENCE_H_
#define STMTSEQUENCE_H_

// Header files
#include <vector>
#include "objects.h"
#include "statements.h"
#include "iir.h"

/***************************************************************
* Class   : StmtSequence
* Purpose : Represent a sequence of statements
* Initial : Maxime Chevalier-Boisvert on October 29, 2008
****************************************************************
Revisions and bug fixes:
*/
class StmtSequence : public IIRNode
{
public:
	
	// Statement vector type definition
	typedef std::vector<Statement*, gc_allocator<Statement*> > StmtVector;
	
	// Constructor
	StmtSequence(StmtVector stmts) : m_statements(stmts) { m_type = SEQUENCE; }
	StmtSequence(Statement* stmt) { m_type = SEQUENCE; m_statements.push_back(stmt); }
	StmtSequence() { m_type = SEQUENCE; }
	
	// Method to recursively copy this node
	StmtSequence* copy() const;
	
	// Method to obtain a string representation of this node
	std::string toString() const;	

	// Method to get all symbols used/read in this sequence
	Expression::SymbolSet getSymbolUses() const;
	
	// Method to get all symbols written/defined in this sequence
	Expression::SymbolSet getSymbolDefs() const;
	
	// Accessor to get the statements
	const StmtVector& getStatements() const { return m_statements; }
	
private:

	// Internal vector of statements
	StmtVector m_statements;
};

#endif // #ifndef STMTSEQUENCE_H_ 
