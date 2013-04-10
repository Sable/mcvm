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

/***************************************************************
*
* The programming code contained in this file was written
* by Nurudeen A. Lameed. For information, please
* contact the author.
*
* Contact information:
* e-mail: nlamee@cs.mcgill.ca
*
***************************************************************/

#include <vector>
#include <iostream>
#include <iterator>
#include <cassert>
#include "analysis_boundscheck.h"

/*******************************************************************
* Function:	operator<<
* Purpose : Overload operator<< for displaying a SymbolExpr pointer
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
std::ostream& operator<<(std::ostream& out, const SymbolExpr* symb)
{
	out << symb->toString();
	
	return out;
}

/*******************************************************************
* Function:	operator<<
* Purpose : Overload operator<< for displaying a FlowFact object
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
std::ostream& operator<<(std::ostream& out, const FlowFact& fFact)
{	
	if (fFact.matrixSymbol && fFact.indexExpr)
	{
		out << "Matrix: " << fFact.matrixSymbol->toString() <<
		" Subscript: " << fFact.indexExpr->toString() <<
		" Constraint: " << (fFact.constraint == FlowFact::UPPER_BOUND ? "U": "L") <<
		" Dimension: " << fFact.indexDim;
	}
		
	return out;
}

/*******************************************************************
* Function:	operator<<
* Purpose : Overload operator<< for displaying a FlowSet object
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
std::ostream& operator<<(std::ostream& out, const FlowSet& fSet)
{	
	out << "\n{\n";
	std::copy(fSet.begin(), fSet.end(), std::ostream_iterator<FlowFact>(out, "\n"));
	out << "}\n";

	return out;
}

/*******************************************************************
* Function:	operator==
* Purpose : Overload operator== for comparing FlowFact objects
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
bool operator==(const FlowFact &fact1, const FlowFact &fact2)
{
	return fact1.matrixSymbol == fact2.matrixSymbol &&
			fact1.indexExpr == fact2.indexExpr &&
			fact1.constraint == fact2.constraint &&
			fact1.indexDim == fact2.indexDim;
}

/*******************************************************************
* Function:	operator<
* Purpose : Overload operator< for comparing FlowFact objects
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
bool operator<(const FlowFact &fact1, const FlowFact &fact2)
{
	assert( fact1.indexExpr != 0 && fact1.matrixSymbol != 0 &&
			fact2.indexExpr != 0 && fact2.matrixSymbol != 0 );
	
	// open string streams 
	std::ostringstream s1, s2;
	
	// build a unique string for fact1
	if (fact1.indexExpr->getExprType() == Expression::INT_CONST )
	{
		s1 << (dynamic_cast<const IntConstExpr*>(fact1.indexExpr))->getValue();
	}
	else
	{
		s1 << fact1.indexExpr;
	}
	
	s1 << fact1.matrixSymbol << fact1.constraint << fact1.indexDim;
	
	//build a unique string for fact2
	if (fact2.indexExpr->getExprType() == Expression::INT_CONST )
	{
		s2 << (dynamic_cast<const IntConstExpr*>(fact2.indexExpr))->getValue();
	}
	else
	{
		s2 << fact2.indexExpr;
	}
	
	s2 << fact2.matrixSymbol << fact2.constraint << fact2.indexDim;

	// compare the two strings 
	return  s1.str() < s2.str();
}

/*******************************************************************
* Function:	operator==
* Purpose : Overload operator== for comparing IndexKey objects
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
bool operator==(const IndexKey& key1, const IndexKey& key2)
{
	return key1.indexExpr->getExprType() == Expression::INT_CONST ? 
			
			key1.indexExpr->getExprType() == Expression::INT_CONST && 
			key2.indexExpr->getExprType() == Expression::INT_CONST &&
			(dynamic_cast<const IntConstExpr*>(key1.indexExpr))->getValue()  == 
			(dynamic_cast<const IntConstExpr*>(key2.indexExpr))->getValue() :
				
			key1.indexExpr == key2.indexExpr && key1.indexDim == key2.indexDim;
}

/*******************************************************************
* Function:	operator<
* Purpose : Overload operator< for comparing IndexKey objects
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
bool operator<(const IndexKey& key1, const IndexKey& key2)
{
	// open string streams 
	std::ostringstream s1, s2;
		
	// build a unique string for indexkey1
	if (key1.indexExpr->getExprType() == Expression::INT_CONST )
	{
		s1 << (dynamic_cast<const IntConstExpr*>(key1.indexExpr))->getValue(); 
	}
	else
	{
		s1 << key1.indexExpr;
	}
		
	s1 << key1.indexDim;
		
	// build a unique string for indexkey1
	if (key2.indexExpr->getExprType() == Expression::INT_CONST )
	{
		s2 << (dynamic_cast<const IntConstExpr*>(key2.indexExpr))->getValue(); 
	}
	else
	{
		s2 << key2.indexExpr;
	}
		
	s2 << key2.indexDim;

	// compare the two strings 
	return  s1.str() < s2.str();
}

/*******************************************************************
* Function:	operator<<
* Purpose : For printing BoundsCheckMap object
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
std::ostream&  operator<<(std::ostream& out, const BoundsCheckMap& ftab)
{	
	int total = 0;
	int mandatoryChecks = 0;
	
	for (BoundsCheckMap::const_iterator it = ftab.begin(), iEnd = ftab.end(); it != iEnd; ++it)
	{
		// display the parameterized expression
		out << it->first->toString() << ":"  << std::endl;
		
		// get the check info for all dimensions
		std::vector<std::bitset<2> > v = it->second;
		
		for (size_t i = 0, size = v.size(); i < size; ++i )
		{
			// retrieve the checks
			std::bitset<2> b = v[i];
			
			total += 2; 	// b.size();
			mandatoryChecks += b.count();	// no of bits that are on
			
			// display the checks for this index as boolean values
			out << '\t' << i << ": " << '\t'<< std::boolalpha << b[0] 
					  << '\n' << "\t\t" << b[1] << std::endl;
		}
	}
	
	// compute savings
	double savings = total - mandatoryChecks; 
	
	std::cout << "Number of possible checks: " << total << std::endl;
	std::cout << "Number of checks eliminated: " << savings << " (" 
	<< ((savings/(total > 0? total: 1))*100) << "%)" << std::endl;
	
	return out;
}

/*******************************************************************
* Function:	ArrayIndexAnalysis::Print
* Purpose : Print analysis result by parameterized expressions
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
void ArrayIndexAnalysis::print() const
{	
	int total = 0;
	int mandatoryChecks = 0;
	
	for (BoundsCheckMap::const_iterator it = factTable.begin(), 
		   iEnd = factTable.end(); it != iEnd; ++it)
	{
		// display the parameterized expression
		std::cout << it->first->toString() << ":"  << std::endl;
		
		// get the check info for all dimensions
		std::vector<std::bitset<2> > v = it->second;
		
		for (size_t i = 0, size = v.size(); i < size; ++i )
		{
			// retrieve the checks
			std::bitset<2> b = v[i];
			
			total += 2; 	// b.size();
			mandatoryChecks += b.count();	// no of bits that are on
			
			// display the checks for this index as boolean values
			std::cout << '\t' << i << ": " << '\t'<< std::boolalpha << b[0] 
					  << '\n' << "\t\t" << b[1] << std::endl;
		}
	}
	
	// compute savings
	double savings = total - mandatoryChecks; 
	
	std::cout << "Number of possible checks: " << total << std::endl;
	std::cout << "Number of checks eliminated: " << savings << " (" 
	<< ((savings/(total > 0? total: 1))*100) << "%)" << std::endl;
}

/*******************************************************************
* Function:	ArrayIndexAnalysis::top
* Purpose : Returns a full flow set containing all possible checks 
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
FlowSet ArrayIndexAnalysis::top() const
{
	// accumulate all initial flow facts ... 
	FlowSet fSet;
	
	for (IndexMatrixMap::const_iterator iter = indexMatMap.begin(),
		 iEnd = indexMatMap.end(); iter != iEnd; ++iter)
	{
		Expression::SymbolSet mSet =  iter->second;		
		for (Expression::SymbolSet::const_iterator miter = mSet.begin(), 
			 itEnd = mSet.end(); miter != itEnd; ++miter)
		{	
	      const SymbolExpr* arrayVar = *miter; 

			// insert flow facts
			fSet.insert(FlowFact(arrayVar, iter->first, FlowFact::LOWER_BOUND));
			fSet.insert(FlowFact(arrayVar, iter->first, FlowFact::UPPER_BOUND));
		}
	}
	
	// return the set of flow facts
	return fSet;
}

/*******************************************************************
* Function:	ArrayIndexAnalysis::isIndex
* Purpose : Check whether a given symbol is an index of an array 
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
inline bool ArrayIndexAnalysis::isIndex(const SymbolExpr* iSymbol) const
{
	// is the symbol used as an index of a parameterized expression?
	IndexMatrixMap::const_iterator iter = 
		std::find_if(indexMatMap.begin(), indexMatMap.end(), FindIndexBySymbol(iSymbol));
	
	return iter != indexMatMap.end();
}

/*********************************************************************************
* Function:	ArrayIndexAnalysis::isIndex
* Purpose : Check whether a given <symbol, dimension> pair is an index of an array 
* Initial : Nurudeen A. Lameed on July 21, 2009
***********************************************************************************
Revisions and bug fixes:
*/
inline bool ArrayIndexAnalysis::isIndex(const IndexKey& ikey) const
{	
	// is the symbol used as an index of a parameterized expression in a
	// given dimension?
	IndexMatrixMap::const_iterator iter = 
		std::find_if(indexMatMap.begin(), indexMatMap.end(), FindIndexKey(ikey));
		
	return iter != indexMatMap.end();
}

