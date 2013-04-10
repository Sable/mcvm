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
#ifndef TRANSFORM_ENDEXPR_H_
#define TRANSFORM_ENDEXPR_H_

// Header files
#include "stmtsequence.h"
#include "functions.h"
#include "endexpr.h"

// Function to pre-process range end expressions in sequences
StmtSequence* processEndExpr(StmtSequence* pSeq, ProgFunction* pFunction);

// Function to pre-process range end sub-expressions in expressions
Expression* processEndExpr(Expression* pExpr, ProgFunction* pFunction, const EndExpr::AssocVector& assocs = EndExpr::AssocVector());

#endif // #ifndef TRANSFORM_ENDEXPR_H_ 
