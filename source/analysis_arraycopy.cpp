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

#include <iostream>
#include <sstream>
#include <iterator>
#include "interpreter.h"
#include "analysis_arraycopy.h"
#include "analysis_copyplacement.h"
#include "cellindexexpr.h"

std::set<std::string> ArrayCopyAnalysis::allocFuncs;
FlowContextMap ArrayCopyAnalysis::flowContextMap;
FlowContext ArrayCopyAnalysis::flowContextGenerator = 1;

/*******************************************************************
* Function:	FlowEntry::operator<<()
* Purpose : print a flow entry object
* Initial : Nurudeen A. Lameed on December 26, 2009
********************************************************************
Revisions and bug fixes:
*/
std::ostream&  operator<<(std::ostream& out, const FlowEntry& entry)
{	
	out <<  "(" << (entry.arrayVar ? entry.arrayVar->toString(): "Dummy") 
		<< "," <<  (entry.allocator ? entry.allocator->toString(): "Param") 
		<< "," << entry.context << ")" ;
	
	return out;
}

/*******************************************************************
* Function:	operator<<()
* Purpose : print a CopyFlowSet object
* Initial : Nurudeen A. Lameed on Sept 26, 2009
********************************************************************
Revisions and bug fixes:
*/
std::ostream&  operator<<(std::ostream& out, const CopyFlowSet& flowSet)
{
	out << " {";
	std::copy(flowSet.begin(), flowSet.end(), 
			std::ostream_iterator<FlowEntry>(out, ","));
  	out << "}";
   	
	return out;
}

/*******************************************************************
* Function:	CopyFlowSetMap::operator<<()
* Purpose : print a CopyFlowSetMap object
* Initial : Nurudeen A. Lameed on December 26, 2009
********************************************************************
Revisions and bug fixes:
*/
std::ostream&  operator<<(std::ostream& out, const CopyFlowSetMap& result)
{
	out << "\n===============Assignment Copy Analysis's Result===============" << std::endl;
	for (CopyFlowSetMap::const_iterator iter = result.begin(), iEnd = result.end(); iter != iEnd; ++iter)
	{
		const AssignStmt* aStmt = dynamic_cast<const AssignStmt*> (iter->first);
		if (!aStmt) continue; // print only assignment statements
		
		const FlowInfo& flowInfo = iter->second;	
		out << "\n" << (iter->first ? iter->first->toString(): "Err") << "\t ::";
		CopyFlowSetVec ins = flowInfo.inSetVec;
		std::copy(ins.begin(), ins.end(), std::ostream_iterator<CopyFlowSet>(out, ";"));
			
		out << "\nGEN:: ";
		CopyFlowSetVec gen = flowInfo.genSetVec;
		std::copy(gen.begin(), gen.end(), std::ostream_iterator<CopyFlowSet>(out, ";"));
			
		out << "\n";
	}
	
	return out;
}

/*******************************************************************
* Function:	ArrayCopyAnalysis::doAnalysis()
* Purpose : Perform array copy analysis on a program function
* Initial : Nurudeen A. Lameed on December 26, 2009
********************************************************************
Revisions and bug fixes:
*/
AnalysisResult ArrayCopyAnalysis::doAnalysis(const ProgFunction* pFunction,
											 const TypeSetString& inArgTypes,
											 bool returnTop)
{
	// std::cout << "performing analysis on " << pFunction->getCurrentBody()->toString() << std::endl;
	AnalysisResult result;

	// get in/out Parameters 
	const ProgFunction::ParamVector& inParams =   pFunction->getInParams();	
	const ProgFunction::ParamVector& outParams = pFunction->getOutParams();

	result.first = SummaryInfo(outParams.size());
	SummaryInfo& sumInfo = result.first;

	if (returnTop)
	{
	  getTop(sumInfo, inParams.size(), outParams.size());
	  return result;
	}

	if (allocFuncs.empty())
	{
		initAllocFuncs();
	}
	
	const LiveVarInfo* liveVarInfo = (const LiveVarInfo*)AnalysisManager::requestInfo(
					&computeLiveVars, pFunction, pFunction->getCurrentBody(), inArgTypes);
	const TypeInferInfo* pTypeInferInfo = (const TypeInferInfo*)AnalysisManager::requestInfo(
					&computeTypeInfo, pFunction, pFunction->getCurrentBody(), inArgTypes);	

	// type inference and live variable analyses required
	assert(pTypeInferInfo != 0);
	assert(liveVarInfo != 0);

	LiveVarMap liveVarMap = liveVarInfo->liveVarMap;
	
	CopyFlowSet startSet;
	StmtSequence::StmtVector paramAllocVec;
	Expression::SymbolSet shadowParams;
	initializeStartSet(inParams, outParams, (ProgFunction*)pFunction, 
					   startSet, shadowParams, paramAllocVec);	
	
	CopyFlowSetMap& analysisInfo = result.second;
	CopyFlowSetMap& loopIterInfo = result.third;

	CopyFlowSet exitSet, retSet, breakSet, contSet;
	
	const Environment* const pEnv = ProgFunction::getLocalEnv(pFunction);

	FlowContextPair fcPair(0,0);	
	// get the current body and perform analysis
	doAnalysis(pFunction->getCurrentBody(), pTypeInferInfo, liveVarMap, startSet, exitSet, 
			   retSet, breakSet, contSet, analysisInfo, loopIterInfo, shadowParams, pEnv, fcPair);

	result.fourth = flowContextMap; 
	makeSummary(exitSet, outParams, paramAllocVec, sumInfo);

	//	std::cout << "Forward analysis completed" << std::endl;
	//std::cout << analysisInfo << std::endl;
	
	return  result;
}

// get flow context
const FlowContextPair& ArrayCopyAnalysis::getFlowContext(const LoopStmt* pLoopStmt)
{
  FlowContextMap::const_iterator it = flowContextMap.find(pLoopStmt);
  if (it != flowContextMap.end())
	{
	  return it->second;
	}
  else
	{
	  FlowContext curContext = flowContextGenerator++;
	  FlowContextPair fcPair(curContext, flowContextGenerator++);
	  flowContextMap[pLoopStmt]= fcPair;
	  //std::cout << "FlowContext: (" << fcPair.first << "," << fcPair.second << ")\n";
	  return flowContextMap[pLoopStmt];
	}
}