/*******************************************************************
* Function:	ArrayIndexAnalysis::findAllMatrixIndexSymbols
* Purpose : Find all indices used in the current function
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
void ArrayIndexAnalysis::findAllMatrixIndexSymbols(const StmtSequence* pFuncBody)
{
	// get the sequence of statements
	const StmtSequence::StmtVector& stmts = pFuncBody->getStatements();
	
	// look for statements with a parameterized expression
	for (StmtSequence::StmtVector::const_iterator it = stmts.begin(), 
		   iEnd = stmts.end(); it != iEnd; ++it)
	{
		Statement* pStmt = *it;
		switch( pStmt->getStmtType() )
		{
			case Statement::ASSIGN:
			{
				AssignStmt* aStmt = dynamic_cast<AssignStmt*>(pStmt);
				Expression::ExprVector lhs = aStmt->getLeftExprs();
				
				for (size_t i=0, size = lhs.size(); i < size; ++i)
				{
					fillSets(lhs[i]);
				}
				
				fillSets(aStmt->getRightExpr());
			}
			break;
			
			case Statement::EXPR:
			{
				ExprStmt* eStmt = dynamic_cast<ExprStmt*>(pStmt);
				fillSets(eStmt->getExpression());
			}
			break;
			
			case Statement::IF_ELSE:
			{
				// convert into an if type
				IfElseStmt* ifstmt = (IfElseStmt*)pStmt;
				
				// get all symbols
				findAllMatrixIndexSymbols(ifstmt->getIfBlock());
				findAllMatrixIndexSymbols(ifstmt->getElseBlock());					
			}
			break;
			
			case Statement::LOOP:
			{
				LoopStmt* lstmt = (LoopStmt*)pStmt;
				
				// loop initialization
				StmtSequence* initSeq = lstmt->getInitSeq();
				findAllMatrixIndexSymbols(initSeq);
				
				// loop test
				StmtSequence* testSeq = lstmt->getTestSeq();
				findAllMatrixIndexSymbols(testSeq);
				
				// loop body
				StmtSequence* sBodySeq = lstmt->getBodySeq();
				findAllMatrixIndexSymbols(sBodySeq);

				// loop increment statement
				StmtSequence* incrSeq = lstmt->getIncrSeq();
				findAllMatrixIndexSymbols(incrSeq);
			}
			
			default:
			{	
			}
		}
	}
}

/*******************************************************************
* Function:	ArrayIndexAnalysis::fillSets
* Purpose : Fill data structures for the flow analysis  
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
void ArrayIndexAnalysis::fillSets(const Expression *expr)
{							
	// what kind of expression?
	switch (expr->getExprType())
	{
		case Expression::PARAM:
		{
			const ParamExpr* paramExpr = dynamic_cast<const ParamExpr*>(expr);
			
			// matrix/array symbol
			SymbolExpr* mSymbol = paramExpr->getSymExpr();
			
			// assume for now, all parameterized expression symbols are matrices ...
			// collect in one place all array/matrices symbols
			allMatrixSymbols.insert(mSymbol);
			
			const Expression::ExprVector& args = paramExpr->getArguments();
			
			// insert the indexes
			for (size_t j = 0, size = args.size(); j < size; ++j)
			{	
				if ( args[j]->getExprType() == Expression::SYMBOL ||
						args[j]->getExprType() == Expression::INT_CONST )
				{	
					IndexKey iKey = {args[j], j};
					
					if (!isIndex(iKey))
					{
						// new matrix, insert it.
						Expression::SymbolSet s;
						s.insert(mSymbol);
						indexMatMap[iKey] = s;			
					}
					else
					{
						indexMatMap[iKey].insert(mSymbol);
					}
				}
			}
		}
		break;
		
		case Expression::BINARY_OP:
		{
			const BinaryOpExpr* bOp = dynamic_cast<const BinaryOpExpr*>(expr);
			fillSets(bOp->getLeftExpr());
			fillSets(bOp->getRightExpr());
		}
		break;
		
		case Expression::UNARY_OP:
		{
			const UnaryOpExpr* uOp = dynamic_cast<const UnaryOpExpr*>(expr);
			fillSets(uOp->getOperand());				
		}
		break;
		
		default:	// do nothing
		{	
		}
	}
}
// end of data collection routines 

// analysis routines
/*******************************************************************
* Function:	ArrayIndexAnalysis::getTypeInfo
* Purpose : Get an array dimensions' information
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
inline TypeInfo ArrayIndexAnalysis::getTypeInfo(const IIRNode* node, const SymbolExpr* symbol) const
{
	// retrieve the type set
	TypeInfoMap post = typeInferenceInfo->postTypeMap;
	TypeSet tSet = (post[node])[symbol];
	
	// look for one with bounds information
	// if none has the info, return an empty type info.
	for (TypeSet::const_iterator it = tSet.begin(), iEnd = tSet.end(); it != iEnd; ++it)
	{
		TypeInfo typ = *it;
		
		if ( typ.getSizeKnown() )
		{
			return *it;
		}
	}
	
	return TypeInfo(); // return an empty type info
}

/*******************************************************************
* Function:	ArrayIndexAnalysis::doAnalysis
* Purpose : Analyze a function body
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
void ArrayIndexAnalysis::doAnalysis(const StmtSequence* pFuncBody, FlowSetMap& flowSetMap)
{
	// find all the index symbols in the current function
	findAllMatrixIndexSymbols(pFuncBody);
	
	// get the initial flow set
	const FlowSet& startSet = top();
	
	// declare output sets
	FlowSet exitSet;
	FlowSet retSet;
	FlowSet breakSet;
	FlowSet contSet;
	
	// perform flow analyis on the function body
	doAnalysis(pFuncBody, 
									startSet,
									exitSet,
									retSet,
									breakSet,
									contSet,
									flowSetMap);
}

/*******************************************************************
* Function:	ArrayIndexAnalysis::doAnalysis
* Purpose : Analyze a loop statement
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
void ArrayIndexAnalysis::doAnalysis(const LoopStmt* pLoopStmt,
						const FlowSet& startSet,
						FlowSet& exitSet,
						FlowSet& retSet,
						FlowSetMap& flowSetMap)
{
	// declare out set
	FlowSet initExitSet;
	
	// declare other output sets
	FlowSet initRetSet;
	FlowSet initBreakSet;
	FlowSet initContSet;
	
	// perform flow analysis on the sequence of statements for loop initialization
	doAnalysis(
			pLoopStmt->getInitSeq(),
			startSet,
			initExitSet,
			initRetSet,
			initBreakSet,
			initContSet,
			flowSetMap
			);
	
	// Declare a variable for the current loop array bound checks
	//FlowSetMap loopSetMap;	
	
	// declare current increment stmt output set
	FlowSet curIncrExitSet;
	
	// loop until a fixed point is reached.
	while(true)
	{
		// Clear the loop info map
		//loopSetMap.clear();
		
		// declare and intialize in set for loop test statement
		FlowSet testStartSet(initExitSet);
		testStartSet.insert(curIncrExitSet.begin(), curIncrExitSet.end());
		
		// 
		FlowSet testExitSet;
		FlowSet testRetSet;
		FlowSet testBreakSet;
		FlowSet testContSet;
		
		// perform flow analysis on the sequence of statements for loop test
		doAnalysis(
				pLoopStmt->getTestSeq(),
				testStartSet,
				testExitSet,
				testRetSet,
				testBreakSet,
				testContSet,
				flowSetMap
				);
		
		FlowSet bodyExitSet;
		FlowSet breakSet;
		FlowSet contSet;
		
		// perform flow analysis on the loop's body
		doAnalysis(
						pLoopStmt->getBodySeq(),
						testExitSet,		// in
						bodyExitSet,		// out
						retSet,
						breakSet,
						contSet,
						flowSetMap
						);
		
		// Merge the test exit map and the break map
		breakSet.insert(testExitSet.begin(), testExitSet.end());
		
		FlowSet incrStartSet(bodyExitSet);
		incrStartSet.insert(contSet.begin(), contSet.end());

		FlowSet incrExitSet;
		FlowSet incrRetSet;
		FlowSet incrBreakSet;
		FlowSet incrContSet;
		
		// perform flow analysis on the sequence of statements for the loop increment statement
		doAnalysis(
						pLoopStmt->getIncrSeq(),
						incrStartSet,
						incrExitSet,
						incrRetSet,
						incrBreakSet,
						incrContSet,
						flowSetMap
						);
		
		// update
		exitSet = breakSet;
		
		// is fixed point reached?
		if (curIncrExitSet == incrExitSet)
		{
			break;
		}
	
		curIncrExitSet = incrExitSet;
	}
	
	// collect the result
	//flowSetMap.insert(loopSetMap.begin(), loopSetMap.end());
}

/*******************************************************************
* Function:	ArrayIndexAnalysis::doAnalysis
* Purpose : Analyze an If then else statement
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
void ArrayIndexAnalysis::doAnalysis(const IfElseStmt* pIfStmt,
								const FlowSet& startSet,
								FlowSet& exitSet,
								FlowSet& retSet,
								FlowSet& breakSet,
								FlowSet& contSet,
								FlowSetMap& flowSetMap)
{
	// Get the condition expression
	Expression* pTestExpr = pIfStmt->getCondition();
	
	flowSetMap[pTestExpr] = startSet;
	FlowSet ifSet;
	FlowSet elseSet;
	
	// analyze the if block
	doAnalysis(
							pIfStmt->getIfBlock(),
							startSet,
							ifSet,
							retSet,
							breakSet,
							contSet,
							flowSetMap
						);
	
	// analyze the else block
	doAnalysis(
								pIfStmt->getElseBlock(),
								startSet,
								elseSet,
								retSet,
								breakSet,
								contSet,
								flowSetMap
							);
	
	// merge set
	exitSet.insert(ifSet.begin(), ifSet.end());
	exitSet.insert(elseSet.begin(), elseSet.end());
}

/*******************************************************************
* Function:	ArrayIndexAnalysis::doAnalysis
* Purpose : Analyze a block ( sequence of statements)
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
void ArrayIndexAnalysis::doAnalysis(const StmtSequence* pStmtSeq,
							const FlowSet& startSet,
							FlowSet& exitSet,
							FlowSet& retSet,
							FlowSet& breakSet,
							FlowSet& contSet,
							FlowSetMap& flowSetMap)
{
	// declare a reference to the sequence of statements
	const StmtSequence::StmtVector& stmts = pStmtSeq->getStatements();
	
	// declare and initalize the current with the input set
	FlowSet curSet = startSet;	
	
	// store the in set for the current sequence of statements
	flowSetMap[pStmtSeq] = curSet;

	// examine the statements and take actions
	for (StmtSequence::StmtVector::const_iterator it = stmts.begin(), 
		   iEnd = stmts.end(); it != iEnd; ++it)
	{
		Statement* pStmt = *it;
		switch( pStmt->getStmtType() )
		{
			case Statement::BREAK:
			{
				// copy the current flow set
				breakSet.insert(curSet.begin(), curSet.end());
			}
			break;
			
			case Statement::CONTINUE:
			{
				// copy the current flow set
				contSet.insert(curSet.begin(), curSet.end());		
			}
			break;
			
			case Statement::RETURN:
			{
				// copy the current flow set
				retSet.insert(curSet.begin(), curSet.end());		
			}
			break;
			
			case Statement::IF_ELSE:
			{
				// store the in-set
				flowSetMap[pStmt] = curSet;
				
				// declare out-set for the analysis 
				FlowSet exitSet;
				
				doAnalysis(
						dynamic_cast<const IfElseStmt*>(pStmt),
						curSet,
						exitSet,
						retSet,
						breakSet,
						contSet,
						flowSetMap
						);
				
				// update the current flow set with the outset from this analysis
				curSet = exitSet;
				
			}
			break;
			
			case Statement::LOOP:
			{
				// store the in-set
				flowSetMap[pStmt] = curSet;
				
				// declare out-set for the analysis 
				FlowSet exitSet;
				
				// perform flow analysis on a loop
				doAnalysis(
						dynamic_cast<const LoopStmt*>(pStmt),
						curSet,
						exitSet,
						retSet,
						flowSetMap
						);
				
				// update the current flow set with the outset from this analysis
				curSet = exitSet;				
			}
			break;
			
			default:
			{
				// store the in-set
				flowSetMap[pStmt] = curSet;
				
				// compute the out-set
				curSet = flowFunction(pStmt, curSet);
			}
		}
	}
	
	// the current out set
	exitSet.insert(curSet.begin(), curSet.end());
}

/*******************************************************************
* Function:	ArrayIndexAnalysis::updateFlowSet
* Purpose : Get all index keys assoaciated with this index symbols
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
void ArrayIndexAnalysis::updateFlowSet(const SymbolExpr* symbol, const TypeInfo::DimVector& bounds, FlowSet& out)
{	
	FlowSet t = top();	// get all checks.
	size_t size = bounds.size();
	
	// update flow set
	for (FlowSet::const_iterator iter = t.begin(), iEnd = t.end(); iter != iEnd; ++iter)
	{
		const FlowFact& ff = *iter;
		if ( (ff.matrixSymbol == symbol) && (ff.indexDim < size) 
				&& (ff.indexExpr->getExprType() == Expression::INT_CONST) )	// valid known dimension
		{	
			// subscript value 
			size_t subscript = (dynamic_cast<const IntConstExpr*>(ff.indexExpr))->getValue();
			
			if (1 > subscript || subscript > bounds[ff.indexDim])  	// outside the range
			{
				out.insert(ff);
			}
			else	// in the range
			{
				out.erase(ff);
			}
		}
	}
}

/*******************************************************************
* Function:	ArrayIndexAnalysis::getAllIndexKeys
* Purpose : Get all index keys assoaciated with this index symbol
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
void ArrayIndexAnalysis::getAllIndexKeys(const SymbolExpr* iSymbol, IndexKeySet& allIndexKeys)
{
  FindIndexBySymbol finder(iSymbol);

	// find the first <index, dimension> pair
	IndexMatrixMap::iterator iEnd = indexMatMap.end(), 
	iter = std::find_if(indexMatMap.begin(), iEnd, finder);
	
	while (iter != iEnd)
	{
		allIndexKeys.insert(iter->first);
		
		// find the next key
		iter = std::find_if(++iter, iEnd, finder);
	}
	
}

/*******************************************************************
* Function:	ArrayIndexAnalysis::searchExpr
* Purpose : Lookup an expression
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
bool ArrayIndexAnalysis::searchCachedExpr(const BinaryOpExpr* bOpExpr, const SymbolExpr*& eqVar) const
{

  for (std::map<const SymbolExpr*, const BinaryOpExpr*>::const_iterator it = symbExprMap.begin(),
		 iEnd = symbExprMap.end(); it != iEnd; ++it)
	{
		const BinaryOpExpr* e = it->second;
		
		// e ... must conform to  the pattern  i op c ... 
		const Expression* elOp = e->getLeftExpr();
		const Expression* erOp = e->getRightExpr();
		const SymbolExpr* eClass = dynamic_cast<const SymbolExpr*>(elOp->getExprType() == Expression::SYMBOL ? elOp : erOp);
		const IntConstExpr* eConst = dynamic_cast<const IntConstExpr*>(elOp->getExprType() == Expression::INT_CONST ? elOp : erOp);
		
		
		// if not, check the class of the DIV
		const Expression* lOp = bOpExpr->getLeftExpr();
		const Expression* rOp = bOpExpr->getRightExpr();
		const SymbolExpr* ivClass = dynamic_cast<const SymbolExpr*>(lOp->getExprType() == Expression::SYMBOL ? lOp : rOp);
		const IntConstExpr* ivConst = dynamic_cast<const IntConstExpr*>(lOp->getExprType() == Expression::INT_CONST ? lOp : rOp);
		
		eqVar = it->first;
		
		if (eClass == ivClass &&  ivConst != 0 && eConst->getValue() == ivConst->getValue() )  // equal ...
		{
			return true;	// equivalent expression found
		}
	}
	
	return  false;	// equivalent expression could not be found
}

/*******************************************************************
* Function:	ArrayIndexAnalysis::deleteCachedExpr
* Purpose : Deletes an invalidated expression
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
int ArrayIndexAnalysis::deleteCachedExpr(const SymbolExpr* bivSymbol)
{
	int noErased = 0;
	SymbolExprMap::const_iterator iter = 
		std::find_if(symbExprMap.begin(), symbExprMap.end(), FindExprByBIV(bivSymbol) );
	
	while (iter != symbExprMap.end())
	{	
		noErased += symbExprMap.erase(iter->first);
		
		// find the next key
		iter = std::find_if( symbExprMap.begin(), symbExprMap.end(), FindExprByBIV(bivSymbol) );
	}
	
	// return the number of items erased.
	return noErased;
}

/*******************************************************************
* Function:	ArrayIndexAnalysis::flowFunction
* Purpose : compute out set from in set based on the type of statement
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
FlowSet ArrayIndexAnalysis::flowFunction(const Statement* stmt, const FlowSet& in)
{
	// declare out flow set 
	FlowSet out;
	
	// copy in flow set into out flow set
	out.insert(in.begin(), in.end());
	
	switch(stmt->getStmtType())
	{
		case Statement::ASSIGN:
		{
			const AssignStmt* aStmt = dynamic_cast<const AssignStmt*>(stmt);
			
			// get the left handsides
			Expression::ExprVector lhs = aStmt->getLeftExprs();
			SymbolExpr* symbol = 0;
			
			if (lhs.size() == 1 && lhs[0]->getExprType() == Expression::SYMBOL)
			{
				symbol = dynamic_cast<SymbolExpr*>(lhs[0]);
			}
			else
			{
				// get all left symbols/expressions
			  for (size_t i=0, size = lhs.size(); i < size; ++i)
				{	
					// update out set, if it is a parameterized expression
					computeOutSet(lhs[i], out);
				}
			}
			
			// check the right hand side 
			Expression* rhs = aStmt->getRightExpr();
					
			// is symbol an array/matrix  with known size?
			if ( allMatrixSymbols.find(symbol) != allMatrixSymbols.end() )  // symbol is used as an array ...
			{	
				// get the type information
				TypeInfo typ = getTypeInfo(aStmt, symbol);				
		
				if ( typ.getSizeKnown())
				{
					const TypeInfo::DimVector& dimV = typ.getMatSize();
					TypeInfo::DimVector bounds;
					
					// remove dimensions with value 1; (e.g. row or column vectors)
					std::remove_copy_if(dimV.begin(), dimV.end(), std::back_inserter(bounds), equalOne);
					
					// update the table
					arrayBoundsMap[symbol] = bounds;
					
					// update the flow set using the bounds
					updateFlowSet(symbol, bounds, out);
				}
			}
			
			// update outset (for parameterized expression)
			computeOutSet(rhs, out);
			
			// update outset (for index update 
			if (isIndex(symbol))
			{
				// compute flow set for index update
				computeOutSet(symbol, rhs, out);
			}
		}
		break;
		
		case Statement::EXPR:
		{
			// convert the statement to an expression
			const ExprStmt* aStmt = dynamic_cast<const ExprStmt*>(stmt);
			
			// compute out set for the statement
			computeOutSet(aStmt->getExpression(), out);
		}
		break;
		
		default: // do nothing;
		{
		}
	}
	
	return out;
}

/*******************************************************************
* Function:	ArrayIndexAnalysis::computeOutSet
* Purpose : a helper function to compute the out set an expression
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
void ArrayIndexAnalysis::computeOutSet(const Expression* expr, FlowSet& out)
{
	// what kind of expression?
	switch (expr->getExprType())	
	{
		case Expression::PARAM:
		{
			// Convert it to a parameter type
			const ParamExpr* paramExpr = dynamic_cast<const ParamExpr*>(expr);
			
			// array/matrix symbol
			SymbolExpr* symbol = paramExpr->getSymExpr();
			const Expression::ExprVector& args = paramExpr->getArguments();
				
			// build a table of checks for the indices  
			std::vector< std::bitset<2> > checkTab(args.size());
			
			TypeInfo::DimVector bounds(args.size(), 0);
								
			ArrayBoundsMap::const_iterator it  = arrayBoundsMap.find(symbol);
			if ( it != arrayBoundsMap.end() )	// already defined, get it
			{
				bounds = it->second;	// get the bounds
			}
			
			// insert the indexes
			for (size_t j = 0, size = args.size(); j < size; ++j)
			{
				// checks for the current index
				// bit 0 represents lower check while bit 1 represents upper check
				std::bitset<2> checks;
				
				// if it is a const, check against the bounds table,  1 >= index <= its dimension bound
				// remove all checks for the current index
				// if the index is greater than the bound from the table, update the 
				// bound for the dimension with this index value ...
				
				// form an <index, dimension> pair
				IndexKey iKey = {args[j], j};
				
				bool mustCheck = false;		// are checks necessary for this access?
				
				// remove check for lower bound
				if ( out.erase( FlowFact(symbol, iKey, FlowFact::LOWER_BOUND)) > 0 )
				{
					checks.set(0);			// a mandatory check
					mustCheck = true;		// a mandatory check must be performed for this access
				}
				
				// remove check for upper bound
				if ( out.erase( FlowFact(symbol, iKey, FlowFact::UPPER_BOUND)) > 0 )
				{
					checks.set(1);			// a mandatory check
					mustCheck = true;		// a mandatory check must be performed for this access
				}
				
				// generate others ... 
				if (  mustCheck && args[j]->getExprType() == Expression::INT_CONST )
				{
					size_t subscript = (dynamic_cast<const IntConstExpr*>(args[j]))->getValue();
					
					// replace the bound for the array dimension for the subscript with the subscript  
					bounds[j] = subscript;   // must be greater than the current value
					
					// update flow set
					updateFlowSet(symbol, bounds, out);
				}
				
				// insert the table
				checkTab[j] = checks;
				
				// update the table
				arrayBoundsMap[symbol] = bounds;
			}
			
			// insert a table of checks for the indices of this array access 
			factTable[paramExpr] = checkTab;
		}
		break;
		
		case Expression::BINARY_OP:
		{	
			// convert to binary operator type
			const BinaryOpExpr* bOpExpr = dynamic_cast<const BinaryOpExpr*>(expr);
			
			// analyse right expr first
			computeOutSet(bOpExpr->getRightExpr(), out);
			
			// analyse left expr
			computeOutSet(bOpExpr->getLeftExpr(), out);
		}
		break;
		
		case Expression::UNARY_OP:
		{
			const UnaryOpExpr* uOpExpr = dynamic_cast<const UnaryOpExpr*>(expr);
						
			// analyse the operand
			computeOutSet(uOpExpr->getOperand(), out);
		}
		break;
		
		default:	// do nothing
		{	
		}
	}
}

/*******************************************************************
* Function:	ArrayIndexAnalysis::computeOutSet
* Purpose : a helper function to compute the out set for an index
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
void ArrayIndexAnalysis::computeOutSet(const SymbolExpr* iSymbol, const Expression* rhs, FlowSet& out)
{	
	// a binary expression?
	if (rhs->getExprType() == Expression::BINARY_OP)
	{
		const BinaryOpExpr* bOp = dynamic_cast<const BinaryOpExpr*>(rhs);
		
		switch(bOp->getOperator())
		{
			// an increase
			case BinaryOpExpr::PLUS:
			case BinaryOpExpr::MULT:
			{				
				computeOutSetIndex(iSymbol, bOp, out, FlowFact::UPPER_BOUND);
			}	 
			break;
			
			// a decrease
			case BinaryOpExpr::MINUS:
			case BinaryOpExpr::DIV:
			{
				computeOutSetIndex(iSymbol, bOp, out, FlowFact::LOWER_BOUND);
			}
			break;
			
			default:
			{	
				// for other kind of operators, generate checks for this index
				computeOutSetIndexAll(iSymbol, out);
			}
			break;
		}
	}
	else	
	{
		// for other kind of expressions, generate checks for this index
		computeOutSetIndexAll(iSymbol, out);
	}
}

/*******************************************************************
* Function:	ArrayIndexAnalysis::computeOutSetIndex
* Purpose : A helper function to compute the out set by updating 
* 		  : checks for all array accesses that use the given index
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
void ArrayIndexAnalysis::computeOutSetIndex(const SymbolExpr* iSymbol, 
		const BinaryOpExpr* bOpExpr, FlowSet& out, const FlowFact::ConstraintType constraint)
{
	// induction variable class or a previously cached definition
	const SymbolExpr* ivClass;
	IndexKeySet iKeySet;
			
	if (isBasicIV(iSymbol, bOpExpr))   
	{	
		// delete all catched expressions that depend on this variable.
		deleteCachedExpr(iSymbol);
		
		IndexKeySet iKeySet;
		getAllIndexKeys(iSymbol, iKeySet);
		
		for (IndexKeySet::const_iterator iKeySetIt = iKeySet.begin(), iEnd = iKeySet.end();
			 iKeySetIt != iEnd; ++iKeySetIt)
		{
			// if it is an index variable, then at least a matrix exists, get the set
			Expression::SymbolSet mSet = indexMatMap[*iKeySetIt];
			
			// iterate over the set
			for (Expression::SymbolSet::const_iterator iter = mSet.begin(), iterEnd = mSet.end(); iter != iterEnd; ++iter)
			{	
				// insert a bound check ...
				out.insert( FlowFact(*iter, *iKeySetIt, constraint) );
			}
		}
	}
	else if (searchCachedExpr(bOpExpr, ivClass))  	// index is not a basic induction var, has the expr been cached?
	{	
		// get all <index, dimension> pairs for this symbol
		getAllIndexKeys(iSymbol, iKeySet);			
		
		// initialize the pair for the class of the dependent iv
		IndexKey eqKey = {ivClass, 0};				
		for (IndexKeySet::const_iterator iKeySetIt = iKeySet.begin(), iEnd = iKeySet.end();
			 iKeySetIt != iEnd; ++iKeySetIt)
		{
			// current <index, dimension> pair
			IndexKey iKey = *iKeySetIt;
							
			// update get the required dimension
			eqKey.indexDim = iKey.indexDim;
				
			// if it is an index variable, then at least a matrix exists, get the set
			Expression::SymbolSet mSet = indexMatMap[iKey];
			for (Expression::SymbolSet::const_iterator iter = mSet.begin(), itEnd = mSet.end();
				 iter != itEnd; ++iter)
			{	
				const SymbolExpr* mSymbol = *iter;
				for (int i=0; i < 2; ++i)
				{
					FlowFact::ConstraintType cons = static_cast<FlowFact::ConstraintType>(i);
					if ( out.find(FlowFact(mSymbol, eqKey, cons)) != out.end() )
					{
						out.insert( FlowFact(mSymbol, iKey, cons) );
					}
					else
					{
						out.erase( FlowFact(mSymbol, iKey, cons) );
					}
				}
			}
		}
	}
	else if (isDependentExpr(bOpExpr, ivClass)) //  the expression has not  been cached; does it have a basic iv?
	{	
		symbExprMap[iSymbol] = bOpExpr; // cache it
		IndexKey cKey = {ivClass, 0};
		getAllIndexKeys(iSymbol, iKeySet);		
		for (IndexKeySet::const_iterator iKeySetIt = iKeySet.begin(), iEnd = iKeySet.end();
			 iKeySetIt != iEnd; ++iKeySetIt)
		{
			// an <index, dimension> pair
			IndexKey iKey = *iKeySetIt;
			
			// update get the required dimension
			cKey.indexDim = iKey.indexDim;
				
			// if it is an index variable, then at least a matrix exists, get the set
			Expression::SymbolSet mSet = indexMatMap[iKey];
			
			// not a basic induction variable, hence add checks for both constraints for all arrays
			for (Expression::SymbolSet::const_iterator iter = mSet.begin(), itEnd = mSet.end(); iter != itEnd; ++iter)
			{	
				const SymbolExpr* mSymbol = *iter;
				FlowFact::ConstraintType otherCons = static_cast<FlowFact::ConstraintType> ((constraint + 1) % 2);
				
				if ( out.find(FlowFact(mSymbol, cKey, otherCons)) != out.end() )
				{
					out.insert( FlowFact(mSymbol, iKey, FlowFact::LOWER_BOUND) );
					out.insert( FlowFact(mSymbol, iKey, FlowFact::UPPER_BOUND) );
				}
				else
				{
					out.erase( FlowFact(mSymbol, iKey, otherCons) );
					out.insert( FlowFact(mSymbol, iKey, constraint) );
				}
			}
		}
	}
	else
	{
		// delete all catched expressions that depend on this variable.
		deleteCachedExpr(iSymbol);
			
		// not an induction variable, hence add checks for both contraints for all arrays
		computeOutSetIndexAll(iSymbol, out);
	}
}

/*******************************************************************
* Function:	ArrayIndexAnalysis::computeOutSetIndexAll
* Purpose : A helper function to compute the out set by updating 
* 		  : checks for all array accesses that use the given index
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
void ArrayIndexAnalysis::computeOutSetIndexAll(const SymbolExpr* iSymbol, FlowSet& out)
{	
	// delete all catched expressions that depend on this index symbol.
	deleteCachedExpr(iSymbol);
	
	// declare a set of <index, dimension> pairs
	IndexKeySet iKeySet;
	
	// get all the pairs for this index symbol
	getAllIndexKeys(iSymbol, iKeySet);		   
	for (IndexKeySet::const_iterator iKeySetIt = iKeySet.begin(), iEnd = iKeySet.end(); 
		 iKeySetIt != iEnd; ++iKeySetIt)
	{
	    const IndexKey& iKey = *iKeySetIt;
		// if it is an index variable, then at least a matrix exists, get the set
		Expression::SymbolSet mSet = indexMatMap[iKey];
		
		// not a basic induction variable, hence add checks for both contraints for all arrays
		for (Expression::SymbolSet::const_iterator iter = mSet.begin(), 
			   itEnd = mSet.end();  iter != itEnd; ++iter)
		{	
	        const SymbolExpr* arrayVar = *iter;
			// insert upper bound check ...
			out.insert( FlowFact(arrayVar, iKey, FlowFact::UPPER_BOUND) );
			
			// insert upper bound check ...
			out.insert( FlowFact(arrayVar, iKey, FlowFact::LOWER_BOUND) );
		}		
	}
}

/*******************************************************************
* Function:	ArrayIndexAnalysis::isBasicIV
* Purpose : Test whether a symbol is a basic induction variable
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
inline bool ArrayIndexAnalysis::isBasicIV(const SymbolExpr* iSymbol, const BinaryOpExpr* bOpExpr) const
{
	// get the lhs and rhs of the expression
	const Expression* lOp = bOpExpr->getLeftExpr();
	const Expression* rOp = bOpExpr->getRightExpr();
	
	// check whether the index symbol is a basic induction variable
	// i.e statement of the form i = i op c(constant); ideally op belongs to {+, -, *, /}
	return  (lOp == iSymbol || rOp == iSymbol) && 
	( (lOp->getExprType() == Expression::INT_CONST || rOp->getExprType() == Expression::INT_CONST ) );
}

/*******************************************************************************
* Function:	ArrayIndexAnalysis::isDependentExpr
* Purpose : Test whether an expression has a an induction variable and a constant
* Initial : Nurudeen A. Lameed on July 21, 2009
*********************************************************************************
Revisions and bug fixes:
*/
inline bool ArrayIndexAnalysis::isDependentExpr(const BinaryOpExpr* bOpExpr, const SymbolExpr*& ivClass) const
{
	// get the lhs and rhs of the expression
	const Expression* lOp = bOpExpr->getLeftExpr();
	const Expression* rOp = bOpExpr->getRightExpr();
	
	// check whether expression of the form i op c; e.g i + 1; j + 5; j - 1; it involves an iv
	ivClass = dynamic_cast<const SymbolExpr*>(lOp->getExprType() == Expression::SYMBOL ? lOp : rOp);
	const IntConstExpr* ivConst = 
		dynamic_cast<const IntConstExpr*>( lOp->getExprType() == Expression::INT_CONST ? lOp : rOp );
	
	return (ivClass != 0 && ivConst != 0);
}

