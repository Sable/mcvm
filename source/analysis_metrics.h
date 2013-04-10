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
#ifndef ANALYSIS_METRICS_H_
#define ANALYSIS_METRICS_H_

// Header files
#include <map>
#include <set>
#include "analysismanager.h"
#include "analysis_reachdefs.h"
#include "environment.h"
#include "stmtsequence.h"
#include "statements.h"
#include "expressions.h"
#include "ifelsestmt.h"
#include "loopstmts.h"

// Forward declarations
class ProgFunction;

// Function set type definition
typedef std::set<Function*> FunctionSet;

/***************************************************************
* Class   : MetricsInfo
* Purpose : Store metrics information about a function
* Initial : Maxime Chevalier-Boisvert on July 22, 2009
****************************************************************
Revisions and bug fixes:
*/
class MetricsInfo : public AnalysisInfo
{
public:
	
	// Number of statements
	size_t numStmts;
	
	// Maximum loop nesting depth
	size_t maxLoopDepth;
	
	// Callee functions
	FunctionSet callees;	
};

// Function to compute metrics info on a function body
AnalysisInfo* computeMetrics(
	const ProgFunction* pFunction,
	const StmtSequence* pFuncBody,
	const TypeSetString& inArgTypes,
	bool returnBottom
);

// Function to get the metrics info for a statement sequence
void getMetrics(
	const StmtSequence* pStmtSeq,
	const ReachDefMap& reachDefs,
	Environment* pLocalEnv,
	size_t curLoopDepth,
	size_t& numStmts,
	size_t& maxLoopDepth,
	FunctionSet& callees
);

// Function to get the metrics info for an expression
void getMetrics(
	const Expression* pExpr,
	const VarDefMap& reachDefs,
	Environment* pLocalEnv,
	FunctionSet& callees
);

#endif // #ifndef ANALYSIS_METRICS_H_
