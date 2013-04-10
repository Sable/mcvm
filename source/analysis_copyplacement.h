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

#ifndef ACOPYPLACEMENT_H_
#define ACOPYPLACEMENT_H_

#include <gc_cpp.h>
#include <gc/gc_allocator.h>
#include "analysis_arraycopy.h"

/*******************************************************
 * Class   : ArrayCopyEntry
 * Purpose : A structure that represent a copy statement
 * Initial : Nurudeen A. Lameed on December 18, 2009
 ********************************************************
 Revisions and bug fixes:
 */
struct CPEntry : public gc 
{
 CPEntry(const Statement* stmt, const ContxInsFlowEntry& fe, size_t index = 0)
	  :pStmt(stmt), stmtIndex(index), flowEntry(fe), flowContext(0) {}
			
	CPEntry():pStmt(0), stmtIndex(0), flowEntry(ContxInsFlowEntry(0, 0)), flowContext(0) {}

   CPEntry(const Statement* stmt, const ContxInsFlowEntry& fe, size_t index, FlowContext fc)
   :pStmt(stmt), stmtIndex(index), flowEntry(fe), flowContext(fc) {}

	bool operator<(const CPEntry& rhs) const
	{
		std::ostringstream lStream, rStream;
		lStream << pStmt << stmtIndex << flowEntry.second << flowEntry.first;
		rStream << rhs.pStmt << rhs.stmtIndex
		<< rhs.flowEntry.second << rhs.flowEntry.first;

		return lStream.str() < rStream.str();
	}

	// for normal equality test
    bool operator==(const CPEntry& rhs) const
    {
        return this->pStmt == rhs.pStmt
                && this->stmtIndex == rhs.stmtIndex
		&& this->flowEntry == rhs.flowEntry;
    }

    // for inequality comparison
    bool operator!=(const CPEntry& rhs) const { return !(*this == rhs);}


	// the statement where this copy should be performed
	const Statement* pStmt;

	// index, in a multiple assignment statement
	size_t stmtIndex;

	// the flow entry of the symbol to be copied
	ContxInsFlowEntry flowEntry;

	// context of the statement where this copy should be performed.
	FlowContext flowContext;
};

// Copy placement flow set
typedef std::set<CPEntry> CPFlowSet;

/************output*****************************************/

// symbols that should be at an assign stmt or a block
//(symbol, symbols to check against)
typedef std::pair<const SymbolExpr*, Expression::SymbolSet> CopyInfo;
typedef std::vector<CopyInfo> StmtCopyVec;
typedef std::vector<StmtCopyVec> BlockCopyVecs;
typedef std::map<const Statement*, BlockCopyVecs> CPMap;

/***************************************************************
 * Class   : Array copy placement analysis info
 * Purpose : Store array copy analysis information
 * Initial : Nurudeen A. Lameed on December 18, 2009
 ****************************************************************
 Revisions and bug fixes:
 */
class ArrayCopyAnalysisInfo : public AnalysisInfo 
{
public:
	ArrayCopyAnalysisInfo(const SummaryInfo& sumInfo,const CPMap& cpMap,
						  const Expression::SymbolSet& paramCopies)
	:summaryInfo(sumInfo), copyMap(cpMap), paramsToCopy(paramCopies){}
	
	// flow analysis' summary info 
	const SummaryInfo summaryInfo;
	
	// array copy placement analysis result(copy)
	const CPMap copyMap;
	
	// parameters to be copied at the entry point
	const Expression::SymbolSet paramsToCopy;
};

/******************************************************************
 * Class   : ArrayCopyPlacement
 * Purpose : perform copy placement using array copy analysis result
 * Initial : Nurudeen A. Lameed on December 18, 2009
 *******************************************************************
 Revisions and bug fixes:
 */
class ArrayCopyPlacement 
{
public:
	