/*******************************************************************
* Function:	ArrayIndexAnalysis::ArrayIndexAnalysis
* Purpose : Constructor
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
ArrayIndexAnalysis::ArrayIndexAnalysis(const StmtSequence* pFuncBody, const TypeInferInfo* pTypeInferInfo)
:typeInferenceInfo(pTypeInferInfo)
{	
	assert(pTypeInferInfo != 0);
}

/*******************************************************************
* Function:	ArrayIndexAnalysis::~ArrayIndexAnalysis
* Purpose : Destructor
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
ArrayIndexAnalysis::~ArrayIndexAnalysis()
{
}

/*******************************************************************
* Function:	computeBoundsCheck
* Purpose : Performs array bounds check elimination
* Initial : Nurudeen A. Lameed on July 21, 2009
********************************************************************
Revisions and bug fixes:
*/
AnalysisInfo* computeBoundsCheck(
	const ProgFunction* pFunction,
	const StmtSequence* pFuncBody,
	const TypeSetString& inArgTypes,
	bool returnBottom	
)
{
	// Perform a type inference analysis on the function body
	const TypeInferInfo* pTypeInferInfo = (const TypeInferInfo*)AnalysisManager::requestInfo(
		&computeTypeInfo,
		pFunction,
		pFuncBody,
		inArgTypes
	);
	
	// create an object to store the output
	BoundsCheckInfo* bcInfo = new BoundsCheckInfo();
	
	// create a handle for the analysis object ... 
	ArrayIndexAnalysis aia(pFuncBody, pTypeInferInfo);
	
	// perform the analysis and collect the results
	aia.doAnalysis(pFuncBody, bcInfo->flowSetMap);
	bcInfo->boundsCheckMap = aia.getFlowAnalysisResult();
	
	// return the result
	return bcInfo;
}