// generate the analysis' summary information
void ArrayCopyAnalysis::makeSummary(const CopyFlowSet& exitSet,
									const ProgFunction::ParamVector& outParams,
									const StmtSequence::StmtVector& paramAllocVec,
									SummaryInfo& sumInfo)
{
	
	for( size_t j = 0, size = outParams.size(); j < size; ++j)
	{
		const SymbolExpr* const &  retSymbol = outParams[j];  
		CopyFlowSet defs;
		getAllDefs(exitSet, retSymbol, defs);	// get all defs for the return symbol
		IndexSet pointsToSet; 
		for(CopyFlowSet::const_iterator dIter = defs.begin(), dEnd = defs.end(); dIter != dEnd; ++dIter)
		{
			const FlowEntry& fe = *dIter;
			const AssignStmt* pAssignStmt = fe.allocator;
			StmtSequence::StmtVector::const_iterator fIter = 
						std::find(paramAllocVec.begin(), paramAllocVec.end(), pAssignStmt);
			if (fIter == paramAllocVec.end()) continue;  // skip this

			size_t index = fIter - paramAllocVec.begin();
			pointsToSet.insert(index);
		}
		
		sumInfo[j] = pointsToSet;
	}
}

// generate the initial start set
CopyFlowSet& ArrayCopyAnalysis::initializeStartSet(const ProgFunction::ParamVector& inParams, 
												   const ProgFunction::ParamVector& outParams,
												   ProgFunction* pFunction, CopyFlowSet& startSet,
												   Expression::SymbolSet& shadowParams,
												   StmtSequence::StmtVector& paramAllocVec)
{
	shadowParams.insert(outParams.begin(), outParams.end());
	for(size_t i=0, size = inParams.size(); i < size; ++i)
	{
		AssignStmt* allocator = new AssignStmt(inParams[i], inParams[i]);
		paramAllocVec.push_back(allocator);
		startSet.insert(FlowEntry(inParams[i], allocator));
		
		// get a shadow entry
		SymbolExpr* shadowParam = pFunction->createTemp();
		startSet.insert(FlowEntry(shadowParam, allocator));
		shadowParams.insert(shadowParam);
	}

	return startSet;
}

/*******************************************************************
* Function:	ArrayCopyAnalysis::doAnalysis()
* Purpose : Perform array copy analysis on a statement sequence
* Initial : Nurudeen A. Lameed on December 26, 2009
********************************************************************
Revisions and bug fixes:
*/
void ArrayCopyAnalysis::doAnalysis(const StmtSequence* pStmtSeq,
								   const TypeInferInfo* pTypeInferInfo,
								   const LiveVarMap& liveVarMap,
								   const CopyFlowSet& startSet,
								   CopyFlowSet& exitSet, CopyFlowSet& retSet,
								   CopyFlowSet& breakSet, CopyFlowSet& contSet,
								   CopyFlowSetMap& analysisInfo,
								   CopyFlowSetMap& loopIterInfo,
								   const Expression::SymbolSet& shadowParams,
								   const Environment* const pEnv,
								   const FlowContextPair& fcPair)
{
	const StmtSequence::StmtVector& stmts = pStmtSeq->getStatements();
	CopyFlowSet curSet = startSet;
	CopyFlowSetVec copyFlowSetVec(1, curSet);
	
	analysisInfo[pStmtSeq] =  FlowInfo(CopyFlowSetVec(), copyFlowSetVec, CopyFlowSetVec());
	
	// look for assignment statements
	for (StmtSequence::StmtVector::const_iterator it = stmts.begin(), iEnd = stmts.end(); it != iEnd; ++it)
	{
		const Statement* pStmt = *it;
		switch( pStmt->getStmtType() )
		{
			case Statement::ASSIGN:
			{	
				CopyFlowSet exitSet;
				const AssignStmt* aStmt = dynamic_cast<const AssignStmt*>(pStmt);
				Expression::ExprVector lvalues = aStmt->getLeftExprs();
				size_t size = lvalues.size();
				CopyFlowSetVec inSetVec(size);			   
				CopyFlowSetVec genSetVec(size);
				CopyFlowSetVec copySetVec(size);
				Expression* rhs = aStmt->getRightExpr();
				if (size > 1)		// multiple assignments statement?
				{	
					Expression::ExprType rExprType = rhs->getExprType();
					if (rExprType == Expression::CELL_INDEX)
					{
						analyzeMultipleAssignCell(aStmt, lvalues, dynamic_cast<const CellIndexExpr*>(rhs), 
												  liveVarMap, curSet, exitSet, inSetVec, genSetVec, 
												  copySetVec, shadowParams, pEnv, fcPair);
					}
					else if (rExprType == Expression::PARAM) // function call?
					{
						analyzeMultipleAssignFunc(pTypeInferInfo, aStmt, lvalues, 
												  dynamic_cast<const ParamExpr*>(rhs), liveVarMap, curSet,
												  exitSet,inSetVec, genSetVec, copySetVec, 
												  shadowParams, pEnv, fcPair);
					}
				}
				else	// a single assignment
				{
					inSetVec[0] = curSet; 
					CopyFlowSet genSet, copySet;
					analyzeSingleAssign(pTypeInferInfo, aStmt, lvalues[0], rhs, liveVarMap, curSet, exitSet,
										genSet, copySet, shadowParams, pEnv, fcPair);
					genSetVec[0] = genSet;
					copySetVec[0] = copySet;
				}
				
				curSet = exitSet;
				analysisInfo[pStmt] = FlowInfo(genSetVec, inSetVec, copySetVec);
			}
			break;
			
			case Statement::IF_ELSE:
			{
				CopyFlowSetVec inSetVec(1, curSet);
				analysisInfo[pStmt] = FlowInfo(CopyFlowSetVec(), inSetVec, CopyFlowSetVec());
				CopyFlowSet exitSet;
				CopyFlowSetMap ifInfo;
				
				// do analysis for an if else statement
				doAnalysis(dynamic_cast<const IfElseStmt*>(pStmt), pTypeInferInfo, liveVarMap,
						   curSet, exitSet, retSet, breakSet, contSet, ifInfo, loopIterInfo, shadowParams,
						   pEnv, fcPair);
				analysisInfo.insert(ifInfo.begin(), ifInfo.end());
				curSet = exitSet;
			}
			break;
			
			case Statement::LOOP:	// handles  while, for, loop
			{
				CopyFlowSetVec inSetVec(1, curSet);
				analysisInfo[pStmt] = FlowInfo(CopyFlowSetVec(), inSetVec, CopyFlowSetVec());
				CopyFlowSet exitSet;
				CopyFlowSetMap loopInfo;
				
				const LoopStmt* pLoopStmt = dynamic_cast<const LoopStmt*>(pStmt);
				FlowContextPair fcPair = getFlowContext(pLoopStmt);
				
				// do analysis for a loop
				doAnalysis(pLoopStmt, pTypeInferInfo, liveVarMap, curSet, exitSet, retSet, 
						   loopInfo, loopIterInfo, shadowParams, pEnv, fcPair);
				analysisInfo.insert(loopInfo.begin(), loopInfo.end());
				curSet = exitSet;
			}
			break;
			
			case Statement::CONTINUE:
			{
				contSet.insert(curSet.begin(), curSet.end());
			}
			break;
			
			case Statement::BREAK:
			{
				breakSet.insert(curSet.begin(), curSet.end());
			}
			break;
			
			case Statement::RETURN:
			{
				retSet.insert(curSet.begin(), curSet.end());
			}
			break;
			
			default: 
			{	
	            CopyFlowSet temp(curSet);
				// get the set of live variables
				LiveVarMap::const_iterator liveVarIt = liveVarMap.find(pStmt);
				assert(liveVarIt != liveVarMap.end());
				Expression::SymbolSet liveVars = liveVarIt->second;				
			    rmNonLiveVars(liveVars, temp, curSet, shadowParams);
				CopyFlowSetVec inSetVec(1, curSet);
				analysisInfo[pStmt] = FlowInfo(CopyFlowSetVec(), inSetVec, CopyFlowSetVec());
			}
		}
		
	}

	exitSet = curSet;
}

