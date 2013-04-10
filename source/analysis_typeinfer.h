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
#ifndef ANALYSIS_TYPEINFER_H_
#define ANALYSIS_TYPEINFER_H_

// Header files
#include <map>
#include <set>
#include "poolalloc.h"
#include "analysismanager.h"
#include "typeinfer.h"
#include "analysis_reachdefs.h"
#include "analysis_livevars.h"
#include "environment.h"
#include "stmtsequence.h"
#include "statements.h"
#include "expressions.h"
#include "ifelsestmt.h"
#include "loopstmts.h"
#include "assignstmt.h"
#include "paramexpr.h"
#include "cellindexexpr.h"
#include "binaryopexpr.h"
#include "unaryopexpr.h"
#include "rangeexpr.h"
#include "matrixexpr.h"
#include "cellarrayexpr.h"
#include "fnhandleexpr.h"

// Forward declarations
class ProgFunction;

// Variable type map type definition
//typedef std::map<const SymbolExpr*, TypeSet> VarTypeMap;
typedef __gnu_cxx::hash_map<const SymbolExpr*, TypeSet, IntHashFunc<const SymbolExpr*>, __gnu_cxx::equal_to<const SymbolExpr*> > VarTypeMap;
//typedef std::map<const SymbolExpr*, TypeSet, std::less<const SymbolExpr*>, PoolAlloc<std::pair<const SymbolExpr*, TypeSet> > > VarTypeMap;

// Type map vector type definition
typedef std::vector<VarTypeMap> TypeMapVector;

// Type information map type definition
//typedef std::map<const IIRNode*, VarTypeMap> TypeInfoMap;
typedef __gnu_cxx::hash_map<const IIRNode*, VarTypeMap, IntHashFunc<const IIRNode*>, __gnu_cxx::equal_to<const IIRNode*> > TypeInfoMap;
//typedef std::map<const IIRNode*, VarTypeMap, std::less<const IIRNode*>, PoolAlloc<std::pair<const IIRNode*, VarTypeMap> > > TypeInfoMap;

// Expression type map type definition
//typedef std::map<const Expression*, TypeSetString> ExprTypeMap;
typedef __gnu_cxx::hash_map<const Expression*, TypeSetString, IntHashFunc<const Expression*>, __gnu_cxx::equal_to<const Expression*> > ExprTypeMap;

/***************************************************************
* Class   : TypeInferInfo
* Purpose : Store type inference analysis information
* Initial : Maxime Chevalier-Boisvert on May 5, 2009
****************************************************************
Revisions and bug fixes:
*/
class TypeInferInfo : public AnalysisInfo
{
public:
	
	// Pre-node variable type info map
	TypeInfoMap preTypeMap;
	
	// Post-node variable type info map
	TypeInfoMap postTypeMap;
	
	// Map of possible variable types at exit points
	VarTypeMap exitTypeMap;
	
	// Possible output argument types
	TypeSetString outArgTypes;
	
	// Map of possible expression types
	ExprTypeMap exprTypeMap;
};

// Function to perform type inference on a function body
AnalysisInfo* computeTypeInfo(
	const ProgFunction* pFunction,
	const StmtSequence* pFuncBody,
	const TypeSetString& inArgTypes,
	bool returnBottom
);

// Function to perform type inference on a statement sequence
void inferTypes(
	const StmtSequence* pStmtSeq,
	const ReachDefMap& reachDefs,
	const LiveVarMap& liveVars,
	Environment* pLocalEnv,
	const VarTypeMap& startMap,
	VarTypeMap& exitMap,
	TypeMapVector& retPoints,
	TypeMapVector& breakPoints, 
	TypeMapVector& contPoints,
	TypeInfoMap& preTypeMap,
	TypeInfoMap& postTypeMap,
	ExprTypeMap& exprTypeMap
);

// Function to perform type inference for an if-else statement
void inferTypes(
	const IfElseStmt* pIfStmt,
	const ReachDefMap& reachDefs,
	const LiveVarMap& liveVars,
	Environment* pLocalEnv,
	const VarTypeMap& startMap,
	VarTypeMap& exitMap,
	TypeMapVector& retPoints,
	TypeMapVector& breakPoints, 
	TypeMapVector& contPoints,
	TypeInfoMap& preTypeMap,
	TypeInfoMap& postTypeMap,
	ExprTypeMap& exprTypeMap
);

