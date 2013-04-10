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
#ifndef TRANSFORM_LOOPS_H_
#define TRANSFORM_LOOPS_H_

// Header files
#include "loopstmts.h"
#include "functions.h"

// Function to simplify loops in a statement sequence
StmtSequence* transformLoops(StmtSequence* pSeq, ProgFunction* pFunction);

// Function to simplify a for loop
void transformForLoop(ForStmt* pForStmt, StmtSequence::StmtVector& stmtVector, ProgFunction* pFunction); 

// Function to simplify a while loop
void transformWhileLoop(WhileStmt* pWhileStmt, StmtSequence::StmtVector& stmtVector, ProgFunction* pFunction);

#endif // #ifndef TRANSFORM_LOOPS_H_ 
