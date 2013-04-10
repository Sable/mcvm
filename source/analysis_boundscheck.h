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

#ifndef ARRAY_INDEX_ANALYSIS_H_
#define ARRAY_INDEX_ANALYSIS_H_

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
#include "analysismanager.h"

/********************************************************************************
* Class   : IndexKey
* Purpose : A structure that represent <Index, dimension> pair of an array access
* Initial : Nurudeen A. Lameed on July 21, 2009
**********************************************************************************
Revisions and bug fixes:
*/
struct IndexKey 
{
	// index symbol ( i, j, etc. )
	const Expression* indexExpr;
	
	// dimension in the array access
	size_t indexDim;
};

/*******************************************************************
* Class   : FlowFact
* Purpose : An abstraction of the flow analysis' facts
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
struct FlowFact
{
	// constraints type; upper or lower bounds
	enum ConstraintType { LOWER_BOUND, UPPER_BOUND };
		
	// a constructor
	FlowFact(const SymbolExpr* mSymbol, IndexKey iKey, ConstraintType cons):
	matrixSymbol(mSymbol), indexExpr(iKey.indexExpr), constraint(cons), indexDim(iKey.indexDim) {}
	
	// overloaded assignment operator (operator=)for copying FlowFact objects
	FlowFact& operator=(const FlowFact &fact2)
	{
		this->matrixSymbol = fact2.matrixSymbol;
		this->indexExpr = fact2.indexExpr;
		this->constraint = fact2.constraint;
		this->indexDim = fact2.indexDim;
		
		return *this;
	}
	
	// matrix symbol
	const SymbolExpr* matrixSymbol;
	
	// index used in this parameterized expression
	const Expression* indexExpr;
	
	// kind of constraint to be enforced just before the current statment
	ConstraintType constraint; 
	
	// index position (dimension ... etc.)
	size_t indexDim;
};

// a map of index symbol and a set of symbols representing the matrices indexed by the index symbol 
typedef std::map<IndexKey, Expression::SymbolSet> IndexMatrixMap;

// a set of flow facts
typedef std::set<FlowFact> FlowSet;

// a map of statement and its flowset.
typedef std::map<const IIRNode*, FlowSet> FlowSetMap;

// a map of symbol and expression
typedef std::map<const SymbolExpr*, const BinaryOpExpr*> SymbolExprMap;

// a map entry with key type <index, dimension> and values as sets of symbols
typedef std::pair<const IndexKey, Expression::SymbolSet> IndexMatrixMapEntry;

// a map entry with key type SymbolExpr and value, Expression
typedef std::pair<const SymbolExpr*, const BinaryOpExpr*> SymbolExprMapEntry;

// a set of <index, dimension> pairs
typedef std::set<IndexKey> IndexKeySet;

// a map of paramterized expressions and their associated checks by dimension
typedef std::map<const ParamExpr*, std::vector<std::bitset<2> > > BoundsCheckMap;

// array symbol and max bounds for each dimension ...
typedef std::map<SymbolExpr*, TypeInfo::DimVector > ArrayBoundsMap;

/**************************************************************************
* Class   : FindIndexKey
* Purpose : Represents a function object for finding an index by symbolExpr
* Initial : Nurudeen A. Lameed on July 21, 2009
***************************************************************************
Revisions and bug fixes:
*/
class FindIndexBySymbol
{
public:
	
	//constructor
	FindIndexBySymbol(const SymbolExpr* iSymbol):indexSymbol(iSymbol) {}
	
	bool operator()(const IndexMatrixMapEntry entry) const
	{
		return indexSymbol == entry.first.indexExpr;
	}
private:
	
	// symbol used as an index
	const SymbolExpr* indexSymbol;
};

/******************************************************************************
* Class   : FindIndexKey
* Purpose : Represents a function object for finding an <index, dimension> pair
* Initial : Nurudeen A. Lameed on July 21, 2009
*******************************************************************************
Revisions and bug fixes:
*/
class FindIndexKey
{
public:
	