	// perform copy placement analysis on a program function
	static ArrayCopyAnalysisInfo* doCopyPlacement(const ProgFunction* pFunction,
												  const AnalysisResult& analysisResult);
	// destructor
	virtual ~ArrayCopyPlacement() {}
private:
	// private constructor, should not be called.
	ArrayCopyPlacement() {}

	// perform a backward placement for a statement sequence
	static void doCopyPlacement(const StmtSequence* pStmtSeq,
								const CopyFlowSetMap& analysisInfo,
								CPFlowSet& startSet,CPFlowSet& exitSet,
								const CPFlowSet* pBreakSet, const CPFlowSet* pContSet,
								const CPFlowSet& retSet, CPMap& cpMap,
								const CopyFlowSetMap& loopIterInfo,
								const FlowContextPair& fcPair);

	// perform a backward placement for a loop
	static void doCopyPlacement(const LoopStmt* pLoopStmt,
								const CopyFlowSetMap& analysisInfo,
								CPFlowSet& startSet, CPFlowSet& exitSet,
								const CPFlowSet& retSet, CPMap& cpMap,
								const CopyFlowSetMap& loopIterInfo,
								const FlowContextPair& fcPair);

	// perform a backward placement for an if statement
	static void doCopyPlacement(const IfElseStmt* pIfElseStmt, 
								const CopyFlowSetMap& analysisInfo,
								CPFlowSet& startSet, CPFlowSet& exitSet,
								const CPFlowSet* pBreakSet, const CPFlowSet* pContSet,
								const CPFlowSet& retSet, CPMap& cpMap,
								const CopyFlowSetMap& loopIterInfo,
								const FlowContextPair& fcPair);
	
	// perform a backward placement for an assignment statement
	static void flowFunction(const AssignStmt* pAssignStmt,
							 const IIRNode* block, const FlowInfo& faInfo, 
							 CPFlowSet& startSet, CPFlowSet& exitSet,
							 CPMap& cpInfo, const CopyFlowSetVec& loopFirstInSets,
							 const FlowContextPair& fcPair);

	// perform a backward placement for an assignment statement
	static void flowFunction(const AssignStmt* pAssignStmt,
							 const IIRNode* block, const FlowInfo& faInfo, 
							 CPFlowSet& startSet, CPFlowSet& exitSet,CPMap& cpInfo,
							 const FlowContextPair& fcPair);

	// merge result of an if statement with the sequence
	static CPFlowSet merge(CPMap& cpInfo, const LoopStmt* ploopStmt, 
						   CPFlowSet& mainSet, CPFlowSet& loopSet, 
						   FlowContext flowContext);
	
	// merge result of an if statement with the sequence
	static CPFlowSet merge(CPMap& cpInfo, const IfElseStmt* pIfStmt, 
						   CPFlowSet& mainSet, CPFlowSet& ifStartSet,
						   CPFlowSet& elseStartSet,
						   FlowContext flowContext);
	
	// find common entries in two copy flow set
	static CPFlowSet& findIntersection(const Statement* pStmt,
									   CPFlowSet& intersec,
									   CPFlowSet& mainSet,
									   CPFlowSet& blockSet,
									   FlowContext flowContext);
	
	// remove a common entry from a copy flow set
	static CPFlowSet& rmIntersection(CPMap& cpInfo, CPFlowSet& blockSet, 
									 const ContxInsFlowEntry& fe);
	
	// locate a copy entry for a given statement
	static bool findCopy(CPMap& cpInfo, const CPEntry& cpe, CopyInfo*& result);
	
	// add a copy symbol and checks to the current result
	static void addCopy(CPMap& cpInfo, const CPEntry& cpe, const Expression::SymbolSet& checks, 
						bool isLoopHeader = false);
	
	// add a copy with no checks to the result
	static void addCopy(CPMap& cpInfo, const CPEntry& cpe, bool isLoopHeader = false)
	{
		addCopy(cpInfo, cpe, Expression::SymbolSet(), isLoopHeader);
	}