/*******************************************************************
* Function:	ArrayCopyAnalysis::flowFunction()
* Purpose : compute outset from inset for a statement
* Initial : Nurudeen A. Lameed on December 26, 2009
********************************************************************
Revisions and bug fixes:
*/
void ArrayCopyAnalysis::analyzeMultipleAssignCell(const AssignStmt* pStmt,
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
												  const FlowContextPair& fcPair)
{
	// get the set of live variables
	LiveVarMap::const_iterator liveVarIt = liveVarMap.find(pStmt);
	assert(liveVarIt != liveVarMap.end());
	Expression::SymbolSet liveVars = liveVarIt->second;
	
	exitSet = startSet;
	
	// remove all variables that are not live from tmp
	rmNonLiveVars(liveVars, startSet, exitSet, shadowParams);
	const SymbolExpr* rArrayVar = getSymFromArrayLValue(rhs, Expression::CELL_INDEX);
	CopyFlowSet curStartSet = startSet;	// start with live variables in the set
	CopyFlowSet curExitSet = exitSet;
	
	for ( size_t i= 0, size = lvalues.size(); i < size; ++i)
	{	
		inSetVec[i] = curStartSet; 
		CopyFlowSet genSet, copySet;
		Expression::ExprType lhsType = lvalues[i]->getExprType();
		if (isArrayLValue(lhsType))
		{
			const SymbolExpr* lArrayVar = getSymFromArrayLValue(lvalues[i], lhsType);
			removeAllDefs(exitSet, lArrayVar); // kill all the previous defs
			genEntryFromRHS(lArrayVar, rArrayVar, liveVars, curStartSet, genSet, exitSet, 
							shadowParams, fcPair);
		}
		else if (lhsType == Expression::PARAM)
		{
		  genCopyGenerator(liveVars, shadowParams, pStmt, lvalues[i], startSet, exitSet, 
							 genSet, copySet, fcPair);
		}
		
		genSetVec[i] = genSet;
		copySetVec[i] = copySet;
		exitSet.insert(genSet.begin(), genSet.end());
		
		// update the start set
		curStartSet = exitSet;
	}
}