	// constructor
	FindIndexKey(const IndexKey iKey):indexKey(iKey) {}
	
	bool operator()(const IndexMatrixMapEntry& entry) const
	{
		const Expression* iExpr = entry.first.indexExpr;
		size_t iDim = entry.first.indexDim;
		
		if ( iExpr->getExprType() == Expression::INT_CONST)
		{
			return indexKey.indexExpr->getExprType() == Expression::INT_CONST &&
					(dynamic_cast<const IntConstExpr*>(indexKey.indexExpr))->getValue() ==
						(dynamic_cast<const IntConstExpr*>(iExpr))->getValue() && 
						indexKey.indexDim == iDim;
		}
		else
		{
			return indexKey.indexExpr == iExpr && indexKey.indexDim == iDim;
		}
	}
private:
	
	// a <symbol, dimension> pair used to access an array
	const IndexKey indexKey;
};

/********************************************************************************
* Class   : FindExprByBIV
* Purpose : Represents a function object for finding an <symbol, expression> pair
* Initial : Nurudeen A. Lameed on July 21, 2009
*********************************************************************************
Revisions and bug fixes:
*/
class FindExprByBIV
{
public:
	FindExprByBIV(const SymbolExpr* iSymbol):bivSymbol(iSymbol) {}
	
	bool operator()(const SymbolExprMapEntry entry) const
	{
		const BinaryOpExpr* e = entry.second;
		const Expression* lOp = e->getLeftExpr();
		const Expression* rOp = e->getRightExpr();
		
		return (lOp == bivSymbol || rOp == bivSymbol);
	}
private:
	const SymbolExpr* bivSymbol;
};


