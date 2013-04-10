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
#ifndef TRANSFORM_SWITCH_H_
#define TRANSFORM_SWITCH_H_

// Header files
#include "functions.h"
#include "stmtsequence.h"
#include "switchstmt.h"
#include "ifelsestmt.h"

// Function to eliminate switch statements in a statement sequence
StmtSequence* transformSwitch(StmtSequence* pSeq, ProgFunction* pFunction);

// Function to transform a switch statement into if-else and loop statements
StmtSequence* transformSwitch(SwitchStmt* pSwitchStmt, ProgFunction* pFunction);

#endif // #ifndef TRANSFORM_SWITCH_H_ 
