// =========================================================================== //
//                                                                             //
// Copyright 2009 Nurudeen Abiodun Lameed and McGill University.               //
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

#ifndef ARRAY_COPY_ANALYSIS_H_
#define ARRAY_COPY_ANALYSIS_H_

#include <sstream>
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <bitset>
#include "iir.h"
#include "stmtsequence.h"
#include "ifelsestmt.h"
#include "loopstmts.h"
#include "expressions.h"
#include "symbolexpr.h"
#include "exprstmt.h"
#include "paramexpr.h"
#include "binaryopexpr.h"
#include "unaryopexpr.h"
#include "functions.h"
#include "constexprs.h"
#include "analysis_typeinfer.h"
#include "analysis_livevars.h"
#include "analysismanager.h"
#include "environment.h"

// context information for flow entries 
typedef int /*size_t*/ FlowContext;
typedef std::pair<FlowContext, FlowContext> FlowContextPair;
typedef std::map<const LoopStmt*, FlowContextPair> FlowContextMap;

// Flow entry without the context number component, i.e. context insensitive
typedef std::pair<const SymbolExpr*, const AssignStmt*> ContxInsFlowEntry;

// a flow entry structure ...
struct FlowEntry: public gc
{
	// constructor for a convenient creation of a flow entry
	FlowEntry(const SymbolExpr* symbExpr, const AssignStmt* aStmt,
			  const FlowContext fC = 0)
	:arrayVar(symbExpr), allocator(aStmt), context(fC){};
	
	// constructor for a convenient creation of a flow entry
	explicit FlowEntry(const ContxInsFlowEntry& fe, const FlowContext fC = 0)
	:arrayVar(fe.first), allocator(fe.second), context(fC){};
		
	// constructor for a convenient creation of a flow entry
	FlowEntry():arrayVar(0), allocator(0), context(0){};
		
	// for less than comparison
	bool operator<(const FlowEntry& rhs) const
	{
		std::ostringstream lStream, rStream;
			
		// serialize the objects 
		lStream << arrayVar << allocator << context;
		rStream << rhs.arrayVar << rhs.allocator << rhs.context;
			
		return lStream.str() < rStream.str();
	}
	
	// for normal equality test
	bool operator==(const FlowEntry& rhs) const 
	{
		return this->arrayVar == rhs.arrayVar 
				&& this->allocator == rhs.allocator
				&& this->context == rhs.context;
	}
	
	// for inequality comparison
	bool operator!=(const FlowEntry& rhs) const { return !(*this == rhs);}

	// array reference variable to the newly allocated array
	const SymbolExpr* arrayVar;
	
	// the allocating site, the allocator
	const AssignStmt* allocator;
	
	// the context generating the flow entry;
	FlowContext context;
};

// set of flow entry objects
typedef std::set<FlowEntry> CopyFlowSet;

// store in or gen set for each lvalue of an assignment statement
typedef std::vector<CopyFlowSet> CopyFlowSetVec;

// store flow analysis' result
struct FlowInfo
{
	FlowInfo(const CopyFlowSetVec& genVec, const CopyFlowSetVec& inVec, 
			 const CopyFlowSetVec& cpVec): genSetVec(genVec), inSetVec(inVec),
	copySetVec(cpVec){}
	
	FlowInfo(): genSetVec(CopyFlowSetVec()), inSetVec(CopyFlowSetVec()),
	copySetVec(CopyFlowSetVec()){};
	
	bool operator==(const FlowInfo& fi) const
	{
		return genSetVec == fi.genSetVec &&
				inSetVec == fi.inSetVec &&
				copySetVec == fi.copySetVec;
	}
		
	CopyFlowSetVec genSetVec;
	CopyFlowSetVec inSetVec;
	CopyFlowSetVec copySetVec;
};

// a pair for storing the in and gen sets per statement
typedef std::map<const IIRNode*, FlowInfo > CopyFlowSetMap;

// flow analysis summary info for a function
typedef std::set<size_t> IndexSet;
typedef std::vector<IndexSet> SummaryInfo;

// the complete Necessary Copy analysis' result for a function
//typedef std::pair< SummaryInfo, CopyFlowSetMap > AnalysisResult;