	// Add check(s) to an existing copy
	static void addCheck(CPMap& cpInfo, const CPEntry& cpe, const Expression::SymbolSet& checks);

	// remove a copy from the current result 
	static bool removeCopy(CPMap& cpInfo, const CPEntry& cpe);
	
	// checks whether there is a backward dependency in the loop
	static bool hasBackDependency(const CopyFlowSet& inSetAtIter1, const CopyFlowSet& inSet, 
							 const FlowEntry& cp);
	
	// relocate a copy entry
	static void moveCopy(const AssignStmt* pAssignStmt,	size_t index,
						 const SymbolExpr* lhs, const CopyFlowSet& genSet,
						 const CopyFlowSet& inSet, CPFlowSet& startSet,
						 CPFlowSet& exitSet, CPMap& cpInfo, 
						 const FlowContextPair& fcPair);
	
	// relocate a copy entry
	static bool moveCopy(const CopyFlowSet& inSet, CPMap& cpInfo,
						 const AssignStmt* pAssignStmt, size_t index,
						 CPFlowSet& fs, const ContxInsFlowEntry&  fe, 
						 const Expression::SymbolSet& checks, 						   
						 FlowContext flowContext);
	
	// A null copy info object for initialization
	static CopyInfo NullEntry;
};

/**********************************************************************
 * Class   : FindByFlowEntry
 * Purpose : function object for locating an abject (given a flow entry)
 * Initial : Nurudeen A. Lameed on December 18, 2009
 ***********************************************************************
 Revisions and bug fixes:
 */
class CPFinderByFlowEntry 
{
public:
	// class constructor, sExpr is a symbol (key to search for later)
	CPFinderByFlowEntry(const ContxInsFlowEntry& flowEntry) :	searchKey(flowEntry){}

	// find a flow entry with symbol ==  searchKey
	bool operator()(const CPEntry& cpe) const {return cpe.flowEntry == searchKey;}
	
private:

	// search key
	ContxInsFlowEntry searchKey;
};

/******************************************************************
 * Class   : CPFinderBySymbol
 * Purpose : function object for locating an abject (given a symbol)
 * Initial : Nurudeen A. Lameed on December 18, 2009
 *******************************************************************
 Revisions and bug fixes:
 */
class CPFinderBySymbol 
{
public:
	// class constructor, stmt is a statement (key to search for later)
	CPFinderBySymbol(const SymbolExpr* pSymbExpr):searchKey(pSymbExpr) {}

	// find a flow entry with symbol =  sKey
	bool operator()(const CPEntry& cpe) const {return cpe.flowEntry.first == searchKey;}
private:
	
	// searck key
	const SymbolExpr* searchKey;
};


/******************************************************************
 * Class   : CPInfoFinderBySymbol
 * Purpose : function object for locating an abject (given a symbol)
 * Initial : Nurudeen A. Lameed on December 18, 2009
 *******************************************************************
 Revisions and bug fixes:
 */
class EntryFinder 
{
public:
	// class constructor, stmt is a statement (key to search for later)
	EntryFinder(const SymbolExpr* pSymbExpr):searchKey(pSymbExpr) {}

	// find a flow entry with symbol =  sKey
	bool operator()(const CopyInfo& cp) const {return cp.first == searchKey;}
private:
	
	// searck key
	const SymbolExpr* searchKey;
};

class ArrayCopyElim
{
 public:

  // Performs array copy optimization analysis
  static AnalysisInfo* computeArrayCopyElim(const ProgFunction* pFunction,
												const StmtSequence* pFuncBody,
												const TypeSetString& inArgTypes,
												bool returnBottom  = false);
 private:
  ArrayCopyElim(){}
  ArrayCopyElim(const ArrayCopyElim& aca){}
};

#endif /*ACOPYPLACEMENT_H_*/
