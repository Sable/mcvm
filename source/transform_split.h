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
#ifndef TRANSFORM_SPLIT_H_
#define TRANSFORM_SPLIT_H_

// Header files
#include <stack>
#include "iir.h"
#include "functions.h"
#include "stmtsequence.h"
#include "statements.h"
#include "expressions.h"
#include "loopstmts.h"

// Function to convert a sequence of statements to split form
StmtSequence* splitSequence(StmtSequence* pSeq, ProgFunction* pFunction);

// Function to convert a statement to split form
StmtSequence::StmtVector splitStatement(Statement* pStmt, ProgFunction* pFunction);

// Function to convert a loop statement to split form
LoopStmt* splitLoopStmt(LoopStmt* pWhileStmt, ProgFunction* pFunction);

// Function to convert an expression to split form
bool splitExpression(Expression* pExpr, StmtSequence::StmtVector& stmtVector, Expression*& pTopExpr, ProgFunction* pFunction);

#endif // #ifndef TRANSFORM_SPLIT_H_