// multiple definitions from a func
void ArrayCopyAnalysis::analyzeMultipleAssignFunc(const TypeInferInfo* pTypeInferInfo,
												  const AssignStmt* pStmt, 
												  Expression::ExprVector& lvalues,
												  const ParamExpr* rhs, const LiveVarMap& liveVarMap,
												  const CopyFlowSet& startSet, CopyFlowSet& exitSet,
												  CopyFlowSetVec& inSetVec, CopyFlowSetVec& genSetVec,
												  CopyFlowSetVec& copySetVec,
												  const Expression::SymbolSet& shadowParams,
												  const Environment* const pEnv,
												  const FlowContextPair& fcPair)
{
	// get the set of live variables
	LiveVarMap::const_iterator liveVarIt = liveVarMap.find(pStmt);
	assert(liveVarIt != liveVarMap.end());
	Expression::SymbolSet liveVars = liveVarIt->second;
	
	exitSet = startSet;
	// remove all variables that are not live from tmp
	rmNonLiveVars(liveVars, startSet, exitSet, shadowParams);
	if (hasArrayVarParam(rhs))
	{
		SummaryInfo funcSumInfo;
		CopyFlowSet funcGenSet;
		const Function* pFunc = getSummaryInfo(pTypeInferInfo, startSet, pStmt, 
											   rhs, funcSumInfo, funcGenSet, pEnv);
	   
		if (pFunc)  // right handside is a function ...
		{	
			if (pFunc->isProgFunction())	// rhs is a program function
			{
				CopyFlowSet curStartSet = startSet;	// start with live variables in the set
				CopyFlowSet curExitSet = exitSet;
				const Expression::ExprVector& funcArgs = rhs->getArguments();
				
				for ( size_t i = 0, size = lvalues.size(); i < size; ++i)
				{	
					bool isNotAnAlias = true;
					inSetVec[i] = curStartSet;	
					CopyFlowSet genSet, copySet;
					Expression::ExprType lhsType = lvalues[i]->getExprType();
					if (isArrayLValue(lhsType))
					{
						const SymbolExpr* lSymbol =  getSymFromArrayLValue(lvalues[i], lhsType);
						removeAllDefs(exitSet, lSymbol); // kill all the previous defs
						const IndexSet& refSet = funcSumInfo[i];
						for(IndexSet::const_iterator it = refSet.begin(), iEnd = refSet.end(); it != iEnd; ++it)
						{
							size_t argIndex = *it;
							Expression::ExprType argExprType = funcArgs[argIndex]->getExprType();
							if (!isArrayLValue(argExprType)) continue; //skip it

							isNotAnAlias = false;
							const SymbolExpr* argSymbol =  getSymFromArrayLValue(funcArgs[argIndex], argExprType);
							genEntryFromRHS(lSymbol, argSymbol, liveVars, curStartSet, genSet, exitSet, 
											shadowParams, fcPair);
						}
						
						if (isNotAnAlias)
						{
							// a new allocator
							insertFlowEntry(liveVars, genSet, lSymbol, pStmt, shadowParams,fcPair.first);
						}
					}
					else if (lhsType == Expression::PARAM)
					{
					  genCopyGenerator(liveVars, shadowParams, pStmt, lvalues[i], startSet, exitSet, genSet, copySet, fcPair);
					}
					
					genSetVec[i] = genSet;
					copySetVec[i] = copySet;
					exitSet.insert(genSet.begin(), genSet.end());
					
					// update the start set
					curStartSet = exitSet;
				}
			}
			else
			{
				CopyFlowSet curStartSet = startSet;
				for (size_t i= 0, size = lvalues.size(); i < size; ++i)
				{
					inSetVec[i] = curStartSet; 
					CopyFlowSet genSet, copySet;
					Expression::ExprType lhsType = lvalues[i]->getExprType();
					if (isArrayLValue(lhsType))
					{						
						const SymbolExpr* lSymbol =  getSymFromArrayLValue(lvalues[i], lhsType);
						for (CopyFlowSet::const_iterator it = funcGenSet.begin(), iEnd = funcGenSet.end(); it != iEnd; ++it)
						{
							insertFlowEntry(liveVars, genSet, lSymbol, (*it).allocator, 
											shadowParams, fcPair.first);
						}
					}
					else if (lhsType == Expression::PARAM)
					{
					  genCopyGenerator(liveVars, shadowParams, pStmt, lvalues[i], startSet, 
									   exitSet, genSet, copySet, fcPair);
					}
					
					genSetVec[i] = genSet;
					copySetVec[i] = copySet;
					exitSet.insert(genSet.begin(), genSet.end());
									
					// update the start set
					curStartSet = exitSet;
				}
			}
		}
		else	// what is it?
		{
			//std::cout << "Not a function?: " << pStmt->toString() << std::endl;
			assert(false && "object type is unknown");
		}
	}
	else
	{
		CopyFlowSet curStartSet = startSet;
		for (size_t i= 0, size = lvalues.size(); i < size; ++i)
		{
			inSetVec[i] = curStartSet; 
			CopyFlowSet genSet;
			Expression::ExprType lhsType = lvalues[i]->getExprType();
			if (isArrayLValue(lhsType))
			{
				const SymbolExpr* lArrayVar =  getSymFromArrayLValue(lvalues[i], lhsType);
				insertFlowEntry(liveVars, genSet, lArrayVar, pStmt, shadowParams, fcPair.first);
			}
			genSetVec[i] = genSet;
			copySetVec[i] = CopyFlowSet();
			exitSet.insert(genSet.begin(), genSet.end());
							
			// update the start set
			curStartSet = exitSet;
		}
	}
}

bool ArrayCopyAnalysis::hasArrayVarParam(const ParamExpr* pParamExpr)
{
  const ParamExpr::ExprVector& args = pParamExpr->getArguments();
  for (size_t i = 0, size = args.size(); i < size; ++i)
	{
		if (!isArrayLValue(args[i]->getExprType())) continue;

		return true;
	}
	
	return false;
}


void ArrayCopyAnalysis::analyzeSingleAssign(const TypeInferInfo* pTypeInferInfo,
											const AssignStmt* pStmt, const Expression* lhs,
											const Expression* rhs, const LiveVarMap& liveVarMap,
											const CopyFlowSet& startSet, CopyFlowSet& exitSet,
											CopyFlowSet& genSet, CopyFlowSet& copySet,
											const Expression::SymbolSet& shadowParams,
											const Environment* const pEnv,
											const FlowContextPair& fcPair)
{
	exitSet = startSet;
	
	// get the set of live variables
	LiveVarMap::const_iterator liveVarIt = liveVarMap.find(pStmt);
	assert(liveVarIt != liveVarMap.end());
	Expression::SymbolSet liveVars = liveVarIt->second;	
	
	// remove all variables that are not live from exitSet
	rmNonLiveVars(liveVars, startSet, exitSet, shadowParams);
	
	if (lhs != rhs) // ignore the trivial case; a = a
  	{ 
		Expression::ExprType lhsType = lhs->getExprType();
		if (isArrayLValue(lhsType))
		{
			const SymbolExpr* lArrayVar = getSymFromArrayLValue(lhs, lhsType);
			
			// kill all the previous defs
			removeAllDefs(exitSet, lArrayVar);
			
			switch(rhs->getExprType())
			{
				case Expression::SYMBOL:		// a = b || a = c{1} || c{1} = b{1}
				case Expression::CELL_INDEX:
				{	
					const SymbolExpr* rArrayVar = getSymFromArrayLValue(rhs, rhs->getExprType());
					genEntryFromRHS(lArrayVar, rArrayVar, liveVars, startSet, genSet, 
									exitSet, shadowParams, fcPair);
				}
				break;
				
				case Expression::MATRIX:		// a = [x1, x2, x3; ...]
				case Expression::RANGE:			// a = 1:1:10 ...
				{	
					// generate a new malloc site
					insertFlowEntry(liveVars, genSet, lArrayVar, pStmt, shadowParams, fcPair.first);
				}
				break;
				
				case Expression::CELLARRAY:
				{
					const CellArrayExpr* cellArrayExpr = dynamic_cast<const CellArrayExpr*>(rhs);
					Expression::ExprVector cells = cellArrayExpr->getSubExprs();
					for (size_t i=0, size = cells.size(); i < size; ++i)
					{
						Expression::ExprType rExprType = cells[i]->getExprType();
						if(!isArrayLValue(rExprType)) continue;

						// generate a new allocation  site for the cell
						const SymbolExpr* rArrayVar = getSymFromArrayLValue(cells[i], rExprType);
						genEntryFromRHS(lArrayVar, rArrayVar, liveVars, startSet, genSet, exitSet, 
											shadowParams, fcPair);
					}
					 
					// insert an entry for the cell array
					insertFlowEntry(liveVars, genSet, lArrayVar, pStmt, shadowParams, fcPair.first);
				}
				break;
				
				case Expression::PARAM:
				{
					const ParamExpr* pParamExpr = dynamic_cast<const ParamExpr*>(rhs);
					if (hasArrayVarParam(pParamExpr))
					{
						SummaryInfo funcSumInfo;
						CopyFlowSet funcGenSet;
						
						// get the copy analysis summary info for the function
						const Function* pFunction = getSummaryInfo(pTypeInferInfo, startSet, pStmt, 
																   pParamExpr, funcSumInfo, funcGenSet, pEnv);
						if(pFunction)		// if it is a function
						{
							if (pFunction->isProgFunction())
							{
								const Expression::ExprVector& funcArgs = pParamExpr->getArguments();
								bool isNotAnAlias = true;
								const IndexSet& refSet = funcSumInfo[0];
								for(IndexSet::const_iterator it = refSet.begin(), iEnd = refSet.end(); it != iEnd; ++it)
								{
									size_t argIndex = *it;
									Expression::ExprType argExprType = funcArgs[argIndex]->getExprType();
									if (!isArrayLValue(argExprType)) continue;

									isNotAnAlias = false;
									const SymbolExpr* argSymbol =  getSymFromArrayLValue(funcArgs[argIndex], argExprType);
									genEntryFromRHS(lArrayVar, argSymbol, liveVars, startSet, genSet, exitSet, 
														shadowParams, fcPair);
								}
								if (isNotAnAlias)		// a new object
								{
									// a new allocation site
									insertFlowEntry(liveVars, genSet, lArrayVar, pStmt, shadowParams,
													fcPair.first);
								}
							}
							else	// it is a library function
							{
							  for (CopyFlowSet::const_iterator it = funcGenSet.begin(), iEnd = funcGenSet.end(); it != iEnd; ++it)
								{
								  insertFlowEntry(liveVars, genSet, lArrayVar, (*it).allocator, shadowParams, fcPair.first);
								}
							}
						}
					}
					else		// for now, assume that there are no globals
					{
					  insertFlowEntry(liveVars, genSet, lArrayVar, pStmt, shadowParams, fcPair.first);
					}
				}
				break;
				
				default:
				{
					// do nothing
				}	
			}
		}
		else if (lhsType == Expression::PARAM)	// array or cell write
		{
		  genCopyGenerator(liveVars, shadowParams, pStmt, lhs, startSet, exitSet, genSet, copySet, fcPair);
		}
		
		exitSet.insert(genSet.begin(), genSet.end());
  	}
}