struct AnalysisResult : public std::pair<SummaryInfo, CopyFlowSetMap>
{
 AnalysisResult():std::pair<SummaryInfo, CopyFlowSetMap>(), 
	third(CopyFlowSetMap()), fourth(FlowContextMap()){}
	AnalysisResult(const SummaryInfo& sumInfo, const CopyFlowSetMap& analysisMap,
				   const CopyFlowSetMap& loopIterMap, const FlowContextMap& fcMap)
	  :std::pair<SummaryInfo, CopyFlowSetMap>(sumInfo, analysisMap), third(loopIterMap),
	fourth(fcMap){}
	CopyFlowSetMap third;
	FlowContextMap fourth;
};



/*******************************************************
* Class   : ArrayCopyAnalysis
* Purpose : A necessary array copy analysis
* Initial : Nurudeen A. Lameed on September 24, 2009
********************************************************
Revisions and bug fixes:
*/
class ArrayCopyAnalysis
{
public:
	
	// perform copy analysis on a program function
	static AnalysisResult doAnalysis(const ProgFunction* pFunction, 
									 const TypeSetString& inArgTypes, bool returnTop = false);
	virtual ~ArrayCopyAnalysis(){}
	
	// returns all unique definitions for an array variable that  are in a given flow set
	static void getAllDefs(const CopyFlowSet& flowSet, const  SymbolExpr* arrayVar, 
						   CopyFlowSet& defs);
	
	// checks whether a variable references a shared array
	static bool isSharedArrayVar(const CopyFlowSet& inSet, const FlowEntry& fe);
	
	// checks whether a variable references a shared array and return all the array reference 
	// variables  in the same equivalence class (i.e reference the same shared array)
	static bool isSharedArrayVar(const CopyFlowSet& inSet, const FlowEntry& def, 
								 Expression::SymbolSet& partMembers);
	static const FlowContextPair& getFlowContext(const LoopStmt* pLoopStmt);
private:
	
	// private constructor; no object should be constructed
	ArrayCopyAnalysis(){}
	
	// do analysis for a statement sequence
	static void doAnalysis(	const StmtSequence* pStmtSeq, const TypeInferInfo* pTypeInferInfo,
							const LiveVarMap& liveVarMap, const CopyFlowSet& startSet,
							CopyFlowSet& exitSet, CopyFlowSet& retSet, CopyFlowSet& breakSet,
							CopyFlowSet& contSet, CopyFlowSetMap& analysisInfo,
							CopyFlowSetMap& loopIterInfo, const Expression::SymbolSet& shadowParams,
							const Environment* const pEnv, 
							const FlowContextPair& fcPair);

	// perform necessary-copy analysis for a loop
	static void doAnalysis(const LoopStmt* pLoopStmt, const TypeInferInfo* pTypeInferInfo,
						   const LiveVarMap& liveVarMap, const CopyFlowSet& startSet,
						   CopyFlowSet& exitSet, CopyFlowSet& retSet,
						   CopyFlowSetMap& analysisInfo, CopyFlowSetMap& loopIterInfo,
						   const Expression::SymbolSet& shadowParams,
						   const Environment* const pEnv, 
						   const FlowContextPair& fcPair); //wContext flowContext = 0);
	
	// perform necessary-copy analysis on an ifelse statement
	static void doAnalysis(const IfElseStmt* pIfStmt, const TypeInferInfo* pTypeInferInfo,
						   const LiveVarMap& liveVarMap, const CopyFlowSet& startSet,
						   CopyFlowSet& exitSet, CopyFlowSet& retSet, CopyFlowSet& breakSet,
						   CopyFlowSet& contSet, CopyFlowSetMap& analysisInfo,
						   CopyFlowSetMap& loopIterInfo, const Expression::SymbolSet& shadowParams,
						   const Environment* const pEnv, 
						   const FlowContextPair& fcPair);//owContext flowContext = 0);
	
 	// flow function for analysizing a single-assignment statement
	static void analyzeSingleAssign(const TypeInferInfo* pTypeInferInfo, 
									const AssignStmt* pStmt, const Expression* lhs,
									const Expression* rhs, const LiveVarMap& liveVarMap,
									const CopyFlowSet& startSet, CopyFlowSet& exitSet,
									CopyFlowSet& genSet, CopyFlowSet& copySet,
									const Expression::SymbolSet& shadowParams,
									const Environment* const pEnv, 
									const FlowContextPair& fcPair);