/*******************************************************************
* Class   : ArrayIndexAnalysis
* Purpose : Array index analysis
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
class ArrayIndexAnalysis
{
public:
	
	// constructor for creating an analysis object
	ArrayIndexAnalysis(const StmtSequence* pFuncBody, const TypeInferInfo* pTypeInferInfo);
	
	virtual ~ArrayIndexAnalysis();	
	
	// return the result of an analysis
	BoundsCheckMap getFlowAnalysisResult() const {return factTable; }
	
	// analyze a function(a sequence of statement)
	void doAnalysis(const StmtSequence* pFuncBody, FlowSetMap& flowSetMap);
	
private:
	
	// predicate
	static bool equalOne(size_t i) { return i == 1; }
	
	// Get an array dimensions' information
	TypeInfo getTypeInfo(const IIRNode* node, const SymbolExpr* symbol) const;
	
	// perform analysis on a loop
	void doAnalysis(const LoopStmt* pLoopStmt,
								const FlowSet& startSet,
								FlowSet& exitSet,
								FlowSet& retSet,
								FlowSetMap& flowSetMap);
	
	// perform analysis on a sequence of statements
	void doAnalysis(const StmtSequence* pStmtSeq,
									const FlowSet& startSet,
									FlowSet& exitSet,
									FlowSet& retSet,
									FlowSet& breakSet,
									FlowSet& contSet,
									FlowSetMap& flowSetMap);
	
	// perform analysis on an ifelse statement
	void doAnalysis(const IfElseStmt* pIfStmt,
										const FlowSet& startSet,
										FlowSet& exitSet,
										FlowSet& retSet,
										FlowSet& breakSet,
										FlowSet& contSet,
										FlowSetMap& flowSetMap);
	
	// update flow set using new bounds for dimensions
	void updateFlowSet(const SymbolExpr* symbol, const TypeInfo::DimVector& bounds, FlowSet& out);
	
	// delete all cached expressions that depend on a given basic iv
	int deleteCachedExpr(const SymbolExpr* bivSymbol);
	
	// search for a given expression from the table of cached expressions
	bool searchCachedExpr(const BinaryOpExpr* bOpExpr, const SymbolExpr*& symb) const;
	
	// print the analysis' result
	void print() const;
	
	// get all indices as used in array accesses
	void getAllIndexKeys(const SymbolExpr* iSymbol, IndexKeySet& allIndexKeys);
	
	// check whether an expression contains a basic induction variable
	inline bool isBasicIV(const SymbolExpr* iSymbol, const BinaryOpExpr* bOpExpr) const;
	
	// check whether an expression contains a basic induction variable
	inline bool isDependentExpr(const BinaryOpExpr* bOpExpr, const SymbolExpr*& ivClass) const;
	
	// check whether a symbol represents an index to a matrix
	inline bool isIndex(const SymbolExpr* iSymbol) const;
	
	// check whether a symbol represents an index to a matrix
	inline bool isIndex(const IndexKey& ikey) const;
	
	// returns full checks for all index symbols
	FlowSet top() const;
	
	// return an empty flow set
	FlowSet bottom() const { return FlowSet(); };

	// flow functions for different statements
	FlowSet flowFunction(const Statement* stmt, const FlowSet& in);
	
	// compute out set for any statement involving a matrix
	void computeOutSet(const Expression* expr, FlowSet& out);
	
	// compute out set for any statement involving an index update
	void computeOutSet(const SymbolExpr* iSymbol, const Expression* rhs, FlowSet& out);
	
	// compute out set for an add, mult ... expression involving an index
	void computeOutSetIndex(const SymbolExpr* iSymbol, const BinaryOpExpr* bOp, 
			FlowSet& out, const FlowFact::ConstraintType constraint);
	
	// compute out set for other expressions involving an index
	void computeOutSetIndexAll(const SymbolExpr* iSymbol, FlowSet& out);
	
	// traverse the tree to find all matrix/index symbols in the function 
	void findAllMatrixIndexSymbols(const StmtSequence* pFuncBody);
	
	// update the index->matrix map
	void fillSets(const Expression* expr);
	
	// a map of index -> matrices
	IndexMatrixMap indexMatMap; 
	
	// array symbol and max bounds for each dimension ... 
	ArrayBoundsMap arrayBoundsMap;
	
	// all matrices/arrays in the function
	Expression::SymbolSet allMatrixSymbols;
	
	// a map of symbol and expression
	SymbolExprMap symbExprMap;
	
	// flow analysis result per parameterized expression;
	// a map of paramExpr node and the checks for all its indices as 
	// used in an expression.
	BoundsCheckMap factTable;
	
	// type inference information
	const TypeInferInfo* typeInferenceInfo;
};

/***************************************************************
* Class   : BoundsCheckInfo
* Purpose : Store bounds checking analysis information
* Initial : Nurudeen A. Lameed on July 21, 2009
****************************************************************
Revisions and bug fixes:
*/
class BoundsCheckInfo : public AnalysisInfo
{
public:
	
	// array index analysis result map
	BoundsCheckMap boundsCheckMap;
	
	// A map of set of checks reaching a given statement
	FlowSetMap flowSetMap;
	
	// return whether a bound check should be performed for a parameterized expr
	// bound = 1 (default), upper bound;  bound = 0, for lower bound
	bool isBoundsCheckRequired(const ParamExpr* expr, size_t dim, int bound = 1) const
	{	
		// get bounds check info for the dimension
		BoundsCheckMap::const_iterator mapItr = boundsCheckMap.find(expr);
		assert (mapItr != boundsCheckMap.end());
		std::vector<std::bitset<2> > dimInfo(mapItr->second);
		
		// sanity check
		assert (0 <= bound && bound < 2 && dim < dimInfo.size());
				
		return dimInfo[dim].test(bound); 
	}
};

// Performs an array bounds check elimination
AnalysisInfo* computeBoundsCheck(
	const ProgFunction* pFunction,
	const StmtSequence* pFuncBody,
	const TypeSetString& inArgTypes,
	bool returnBottom	
);

// function prototype BoundsCheckMap::operator<<
std::ostream&  operator<<(std::ostream& out, const BoundsCheckMap& ftab);

#endif /*ARRAY_INDEX_ANALYSIS_H_*/