/*******************************************************************
* Function:	ArrayCopyAnalysis::doAnalysis()
* Purpose : Perform array copy analysis on a loop
* Initial : Nurudeen A. Lameed on December 26, 2009
********************************************************************
Revisions and bug fixes:
*/
void ArrayCopyAnalysis::doAnalysis(const LoopStmt* pLoopStmt,
								   const TypeInferInfo* pTypeInferInfo,
								   const LiveVarMap& liveVarMap,
								   const CopyFlowSet& startSet,
								   CopyFlowSet& exitSet, CopyFlowSet& retSet,
								   CopyFlowSetMap& analysisInfo,
								   CopyFlowSetMap& loopIterInfo,
								   const Expression::SymbolSet& shadowParams,
								   const Environment* const pEnv,	
								   const FlowContextPair& fcPair)
{
	CopyFlowSetMap curLoopSetMap;
	CopyFlowSet initExitSet;
	CopyFlowSet initRetSet;
	CopyFlowSet initBreakSet;
	CopyFlowSet initContSet;
	
	// perform flow analysis on the sequence of statements for loop initialization
	CopyFlowSetMap initSetMap;
	doAnalysis(pLoopStmt->getInitSeq(), pTypeInferInfo, liveVarMap, startSet, 
			   initExitSet, initRetSet, initBreakSet, initContSet, initSetMap, 
			   loopIterInfo, shadowParams, pEnv, fcPair);

	CopyFlowSet loopStartSet(initExitSet);
	//makeLoopStartSet(initExitSet, loopStartSet, flowContext);
	
	CopyFlowSet prevLoopStartSet;
	CopyFlowSet curIncrExitSet;
	CopyFlowSetMap testSetMap;
	CopyFlowSetMap bodySetMap;
	CopyFlowSetMap incrSetMap;

	// we need to collect useful information for 
	// detecting backward dependency in a loop
	// in the first iteration of the loop
	size_t i = 1;

	while(true)
	{
		CopyFlowSet testExitSet;
		CopyFlowSet testRetSet;
		CopyFlowSet testBreakSet;
		CopyFlowSet testContSet;
		
		// perform flow analysis on the sequence of statements for the loop test
		doAnalysis(pLoopStmt->getTestSeq(), pTypeInferInfo, liveVarMap, loopStartSet, 
				   testExitSet,testRetSet, testBreakSet, testContSet, testSetMap,
				   loopIterInfo, shadowParams, pEnv, fcPair);
		
		CopyFlowSet bodyExitSet;
		CopyFlowSet breakSet;
		CopyFlowSet contSet;
		
		// perform flow analysis on the loop's body
		doAnalysis(pLoopStmt->getBodySeq(), pTypeInferInfo, liveVarMap, testExitSet, 
				   bodyExitSet, retSet, breakSet, contSet, bodySetMap, loopIterInfo,
				   shadowParams, pEnv, fcPair);
		
		// Merge the test exit set and the break set, (i.e. merge false-loop-test path and break)
		breakSet.insert(testExitSet.begin(), testExitSet.end());
		
		CopyFlowSet incrStartSet(bodyExitSet);
		incrStartSet.insert(contSet.begin(), contSet.end());
		CopyFlowSet incrRetSet;
		CopyFlowSet incrBreakSet;
		CopyFlowSet incrContSet;
		
		// perform flow analysis on the sequence of statements for the loop increment statement
		doAnalysis(pLoopStmt->getIncrSeq(), pTypeInferInfo, liveVarMap, incrStartSet, 
				   curIncrExitSet, incrRetSet, incrBreakSet, incrContSet, incrSetMap,
				   loopIterInfo, shadowParams, pEnv, fcPair);
		
		if (i++ == 1)	// collect in sets (first iteration only)
		{
			loopIterInfo.insert(initSetMap.begin(), initSetMap.end());
			loopIterInfo.insert(testSetMap.begin(), testSetMap.end());
			loopIterInfo.insert(bodySetMap.begin(), bodySetMap.end());
			loopIterInfo.insert(incrSetMap.begin(), incrSetMap.end());
			//			loopStartSet = curIncrExitSet;
		}
		//	else
		//{
			// accumulating results ... for the loop start set 
			// beginning with the second iteration
			loopStartSet.insert(curIncrExitSet.begin(), curIncrExitSet.end()); 
			//}
		
		// is the minimum fixed point reached?
		if (loopStartSet == prevLoopStartSet)
		{
			break;
		}
		
		prevLoopStartSet = loopStartSet;
		exitSet = breakSet;
	}

	exitSet = loopStartSet; // .insert(initExitSet.begin(), initExitSet.end());
	//std::cout << "Loop exit set " << exitSet << "\n";	

	// collect the result
	analysisInfo.insert(initSetMap.begin(), initSetMap.end());
	analysisInfo.insert(testSetMap.begin(), testSetMap.end());
	analysisInfo.insert(bodySetMap.begin(), bodySetMap.end());
	analysisInfo.insert(incrSetMap.begin(), incrSetMap.end());
}