	// flow function for analysing a multiple-assignments statement
	static void analyzeMultipleAssignFunc(const TypeInferInfo* pTypeInferInfo,
										  const AssignStmt* pStmt,
										  Expression::ExprVector& lvalues,
										  const ParamExpr* rhs, const LiveVarMap& liveVarMap,
										  const CopyFlowSet& startSet, CopyFlowSet& exitSet,
										  CopyFlowSetVec& inSetVec, CopyFlowSetVec& genSetVec,
										  CopyFlowSetVec& copySetVec,
										  const Expression::SymbolSet& shadowParams,
										  const Environment* const pEnv, 
										  const FlowContextPair& fcPair);
	
	// flow function for analysing a multiple-assignments statement
	static void analyzeMultipleAssignCell(const AssignStmt* pStmt, 
										  Expression::ExprVector& lvalues,
										  const CellIndexExpr* rhs, 
										  const LiveVarMap& liveVarMap,
										  const CopyFlowSet& startSet,
										  CopyFlowSet& exitSet,
										  CopyFlowSetVec& inSetVec,
										  CopyFlowSetVec& genSetVec,
										  CopyFlowSetVec& copySetVec,
										  const Expression::SymbolSet& shadowParams,
										  const Environment* const pEnv, 
										  const FlowContextPair& fcPair);
	
	// create the initial start set
	static CopyFlowSet& initializeStartSet(const ProgFunction::ParamVector& inParams, 
										   const ProgFunction::ParamVector& outParams,
										   ProgFunction* pFunction, CopyFlowSet& startSet,
										   Expression::SymbolSet& shadowParams,
										   StmtSequence::StmtVector& entryMallocSites);

	// prepare the result after the complete analysis of a funcmtion
	static void makeSummary(const CopyFlowSet& exitSet, const ProgFunction::ParamVector& outParams,
							const StmtSequence::StmtVector& paramAllocVec, SummaryInfo& sumInfo);


	// remove the definitions of an array variable from a flow set
	static void removeAllDefs(CopyFlowSet& flowSet, const SymbolExpr* arrayVar);	
	
	// insert an entry  after checking for liveness
	static void insertFlowEntry(const Expression::SymbolSet& liveVars,
								CopyFlowSet& genSet, const SymbolExpr* arrayVar,
								const AssignStmt* allocator, 
								const Expression::SymbolSet& shadowParams,
								FlowContext flowContext = 0)
	{
		if (isLive(arrayVar, liveVars, shadowParams))
		{
			genSet.insert(FlowEntry(arrayVar, allocator, flowContext));
		}
	}

	// generate new defs for the lhs from the definitions of the rhs
	static void genEntryFromRHS(const SymbolExpr* lSymbol, const SymbolExpr* rSymbol,
								const Expression::SymbolSet& liveVars, 
								const CopyFlowSet& startSet, CopyFlowSet& genSet,
								CopyFlowSet& exitSet,
								const Expression::SymbolSet& shadowParams,
								const FlowContextPair& fcPair);

	// generate a new copy generator at an array update statement	
	static void genCopyGenerator( const Expression::SymbolSet& liveVars, 
								  const Expression::SymbolSet& shadowParams,
								  const AssignStmt* pStmt, const Expression* pExpr,
								  const CopyFlowSet& startSet,
								  CopyFlowSet& exitSet, CopyFlowSet& genSet,
								  CopyFlowSet& copySet, 
								  const FlowContextPair& fcPair);

	// determine whether a function is a memory-allocation function
	static bool isAllocFunc(const std::string& funcName)
	{ 
		return std::find(allocFuncs.begin(), allocFuncs.end(), 
						 funcName) != allocFuncs.end();
	}
		
	// determine whether an array variable is live or not
	static bool isLive(const SymbolExpr* arrayVar, const Expression::SymbolSet& liveVars,
					   const Expression::SymbolSet& shadowParams)
	{
		SymbolExpr* nArrayVar = const_cast<SymbolExpr*>(arrayVar);
		return  shadowParams.find(nArrayVar) != shadowParams.end() || 
		  liveVars.find(nArrayVar) != liveVars.end();
	}

