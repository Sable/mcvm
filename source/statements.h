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
#ifndef STATEMENTS_H_
#define STATEMENTS_H_

// Header files
#include "iir.h"
#include "expressions.h"

/***************************************************************
* Class   : Statement
* Purpose : Base class for statements
* Initial : Maxime Chevalier-Boisvert on October 27, 2008
****************************************************************
Revisions and bug fixes:
*/
class Statement : public IIRNode
{
 public:

  // Define different annotation types; currently
  // this is used to identify loop depth in a loop 
  // nest; see loopstmts for more information.
  enum Annotations { 
    // no annotations is set
    NONE = 0,
    
    // if set, the statement is in a loop
    IN_LOOP = 1, 

    // if set, it is in the outermost loop of a loop nest
    OUTERMOST = 1 << 1,

    // is in the innermost loop of a loop nest
    INNERMOST = 1 << 2
  };

  // Enumerate statement types
  enum StmtType
  {
    IF_ELSE,
    SWITCH,
    FOR,
    WHILE,
    LOOP,
    COMPOUND_END, // used as a sentinel 
    BREAK,
    CONTINUE,
    RETURN,
    ASSIGN,
    EXPR
  };
	
  // Constructor and destructor
 Statement() : m_suppressOut(true), m_annotations(NONE)  { m_type = STATEMENT; }
  virtual ~Statement() {}
	
  // Method to recursively copy this node
  virtual Statement* copy() const = 0;
	
  // Method to get all symbols read/used by this statement
  virtual Expression::SymbolSet getSymbolUses() const { return Expression::SymbolSet(); } 
	
  // Method to get all symbols written/defined by this statement
  virtual Expression::SymbolSet getSymbolDefs() const { return Expression::SymbolSet(); }
	
  // Method to set the output suppression flag
  void setSuppressFlag(bool val) { m_suppressOut = val; }
	
  // Accessor to get the statement type
  StmtType getStmtType() const { return m_stmtType; }
	
  // Accessor to get the output suppression flag
  bool getSuppressFlag() const { return m_suppressOut; }

  bool isStmtInLoop() const { return (m_annotations & IN_LOOP) != 0; }

  void addAnnotation(Annotations annotation) {
    m_annotations |= annotation;
  }

  unsigned getAnnotations() { return m_annotations; }
	
 protected:
	
  // Statement type
  StmtType m_stmtType;
	
  // Output suppression flag
  bool m_suppressOut;

  unsigned  m_annotations;
};

#endif // #ifndef STATEMENTS_H_