void ArrayCopyAnalysis::makeLoopStartSet(const CopyFlowSet& src, CopyFlowSet& dest, 
										 FlowContext flowContext)
{
  for(CopyFlowSet::const_iterator it = src.begin(), iEnd = src.end(); it != iEnd; ++it)
  {
      const FlowEntry& entry = *it;
      dest.insert(FlowEntry(entry.arrayVar, entry.allocator, flowContext));
  }
}


/*******************************************************************
* Function:	ArrayCopyAnalysis::doAnalysis()
* Purpose : Perform array copy analysis on an ifelse statement
* Initial : Nurudeen A. Lameed on December 26, 2009
********************************************************************
Revisions and bug fixes:
*/
void ArrayCopyAnalysis::doAnalysis(const IfElseStmt* pIfStmt,
								   const TypeInferInfo* pTypeInferInfo,
								   const LiveVarMap& liveVarMap,
								   const CopyFlowSet& startSet,
								   CopyFlowSet& exitSet, CopyFlowSet& retSet,
								   CopyFlowSet& breakSet, CopyFlowSet& contSet,
								   CopyFlowSetMap& analysisInfo,
								   CopyFlowSetMap& loopIterInfo,
								   const Expression::SymbolSet& shadowParams,
								   const Environment* pEnv, 
								   const FlowContextPair& fcPair)
{
	// Get the condition expression
	Expression* pTestExpr = pIfStmt->getCondition();
	
	CopyFlowSetVec inSetVec(1, startSet);
	analysisInfo[pTestExpr] = FlowInfo(CopyFlowSetVec(), inSetVec, CopyFlowSetVec());
	
	CopyFlowSet ifSet;
	CopyFlowSet elseSet;
	CopyFlowSet ifRetSet, ifBreakSet, ifContSet;
	
	// analyze the if block
	CopyFlowSetMap ifSetMap;
	doAnalysis(pIfStmt->getIfBlock(), pTypeInferInfo, liveVarMap, startSet, ifSet, ifRetSet,
			   ifBreakSet, ifContSet, ifSetMap, loopIterInfo, shadowParams, pEnv, fcPair);
	// merge set
	if (ifBreakSet.empty() && ifContSet.empty() && ifRetSet.empty())
	{
		exitSet.insert(ifSet.begin(), ifSet.end());
	}
	else
	{
		retSet.insert(ifRetSet.begin(), ifRetSet.end());
		breakSet.insert(ifBreakSet.begin(), ifBreakSet.end());
		contSet.insert(ifContSet.begin(), ifContSet.end());
	}
	
	CopyFlowSet elseRetSet, elseBreakSet, elseContSet;
	
	// analyze the else block
	CopyFlowSetMap elseSetMap;
	doAnalysis(pIfStmt->getElseBlock(), pTypeInferInfo, liveVarMap, startSet, elseSet, elseRetSet, 
			   elseBreakSet, elseContSet, elseSetMap, loopIterInfo, shadowParams, pEnv, fcPair);
	
	// merge set
	if (elseBreakSet.empty() && elseContSet.empty() && elseRetSet.empty())
	{
		exitSet.insert(elseSet.begin(), elseSet.end());
	}
	else
	{
		retSet.insert(elseRetSet.begin(), elseRetSet.end());
		breakSet.insert(elseBreakSet.begin(), elseBreakSet.end());
		contSet.insert(elseContSet.begin(), elseContSet.end());
	}
	
	// merge map
	analysisInfo.insert(ifSetMap.begin(), ifSetMap.end());
	analysisInfo.insert(elseSetMap.begin(), elseSetMap.end());
}

inline const SymbolExpr* ArrayCopyAnalysis::getSymFromArrayLValue(const Expression* lvalue, 
																  Expression::ExprType exprType)
{
	if (exprType == Expression::SYMBOL)
	{
		return dynamic_cast<const SymbolExpr*>(lvalue);
	}
	else if (exprType == Expression::CELL_INDEX)
	{
		return (dynamic_cast<const CellIndexExpr*>(lvalue))->getSymExpr();
	}
	else
	{
		return 0;
	}
}


void ArrayCopyAnalysis::rmNonLiveVars(const Expression::SymbolSet& liveVars,
									  const CopyFlowSet& startSet, CopyFlowSet& exitSet,
									  const Expression::SymbolSet& shadowParams)
{
  for (CopyFlowSet::const_iterator it = startSet.begin(), iEnd = startSet.end(); it != iEnd; ++it)
  {
		const FlowEntry& fe = *it;
		SymbolExpr* eSymbol = const_cast<SymbolExpr*>(fe.arrayVar);
		if (isLive(eSymbol, liveVars, shadowParams)) continue;

 		exitSet.erase(fe);
   }
}