	// determine whether an expression is an array lvalue 
	static bool isArrayLValue(Expression::ExprType exprType)
	{ 
		return exprType == Expression::SYMBOL || exprType == Expression::CELL_INDEX;
	}

	// determine whether a parameterized expression has an array variable as a parameter.
	static bool hasArrayVarParam(const ParamExpr* pParamExpr);

	// load all known memory-allocation functions
	static void initAllocFuncs();
   	
	// generate conservative analysis' flow information
	static void genConservInfo(const CopyFlowSet& startSet, const ParamExpr* pParamExpr,
							   CopyFlowSet& genSet);
	
	// get the necessary copy analysis' summary info of a function
	static const Function* getSummaryInfo(const TypeInferInfo* pTypeInferInfo, 
										  const CopyFlowSet& startSet, const AssignStmt* pStmt,
										  const ParamExpr* rhs, SummaryInfo& funcSumInfo,
										  CopyFlowSet& funcGenSet, const Environment* const pEnv);
	
	// remove all variable that are not live from the exitSet
	static void rmNonLiveVars(const Expression::SymbolSet& liveVars, const CopyFlowSet& startSet, 
							  CopyFlowSet& exitSet, const Expression::SymbolSet& shadowParams);
	
	// return an array reference symbol.
	static const SymbolExpr* getSymFromArrayLValue(const Expression* lvalue, 
												   Expression::ExprType exprType);

	// generate an input set for a loop having the loop's context # 
    static void makeLoopStartSet(const CopyFlowSet& src, CopyFlowSet& dest, 
								 FlowContext newContext);

	// change the context of the flow entries with the allocator to the special context
	static bool makeContextSpecial(const AssignStmt* allocator, CopyFlowSet& exitSet,
								   FlowContext loopPrevContext);

	// generate summary info in which all return variables reference all parameters
	static void getTop(SummaryInfo& sumInfo, size_t ipSize, size_t opSize);

	// a set of memory-allocation functions
	static std::set<std::string> allocFuncs;
	
	// special context value
	static const FlowContext SPECIAL_CONTEXT = -1;

	static FlowContextMap flowContextMap;
	static FlowContext flowContextGenerator;
};

/*******************************************************
* Class   : ArrayCopyAnalysis
* Purpose : function object for locating an abject
* Initial : Nurudeen A. Lameed on September 24, 2009
********************************************************
Revisions and bug fixes:
*/
class FinderBySymbol
{
public :
	// class constructor, sExpr is a symbol (key to search for later)
	FinderBySymbol(const SymbolExpr* sExpr):arrayVarKey(sExpr){}
	
	// find a flow entry with a given array variable
	bool operator()(const FlowEntry& fe) const
	{
		return fe.arrayVar == arrayVarKey; 
	}
	
private:
	
	// the key to be located
	const SymbolExpr* arrayVarKey;
};

/*******************************************************
* Class   : FinderByStat
* Purpose : function object for locating an abject 
* Initial : Nurudeen A. Lameed on September 24, 2009
********************************************************
Revisions and bug fixes:
*/
class FinderByStmt
{
public :
	FinderByStmt(const Statement* pStmt, FlowContext flowContext):
	allocatorKey(pStmt), contextKey(flowContext){}
	
	// find a flow entry with a given allocator
	bool operator()(const FlowEntry& fe) const
	{
		return fe.allocator == allocatorKey && fe.context == contextKey; 
	}
	
private:
	
	// the allocator (key) to be located
	const Statement* allocatorKey;

	// the context number
	FlowContext contextKey;
};

// utility to get the types of the args of a parameterized expression
void getFuncArgTypes(const TypeInferInfo* pTypeInferInfo, const AssignStmt* pStmt,
					 const ParamExpr* pParamExpr, TypeSetString& inArgTypes);

// operator<< for displaying output
std::ostream&  operator<<(std::ostream& out, const FlowEntry& entry);
std::ostream&  operator<<(std::ostream& out, const CopyFlowSet& flowSet);
std::ostream&  operator<<(std::ostream& out, const CopyFlowSetMap& result);

#endif /*ARRAY_COPY_ANALYSIS_H_*/