// Function perform type inference for a loop statement
void inferTypes(
	const LoopStmt* pLoopStmt,
	const ReachDefMap& reachDefs,
	const LiveVarMap& liveVars,
	Environment* pLocalEnv,
	const VarTypeMap& startMap,
	VarTypeMap& exitMap,
	TypeMapVector& retPoints,
	TypeInfoMap& preTypeMap,
	TypeInfoMap& postTypeMap,
	ExprTypeMap& exprTypeMap
);

// Function perform type inference for an assignment statement
void inferTypes(
	const AssignStmt* pAssignStmt,
	const VarDefMap& reachDefs,
	Environment* pLocalEnv,
	VarTypeMap& varTypes,
	ExprTypeMap& exprTypeMap
);

// Function perform type inference for an expression
TypeSetString inferTypes(
	const Expression* pExpression,
	const VarDefMap& reachDefs,
	Environment* pLocalEnv,
	const VarTypeMap& varTypes,
	ExprTypeMap& exprTypeMap
);

// Function perform type inference for a parameterized expression
TypeSetString inferTypes(
	const ParamExpr* pParamExpr,
	const VarDefMap& reachDefs,
	Environment* pLocalEnv,
	const VarTypeMap& varTypes,
	ExprTypeMap& exprTypeMap
);

// Function perform type inference for a cell indexing expression
TypeSetString inferTypes(
	const CellIndexExpr* pCellExpr,
	const VarDefMap& reachDefs,
	Environment* pLocalEnv,
	const VarTypeMap& varTypes,
	ExprTypeMap& exprTypeMap
);

// Function perform type inference for a binary expression
TypeSetString inferTypes(
	const BinaryOpExpr* pBinaryExpr,
	const VarDefMap& reachDefs,
	Environment* pLocalEnv,
	const VarTypeMap& varTypes,
	ExprTypeMap& exprTypeMap
);

// Function perform type inference for a unary expression
TypeSetString inferTypes(
	const UnaryOpExpr* pUnaryExpr,
	const VarDefMap& reachDefs,
	Environment* pLocalEnv,
	const VarTypeMap& varTypes,
	ExprTypeMap& exprTypeMap
);

// Function perform type inference for a symbol expression
TypeSetString inferTypes(
	const SymbolExpr* pSymbolExpr,
	const VarDefMap& reachDefs,
	Environment* pLocalEnv,
	const VarTypeMap& varTypes,
	ExprTypeMap& exprTypeMap
);

// Function perform type inference for a range expression
TypeSetString inferTypes(
	const RangeExpr* pRangeExpr,
	const VarDefMap& reachDefs,
	Environment* pLocalEnv,
	const VarTypeMap& varTypes,
	ExprTypeMap& exprTypeMap
);

// Function perform type inference for a matrix expression
TypeSetString inferTypes(
	const MatrixExpr* pMatrixExpr,
	const VarDefMap& reachDefs,
	Environment* pLocalEnv,
	const VarTypeMap& varTypes,
	ExprTypeMap& exprTypeMap
);

// Function perform type inference for a cell array expression
TypeSetString inferTypes(
	const CellArrayExpr* pCellExpr,
	const VarDefMap& reachDefs,
	Environment* pLocalEnv,
	const VarTypeMap& varTypes,
	ExprTypeMap& exprTypeMap
);

// Function perform type inference for a function handle expression
TypeSetString inferTypes(
	const FnHandleExpr* pHandleExpr,
	const VarDefMap& reachDefs,
	Environment* pLocalEnv,
	const VarTypeMap& varTypes,
	ExprTypeMap& exprTypeMap
);

// Function to analyze matrix indexing argument types
void analyzeIndexTypes(
	const ParamExpr::ExprVector& argVector,
	const VarDefMap& reachDefs,
	Environment* pLocalEnv,
	const VarTypeMap& varTypes,
	ExprTypeMap& exprTypeMap,
	size_t& numIndexDims,
	bool& isScalarIndexing,
	bool& isMatrixIndexing
);

// Function to reduce type sets in a variable map
void varTypeMapReduce(
	VarTypeMap& varTypes
);

// Function to obtain the union of two variable type maps
VarTypeMap varTypeMapUnion(
	const VarTypeMap& mapA,
	const VarTypeMap& mapB
);

// Function to perform the union of multiple type maps
VarTypeMap typeMapVectorUnion(
	const TypeMapVector& typeMaps
);

#endif // #ifndef ANALYSIS_TYPEINFER_H_ 