bool ArrayCopyAnalysis::makeContextSpecial(const AssignStmt* allocator, 
										   CopyFlowSet& exitSet, FlowContext loopPrevContext)
{
  CopyFlowSet temp;
  for(CopyFlowSet::const_iterator it = exitSet.begin(), iEnd = exitSet.end(); it != iEnd; ++it)
  {
	const FlowEntry& fe = *it;
	if (fe.allocator == allocator)
	{
	  temp.insert(fe);
	}
  }

  for(CopyFlowSet::const_iterator it = temp.begin(), iEnd = temp.end(); it != iEnd; ++it)
  {
	const FlowEntry& fe = *it;
	exitSet.erase(fe);
	exitSet.insert(FlowEntry(fe.arrayVar, fe.allocator, loopPrevContext));
  }

  return temp.size() > 0;
}


void ArrayCopyAnalysis::genCopyGenerator(const Expression::SymbolSet& liveVars,
										 const Expression::SymbolSet& shadowParams,
										 const AssignStmt* pStmt, const Expression* pExpr, 
										 const CopyFlowSet& startSet,
										 CopyFlowSet& exitSet,	CopyFlowSet& genSet,
										 CopyFlowSet& copySet,	const FlowContextPair& fcPair)
{ 
	const ParamExpr* pParamExpr = dynamic_cast<const ParamExpr*>(pExpr);
	SymbolExpr* lArrayVar = pParamExpr->getSymExpr();
	bool mustReplaceDefs = false;
	CopyFlowSet defs;
	getAllDefs(exitSet, lArrayVar, defs);

	// is the var dead after the current array update statement?
	// still, we must ensure that a shared array is not updated
	if (defs.empty())	
	{
	   getAllDefs(startSet, lArrayVar, defs);
	}

	// partition the in-set based on allocators
	for (CopyFlowSet::const_iterator it = defs.begin(), iEnd = defs.end(); it != iEnd; ++it)
	{
		const FlowEntry& def = *it;
		if (!isSharedArrayVar(exitSet, def)) continue;	
	
		if (def.allocator == pStmt)  // a cycle found
		{
		  makeContextSpecial(pStmt, exitSet, fcPair.second);
		}		
		
		mustReplaceDefs = true;				
		copySet.insert(def);
	}

	if (mustReplaceDefs)
	{
		removeAllDefs(exitSet, lArrayVar);
		genSet.insert(FlowEntry(lArrayVar, pStmt, fcPair.first));
	}
}

// generate new defs for the lhs from the definitions of the rhs
void ArrayCopyAnalysis::genEntryFromRHS(const SymbolExpr* lArrayVar,
										const SymbolExpr* rArrayVar,
										const Expression::SymbolSet& liveVars,
										const CopyFlowSet& startSet,
										CopyFlowSet& genSet, CopyFlowSet& exitSet,
										const Expression::SymbolSet& shadowParams,
										const FlowContextPair& fcPair)
{
	FinderBySymbol entryFinder(rArrayVar);
	CopyFlowSet::const_iterator iEnd = startSet.end(), def = 
	std::find_if(startSet.begin(), iEnd, entryFinder);
	while (def != iEnd)
	{
		const FlowEntry& fe = *def;
		FlowContext fC = fe.context;
		
		// if we are in a loop and the definition was 
		// generated outside the loop
		if (fe.context != fcPair.first && fe.context != fcPair.second)
		{
			fC = fcPair.first;	// use the loop's context number	
			
			// add a shadow entry for the rhs
			exitSet.insert(FlowEntry(fe.arrayVar, fe.allocator, fC));			
		}
		
		// insert a flow entry for the lhs
		insertFlowEntry(liveVars, genSet, lArrayVar, fe.allocator, shadowParams, fC);
		def = std::find_if(++def, iEnd, entryFinder);
	}
}


/*******************************************************************
* Function:	ArrayCopyAnalysis::genConservInfo()
* Purpose : generate flow info conservatively
* Initial : Nurudeen A. Lameed on December 26, 2009
********************************************************************
Revisions and bug fixes:
*/
void ArrayCopyAnalysis::genConservInfo(const CopyFlowSet& startSet,
									   const ParamExpr* pParamExpr,
									   CopyFlowSet& genSet)
{
	// for now assume that r points to all the params (conservative)
	const Expression::ExprVector& arguments = pParamExpr->getArguments();
	for (size_t i = 0, size = arguments.size(); i < size; ++i)
	{
	  if (arguments[i]->getExprType() != Expression::SYMBOL) continue;

	  SymbolExpr* argSymbol = (SymbolExpr*)arguments[i];								
	  CopyFlowSet defs;
			
	  // get all definitions for the symbol ...; arg must be defined
	  getAllDefs(startSet, argSymbol, defs);
	  for (CopyFlowSet::const_iterator dIter = defs.begin(), dEnd = defs.end(); dIter != dEnd; ++dIter)
	  {
		  genSet.insert(FlowEntry(0, (*dIter).allocator, 0));
	  }
	}
}

/*******************************************************************
* Function:	ArrayCopyAnalysis::initMallocFuncs()
* Purpose : load all malloc functons 
* Initial : Nurudeen A. Lameed on Sept 26, 2009
********************************************************************
Revisions and bug fixes:
*/
void ArrayCopyAnalysis::initAllocFuncs()
{
	// TODO: load from a file
	
	allocFuncs.insert("rand");
	allocFuncs.insert("randn");
	allocFuncs.insert("magic");
	allocFuncs.insert("zeros");
	allocFuncs.insert("ones");
	allocFuncs.insert("unique");
	allocFuncs.insert("toeplitz");
	allocFuncs.insert("diag");
	allocFuncs.insert("eye");
	allocFuncs.insert("reshape");
	allocFuncs.insert("repmat");
}

/*******************************************************************
* Function:	ArrayCopyAnalysis::getAllDefs()
* Purpose : return all unique definitions in a flow set
* Initial : Nurudeen A. Lameed on December 26, 2009
********************************************************************
Revisions and bug fixes:
*/
void ArrayCopyAnalysis::getAllDefs(const CopyFlowSet& flowSet, 
								   const SymbolExpr* symbol, CopyFlowSet& defs)
{
	FinderBySymbol entryFinder(symbol);
	CopyFlowSet::const_iterator iEnd = flowSet.end(), def = 
	std::find_if(flowSet.begin(), iEnd, entryFinder);
	
	// collect all defs, if any 
	while ( def != iEnd )
	{
		defs.insert(*def);
		def = std::find_if(++def, iEnd, entryFinder);
	}
}

/*******************************************************************
* Function:	ArrayCopyAnalysis::removeAllDefs()
* Purpose : remove definitions from a flow set
* Initial : Nurudeen A. Lameed on December 26, 2009
********************************************************************
Revisions and bug fixes:
*/
void ArrayCopyAnalysis::removeAllDefs(CopyFlowSet& flowSet, const SymbolExpr* symbol)
{
	// remove all defitions for symbol from the set
	FinderBySymbol entryFinder(symbol);

	CopyFlowSet::const_iterator def = 
	std::find_if(flowSet.begin(), flowSet.end(), entryFinder);
	while ( def != flowSet.end() )
	{
		flowSet.erase(def);

		// flowSet is mutating, so begin, end change ...
		def = std::find_if(flowSet.begin(), flowSet.end(), entryFinder);
	}
}

/*******************************************************************
* Function:	ArrayCopyAnalysis::getCount()
* Purpose : counts the number of unique definitions for a symbol
* Initial : Nurudeen A. Lameed on December 26, 2009
********************************************************************
Revisions and bug fixes:
*/
bool ArrayCopyAnalysis::isSharedArrayVar(const CopyFlowSet& inSet, const FlowEntry& def)
{
	FinderByStmt entryFinder(def.allocator, def.context);
	CopyFlowSet::const_iterator iEnd = inSet.end(), it = 
	std::find_if(inSet.begin(), iEnd, entryFinder);

	while ( it != iEnd )
	{	
		if ((*it).arrayVar != def.arrayVar) return true;

		it = std::find_if(++it, iEnd, entryFinder);
	}
	
	return false;
}

/*******************************************************************
* Function:	ArrayCopyAnalysis::getCount()
* Purpose : counts the number of unique definitions for a symbol
* Initial : Nurudeen A. Lameed on December 26, 2009
********************************************************************
Revisions and bug fixes:
*/
bool ArrayCopyAnalysis::isSharedArrayVar(const CopyFlowSet& inSet, const FlowEntry& def,
										   Expression::SymbolSet& partMembers)
{
    bool isSharedArray = false;
	FinderByStmt entryFinder(def.allocator, def.context);
	CopyFlowSet::const_iterator iEnd = inSet.end(), it = 
	std::find_if(inSet.begin(), iEnd, entryFinder);

	while ( it != iEnd )
	{	
		const FlowEntry& otherDef = *it;
		if (otherDef.arrayVar != def.arrayVar)	// other def symbol != this symbol
		{
			isSharedArray = true;
			partMembers.insert(const_cast<SymbolExpr*>(otherDef.arrayVar));
		}
		
		it = std::find_if(++it, iEnd, entryFinder);
	}
	
	return isSharedArray;
}

const Function* ArrayCopyAnalysis::getSummaryInfo(const TypeInferInfo* pTypeInferInfo,
												  const CopyFlowSet& startSet, 
												  const AssignStmt* pStmt, const ParamExpr* rhs, 
												  SummaryInfo& funcSumInfo, CopyFlowSet& funcGenSet, 
												  const Environment* const pEnv) 
{
	Function* pFunction = 0;
	const ParamExpr* pParamExpr = dynamic_cast<const ParamExpr*>(rhs);					
	SymbolExpr* pSymbol = pParamExpr->getSymExpr();
	const DataObject* pObject = Environment::lookup(pEnv, pSymbol);
	if (pObject && pObject->getType() == DataObject::FUNCTION)
	{
		pFunction = (Function*)pObject;
		if (pFunction->isProgFunction())
		{
			ProgFunction* pProgFunction = dynamic_cast<ProgFunction*>(pFunction);
			TypeSetString funcInArgTypes;
			getFuncArgTypes(pTypeInferInfo, pStmt, rhs, funcInArgTypes);
			const ArrayCopyAnalysisInfo* pArrayCopyInfo = 
				(const ArrayCopyAnalysisInfo*)AnalysisManager::requestInfo(&ArrayCopyElim::computeArrayCopyElim, 
				pProgFunction, pProgFunction->getCurrentBody(), funcInArgTypes);
			assert(pArrayCopyInfo);
			funcSumInfo = pArrayCopyInfo->summaryInfo; 
		}
		else
		{
			if (isAllocFunc(pFunction->getFuncName()))
			{
				// a new allocation site
				funcGenSet.insert(FlowEntry(0, pStmt, 0));
			}
			else	
			{	
				genConservInfo(startSet, pParamExpr, funcGenSet);
			}
		}
	}
	
	return pFunction;
}


// generate Top
void ArrayCopyAnalysis::getTop(SummaryInfo& sumInfo, size_t ipSize, size_t opSize)
{
  for( size_t j = 0; j < opSize; ++j)
  {
	IndexSet pointsToSet;
	for(size_t k = 0; k < ipSize; ++k)
	{
   		pointsToSet.insert(k);
   	}
   	sumInfo[j] = pointsToSet;
  }
}

void getFuncArgTypes(const TypeInferInfo* pTypeInferInfo, const AssignStmt* pStmt,
					 const ParamExpr* pParamExpr, TypeSetString& inArgTypes)
{
	const TypeInfoMap& typeMap = pTypeInferInfo->preTypeMap;
	TypeInfoMap::const_iterator typeInfoIter = typeMap.find(pStmt);
	assert(typeInfoIter != typeMap.end());
	
	const VarTypeMap& varTypes = typeInfoIter->second;
	const ParamExpr::ExprVector& arguments = pParamExpr->getArguments();
	for (size_t i = 0, size = arguments.size(); i < size; ++i)
	{
		Expression* pArgExpr = arguments[i];
		TypeSet exprTypes;
		if (pArgExpr->getExprType() == Expression::SYMBOL)
		{
			// Get the type set associated with the symbol
			VarTypeMap::const_iterator typeItr = varTypes.find((SymbolExpr*)pArgExpr);
			exprTypes = (typeItr != varTypes.end())? typeItr->second:TypeSet();
			inArgTypes.push_back(exprTypes);
		}
		else
		{
			// Get the type set associated with the expression
			ExprTypeMap::const_iterator typeItr = pTypeInferInfo->exprTypeMap.find(pArgExpr);
			assert (typeItr != pTypeInferInfo->exprTypeMap.end());			
			inArgTypes.insert(inArgTypes.end(), typeItr->second.begin(), typeItr->second.end());
		}
	}
}

