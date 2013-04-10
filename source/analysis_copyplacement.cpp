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

#include "analysis_copyplacement.h"

CopyInfo ArrayCopyPlacement::NullEntry(0, Expression::SymbolSet());

/*******************************************************
 * Class   : operator<<
 * Purpose : print a CPEntry object
 * Initial : Nurudeen A. Lameed on December 18, 2009
 ********************************************************
 Revisions and bug fixes:
 */
std::ostream&  operator<<(std::ostream& out, const CPEntry cpe)
{	
	if (cpe.pStmt)
	{
		const SymbolExpr* symb = cpe.flowEntry.first;
		const Statement* s = cpe.flowEntry.second;
		
		out << "<" << cpe.pStmt->toString() << ",";
		out << cpe.stmtIndex << ",";
		out << "(" << (symb? symb->toString(): "null") << ",";
		out << (s? s->toString(): "param") << ")";
		out << ">";
	}
	
	return out;
}

/*******************************************************
 * Class   : operator<<
 * Purpose : print a CopyInfo object
 * Initial : Nurudeen A. Lameed on December 18, 2009
 ********************************************************
 Revisions and bug fixes:
 */
std::ostream&  operator<<(std::ostream& out, const CopyInfo& copyInfo)
{	
	const SymbolExpr* symb = copyInfo.first;
	const Expression::SymbolSet& checks = copyInfo.second;
	
	if (symb)
	{
		out << "<" << symb->toString();
		out << ">" << ":";
		std::ostream_iterator<const SymbolExpr*> output(out, ";");
		std::copy(checks.begin(), checks.end(), output);
	}
	
	return out;
}

/*******************************************************
 * Class   : operator<<
 * Purpose : print a CPFlowSet object
 * Initial : Nurudeen A. Lameed on December 18, 2009
 ********************************************************
 Revisions and bug fixes:
 */
std::ostream& operator<<(std::ostream& out, const CPFlowSet& fSet)
{
	out << "{";
	std::ostream_iterator<CPEntry> output(out, ";");
	std::copy(fSet.begin(), fSet.end(), output);
	out << "}";
	
	return out;
}

/*******************************************************
 * Class   : operator<<
 * Purpose : print a CPMap object
 * Initial : Nurudeen A. Lameed on December 18, 2009
 ********************************************************
 Revisions and bug fixes:
 */
std::ostream&  operator<<(std::ostream& out, const CPMap& result)
{
	out << "\n=====Array Copy Placement Analysis' Result=====" << std::endl;
	out << "Table size: " << result.size() << std::endl;

	for (CPMap::const_iterator it = result.begin(), iEnd = result.end(); it != iEnd; ++it)
	{
		const Statement* pStmt = it->first;
		const BlockCopyVecs& blockVecs = it->second;
		const StmtCopyVec& copies = blockVecs[0];
		
		out << pStmt->toString() << ":[";
		std::ostream_iterator<CopyInfo> output(out, ";");
		std::copy(copies.begin(), copies.end(), output);
		out << "] ";

		if (blockVecs.size() > 1)
		{
			const StmtCopyVec& hcopies = blockVecs[1];
			out << "Loop header [";
			std::ostream_iterator<CopyInfo> output(out, ";");
			std::copy(hcopies.begin(), hcopies.end(), output);
			out << "]";
		}
		
		out << "\n";
	}
	
	return out;
}

/*******************************************************************
* Function:	ArrayCopyPlacement::doCopyPlacement()
* Purpose : perform copy placement for a function
* Initial : Nurudeen A. Lameed on December 26, 2009
********************************************************************
Revisions and bug fixes:
*/
ArrayCopyAnalysisInfo* ArrayCopyPlacement::doCopyPlacement(const ProgFunction* pFunction,
														   const AnalysisResult& analysisResult)
{
	const CopyFlowSetMap& analysisInfo = analysisResult.second;
	CPMap cpInfo;	// analysis' results
	CPFlowSet mainSet; 
	//	const CopyFlowSetMap& loopIterInfo;
	const FlowContextPair fcPair(0,0);	

	doCopyPlacement(pFunction->getCurrentBody(), analysisInfo, mainSet, mainSet, 
					0, 0, mainSet, cpInfo, analysisResult.third, fcPair);
	
	// parameters that should be copied at the beginning of the function
	Expression::SymbolSet paramsToCopy;
	
	// std::cout << "Final flow set " << mainSet << std::endl;

	int leftOver = mainSet.size();
    const ProgFunction::ParamVector& inParams = pFunction->getInParams();
	ProgFunction::ParamVector::const_iterator ipBegin = inParams.begin(); 
	ProgFunction::ParamVector::const_iterator ipEnd = inParams.end();

	for(CPFlowSet::const_iterator it = mainSet.begin(), iEnd = mainSet.end(); it != iEnd; ++it)
	{
		const CPEntry& cpe = *it;

		ProgFunction::ParamVector::const_iterator fIt = 
		std::find(ipBegin, ipEnd, (cpe.flowEntry.first));
		if (fIt != ipEnd) --leftOver;

		if (!removeCopy(cpInfo, cpe)) continue;

		paramsToCopy.insert((SymbolExpr*)cpe.flowEntry.first);	
	}

    // sanity check: only the parameters may remain in the final set
	assert(leftOver == 0 && "Flow set must be empty");


	//std::cout << "Backward analysis(placement) completed" << std::endl;
	//std::cout << cpInfo << std::endl;

	return new ArrayCopyAnalysisInfo(analysisResult.first, cpInfo, paramsToCopy);
}

/*******************************************************************
* Function:	ArrayCopyPlacement::doCopyPlacement()
* Purpose : perform copy placement for a sequence of statement
* Initial : Nurudeen A. Lameed on December 26, 2009
********************************************************************
Revisions and bug fixes:
*/
void ArrayCopyPlacement::doCopyPlacement(const StmtSequence* pStmtSeq,
										 const CopyFlowSetMap& analysisInfo,
										 CPFlowSet& startSet, CPFlowSet& exitSet, 
										 const CPFlowSet* pBreakSet, const CPFlowSet* pContSet,
										 const CPFlowSet& retSet, CPMap& cpInfo,
										 const CopyFlowSetMap& loopIterInfo,
										 const FlowContextPair& fcPair)
{
	const StmtSequence::StmtVector& stmts = pStmtSeq->getStatements();
	
	// perform analysis for each statement in a sequence of statements
	for (StmtSequence::StmtVector::const_reverse_iterator it = stmts.rbegin(), 
		   iEnd = stmts.rend(); it != iEnd; ++it)
	{
		const Statement* pStmt = *it;
		switch (pStmt->getStmtType()) 
		{
			case Statement::ASSIGN: 
			{
				const AssignStmt* pAssignStmt = dynamic_cast<const AssignStmt*>(pStmt);
				CopyFlowSetMap::const_iterator iter = analysisInfo.find(pAssignStmt);
				if (iter != analysisInfo.end()) 
				{ 
				  const CopyFlowSetMap::const_iterator lIter = loopIterInfo.find(pAssignStmt);
				  if (lIter != loopIterInfo.end())	// found
				  {
					// flow information in iteration 1
					const FlowInfo& faInfoIt1 = lIter->second;
					const CopyFlowSetVec& loopFirstInSets = faInfoIt1.inSetVec;
					flowFunction(pAssignStmt, pStmtSeq, iter->second, startSet, exitSet, 
								 cpInfo, loopFirstInSets, fcPair);
				  }
				  else
				  {
					flowFunction(pAssignStmt, pStmtSeq, iter->second, startSet, exitSet, cpInfo, fcPair);
				  }
				}
			}
			break;
		
			case Statement::IF_ELSE: 
			{
				CPFlowSet ifStartSet;
				const IfElseStmt* pIfElseStmt = dynamic_cast<const IfElseStmt*>(pStmt);		
				doCopyPlacement(pIfElseStmt, analysisInfo, ifStartSet, startSet, pBreakSet, 
								pContSet, retSet, cpInfo, loopIterInfo, fcPair);
				startSet = ifStartSet;
			}
			break;
		
			case Statement::LOOP: 
			{
	            const LoopStmt* pLoopStmt = dynamic_cast<const LoopStmt*>(pStmt);
			    const FlowContextPair& fcPair = ArrayCopyAnalysis::getFlowContext(pLoopStmt);
				CPFlowSet loopStartSet;
				doCopyPlacement(pLoopStmt, analysisInfo, loopStartSet, startSet, retSet, 
								cpInfo, loopIterInfo, fcPair);
				startSet = loopStartSet;
			}
			break;
			
			case Statement::BREAK: 
			{
				exitSet = *pBreakSet;
			}
			break;
			
			case Statement::CONTINUE: 
			{
				exitSet = *pContSet;
			}
			break;
			
			case Statement::RETURN: 
			{
				exitSet = retSet;
			}
			break;
			
			default: {}
		}
	}
}

/*******************************************************************
* Function:	ArrayCopyPlacement::doCopyPlacement()
* Purpose : perform copy placement for a loop
* Initial : Nurudeen A. Lameed on December 26, 2009
********************************************************************
Revisions and bug fixes:
*/
void ArrayCopyPlacement::doCopyPlacement(const LoopStmt* pLoopStmt,
										 const CopyFlowSetMap& analysisInfo,
										 CPFlowSet& startSet, CPFlowSet& exitSet, 
										 const CPFlowSet& retSet, CPMap& cpInfo,
										 const CopyFlowSetMap& loopIterInfo,
										 const FlowContextPair& fcPair)
{
	CPFlowSet loopBreakSet = exitSet;
	CPFlowSet loopContSet;
	CPFlowSet loopExitSet;		// start with the exit set
	CPFlowSet loopStartSet;
	CPFlowSet prevStartSet;

    CPMap loopCPMap; // current result

	while (true)
	{
		loopCPMap = cpInfo;		// always start with the same output
		loopExitSet = exitSet;	// always start with the same exit set
		
		// perform copy placement analysis on the body
		doCopyPlacement(pLoopStmt->getBodySeq(), analysisInfo, loopStartSet, loopExitSet, 
						&loopBreakSet, &loopContSet, retSet, loopCPMap, loopIterInfo, fcPair);

		// set the continue flow set
		loopContSet = loopStartSet;

		// If the fixed point is reached, stop		
		if (prevStartSet == loopStartSet)
		{
		   break;
		}

		prevStartSet = loopStartSet;
	}
	
	// merge with the sequence
	startSet = merge(loopCPMap, pLoopStmt, loopExitSet, loopStartSet, fcPair.first);
	cpInfo = loopCPMap;
}

/*******************************************************************
* Function:	ArrayCopyPlacement::doCopyPlacement()
* Purpose : perform copy placement for an if else statement
* Initial : Nurudeen A. Lameed on December 26, 2009
********************************************************************
Revisions and bug fixes:
*/
void ArrayCopyPlacement::doCopyPlacement(const IfElseStmt* pIfElseStmt,
										 const CopyFlowSetMap& analysisInfo,
										 CPFlowSet& startSet, CPFlowSet& exitSet,
										 const CPFlowSet* pBreakSet,
										 const CPFlowSet* pContSet,
										 const CPFlowSet& retSet, CPMap& cpInfo,
										 const CopyFlowSetMap& loopIterInfo,
										 const FlowContextPair& fcPair)
{
	CPFlowSet ifStartSet, elseStartSet;
	CPFlowSet ifExitSet(exitSet);
	CPFlowSet elseExitSet(exitSet);
	
	// else block
	doCopyPlacement(pIfElseStmt->getElseBlock(), analysisInfo, elseStartSet,
					elseExitSet, pBreakSet, pContSet, retSet, cpInfo, loopIterInfo, fcPair);
	
	// if block
	doCopyPlacement(pIfElseStmt->getIfBlock(), analysisInfo, ifStartSet,
					ifExitSet, pBreakSet, pContSet, retSet, cpInfo, loopIterInfo, fcPair);
	
	// find the intersection of the two exit sets
	CPFlowSet intersec, diff;
	if (ifExitSet.empty()) 
	{
		intersec = elseExitSet;
	} 
	else if (elseExitSet.empty()) 
	{
		intersec = ifExitSet;
	} 
	else 
	{
		// exclude those entries removed by either block
		std::set_intersection(ifExitSet.begin(), ifExitSet.end(),
		elseExitSet.begin(), elseExitSet.end(), std::inserter(intersec, intersec.begin()));
	}
	
	// compute the final startSet.
	startSet = merge(cpInfo, pIfElseStmt, intersec, ifStartSet, elseStartSet, fcPair.first);
}

/*******************************************************************
* Function:	ArrayCopyPlacement::findIntersection()
* Purpose : find common entries in two copy flow set
* Initial : Nurudeen A. Lameed on December 26, 2009
********************************************************************
Revisions and bug fixes:
*/
CPFlowSet& ArrayCopyPlacement::findIntersection(const Statement* pStmt, CPFlowSet& intersec,
												CPFlowSet& mainSet, CPFlowSet& blockSet,
												FlowContext flowContext)
{
	if (mainSet.empty()) return intersec;
	
	// find the intersection
	for (CPFlowSet::const_iterator it = blockSet.begin(), mBegin = mainSet.begin(), 
			 mEnd = mainSet.end(), iEnd = blockSet.end(); it != iEnd; ++it)
    {	
		const CPEntry& oldCpe = *it;
		CPFlowSet::const_iterator mIt = std::find_if(mBegin, mEnd, CPFinderByFlowEntry(oldCpe.flowEntry));		
		if(mIt == mainSet.end()) continue; 	// if not found, skip it
		intersec.insert(CPEntry(pStmt, oldCpe.flowEntry, flowContext));
	}
		
	return intersec;
}

/*******************************************************************
* Function:	ArrayCopyPlacement::rmIntersection()
* Purpose : remove a common entry from a copy flow set
* Initial : Nurudeen A. Lameed on December 26, 2009
********************************************************************
Revisions and bug fixes:
*/
CPFlowSet& ArrayCopyPlacement::rmIntersection(CPMap& cpInfo, CPFlowSet& blockSet, 
											  const ContxInsFlowEntry& fe)
{
	// find the intersection, assumes that fe exists, no check is done
	CPFlowSet::const_iterator mIt = 
			std::find_if(blockSet.begin(), blockSet.end(), CPFinderByFlowEntry(fe));
	if ( mIt != blockSet.end() )
	{
		removeCopy(cpInfo, *mIt);
		blockSet.erase(mIt);
	}
	
	return blockSet;
}

/*******************************************************************
* Function:	ArrayCopyPlacement::merge
* Purpose : merge entries in ifelse statement with the sequence
* Initial : Nurudeen A. Lameed on December 26, 2009
********************************************************************
Revisions and bug fixes:
*/
CPFlowSet ArrayCopyPlacement::merge(CPMap& cpInfo, const IfElseStmt* pIfElseStmt, 
									CPFlowSet& mainSet, CPFlowSet& ifStartSet,
									CPFlowSet& elseStartSet, FlowContext flowContext)
{
	CPFlowSet result;
	
	// find the intersection with the if block
	result = findIntersection(pIfElseStmt, result, mainSet, ifStartSet, flowContext);
	
	// repeat for the else block
	result = findIntersection(pIfElseStmt, result, mainSet, elseStartSet, flowContext);
	
	// find the intersection between the if else blocks
	result = findIntersection(pIfElseStmt, result, ifStartSet, elseStartSet, flowContext);
	
	// find the difference and move the common entry
	for (CPFlowSet::const_iterator it = result.begin(), iEnd = result.end(); it != iEnd; ++it)
	{
		const CPEntry& cpe = *it;
		rmIntersection(cpInfo, mainSet, cpe.flowEntry);
		
		// do for the ifelse block
		rmIntersection(cpInfo, ifStartSet, cpe.flowEntry);
		rmIntersection(cpInfo, elseStartSet, cpe.flowEntry);
		
		// add the moved (merged) copy
		addCopy(cpInfo, cpe);
	}
	
	// copy the rest
	result.insert(mainSet.begin(), mainSet.end());
	result.insert(ifStartSet.begin(), ifStartSet.end());
	result.insert(elseStartSet.begin(), elseStartSet.end());
	
	return result;
}

/*******************************************************************
* Function:	ArrayCopyPlacement::merge()
* Purpose : merge result of a loop statement with the sequence
* Initial : Nurudeen A. Lameed on December 26, 2009
********************************************************************
Revisions and bug fixes:
*/
CPFlowSet ArrayCopyPlacement::merge(CPMap& cpInfo, const LoopStmt* ploopStmt, 
									CPFlowSet& mainSet, CPFlowSet& blockSet, 
									FlowContext flowContext)
{
	CPFlowSet result;
	
	// find the intersection
	result = findIntersection(ploopStmt, result, mainSet, blockSet, flowContext);
	
	// find the difference and move the common entry
	for (CPFlowSet::const_iterator it = result.begin(), iEnd = result.end(); it != iEnd; ++it)
	{
		const CPEntry& oldCpe = *it;
		
		rmIntersection(cpInfo, mainSet, oldCpe.flowEntry);
		rmIntersection(cpInfo, blockSet, oldCpe.flowEntry);
		
		// add the moved (merged) copy
		addCopy(cpInfo, *it);
	}
	
	// we assume a loop will be executed at least once, so we will attach the 
	// remaining entries to the loop; to be generated just be before the loop
	for (CPFlowSet::const_iterator it = blockSet.begin(); it != blockSet.end(); ++it)
	{
		const CPEntry& oldCpe = *it;
		removeCopy(cpInfo, oldCpe);
		CPEntry newCpe(ploopStmt, oldCpe.flowEntry, flowContext);
		addCopy(cpInfo, newCpe);
		result.insert(newCpe);
	}
	
	// copy the rest
	result.insert(mainSet.begin(), mainSet.end());
	
	return result;
}

/*******************************************************************
* Function:	ArrayCopyPlacement::flowFunction()
* Purpose : compute outset from inset for a statement
* Initial : Nurudeen A. Lameed on December 26, 2009
********************************************************************
Revisions and bug fixes:
*/
void ArrayCopyPlacement::flowFunction(const AssignStmt* pAssignStmt,
									  const IIRNode* block, const FlowInfo& faInfo,
									  CPFlowSet& startSet, CPFlowSet& exitSet, CPMap& cpInfo,
									  const FlowContextPair& fcPair)
{
	const CopyFlowSetVec& genSetVec = faInfo.genSetVec;
	const CopyFlowSetVec& inSetVec = faInfo.inSetVec;
	const CopyFlowSetVec& copySetVec = faInfo.copySetVec;

	Expression::ExprVector lvalues = pAssignStmt->getLeftExprs();
	
	// for each lvalue do ....
	for (size_t i=0, size = lvalues.size(); i < size; ++i) 
	{
		Expression* pExpr = lvalues[i];
		const CopyFlowSet&  faGenSet = genSetVec[i];
		const CopyFlowSet&  faInSet = inSetVec[i];
		const CopyFlowSet&  faCopySet = copySetVec[i];

		switch (pExpr->getExprType()) 
		{
			case Expression::SYMBOL: 
			{
				SymbolExpr* lArrayVar = dynamic_cast<SymbolExpr*>(pExpr);
				moveCopy(pAssignStmt, i, lArrayVar, faGenSet, faInSet, startSet, exitSet, cpInfo, fcPair);
			}
			break;

			case Expression::PARAM: 
			{
				for (CopyFlowSet::const_iterator it = faCopySet.begin(), iEnd = faCopySet.end(); it != iEnd; ++it) 
				{
					const FlowEntry& fe = *it;
					ContxInsFlowEntry cfe(fe.arrayVar, fe.allocator);
					CPEntry cpe(pAssignStmt, cfe, i, fcPair.first);
					addCopy(cpInfo, cpe);
					startSet.insert(cpe);
				}
			}
			break;
			
			default: 
			{
			}
		}
	}
}


void ArrayCopyPlacement::flowFunction(const AssignStmt* pAssignStmt,
									  const IIRNode* block, const FlowInfo& faInfo,
									  CPFlowSet& startSet, CPFlowSet& exitSet,
									  CPMap& cpInfo, const CopyFlowSetVec& loopFirstInSets,
									  const FlowContextPair& fcPair) 
{
	const CopyFlowSetVec& genSetVec = faInfo.genSetVec;
	const CopyFlowSetVec& inSetVec = faInfo.inSetVec;
	const CopyFlowSetVec& copySetVec = faInfo.copySetVec;
	Expression::ExprVector lvalues = pAssignStmt->getLeftExprs();
	
	// for each lvalue do ....
	for (size_t i=0, size = lvalues.size(); i < size; ++i) 
	{
		Expression* pExpr = lvalues[i];
		const CopyFlowSet&  faGenSet = genSetVec[i];
		const CopyFlowSet&  faInSet = inSetVec[i];
		const CopyFlowSet&  faCopySet = copySetVec[i];

		switch (pExpr->getExprType()) 
		{
			case Expression::SYMBOL: 
			{
				SymbolExpr* lArrayVar = dynamic_cast<SymbolExpr*>(pExpr);
				moveCopy(pAssignStmt, i, lArrayVar, faGenSet, faInSet, 
						 startSet, exitSet, cpInfo, fcPair);
			}
			break;

			case Expression::PARAM: 
			{
				CPFlowSet genCopies;
				bool isNotFinalLoc  = true; 
				for (CopyFlowSet::const_iterator it = faCopySet.begin(), iEnd = faCopySet.end(); it != iEnd; ++it) 
				{
					const FlowEntry& fe = *it;
					ContxInsFlowEntry cfe(fe.arrayVar, fe.allocator);
					CPEntry cpe(pAssignStmt, cfe, i, fcPair.first);
					addCopy(cpInfo, cpe);
					genCopies.insert(cpe);

					// required loop first iteration result for each statement of the loop
					// to determine if is there a backward dependency?
					isNotFinalLoc = isNotFinalLoc && !hasBackDependency(loopFirstInSets[i], faInSet, fe); 
				}
				
				if (isNotFinalLoc)
				{
					startSet.insert(genCopies.begin(), genCopies.end());
				}
			}
			break;

			default: 
			{
			}
		}
	}
}

/*******************************************************
* Class   : ArrayCopyPlacement::moveCopy()
* Purpose : relocate a copy entry
* Initial : Nurudeen A. Lameed on Decemeber 26, 2009
********************************************************
Revisions and bug fixes:
*/
void ArrayCopyPlacement::moveCopy(const AssignStmt* pAssignStmt, size_t index,
								  const SymbolExpr* lArrayVar, const CopyFlowSet& genSet,
								  const CopyFlowSet& inSet,	CPFlowSet& startSet,
								  CPFlowSet& exitSet, CPMap& cpInfo,
								  const FlowContextPair& fcPair)
{	
	SymbolExpr* rArrayVar = dynamic_cast<SymbolExpr*>(pAssignStmt->getRightExpr());
	
	// for the current block
	if ( !startSet.empty() )
	{
		CPFlowSet killSet;
		for (CPFlowSet::const_iterator it = startSet.begin(), iEnd = startSet.end(); it != iEnd; ++it)
		{
			const CPEntry& cpe = *it;
			if (cpe.flowEntry.first != lArrayVar && cpe.flowEntry.first != rArrayVar) continue;
			
			killSet.insert(cpe);
		}
		
		for (CPFlowSet::const_iterator kIt = killSet.begin(), kEnd = killSet.end(); kIt != kEnd; ++kIt)
		  {
			startSet.erase(*kIt);
		  }
	} 
	  		
	// consider the copies from other blocks below the current block
    CPFlowSet killOthers;
	for (CPFlowSet::const_iterator cIt = exitSet.begin(), cEnd = exitSet.end(); cIt != cEnd; ++cIt)
	  {
		const CPEntry& cpe = *cIt;
		if (cpe.flowEntry.first != lArrayVar && cpe.flowEntry.first != rArrayVar) continue;		
	    
		if (cpe.flowContext == fcPair.first) continue; // prevents moving a copy into a different context 

		if (rArrayVar)					//	instead, generate a runtime check at the current loc, cpe.pStmt
		{								//  because the loop's body might not be executed 
			Expression::SymbolSet checks;
			checks.insert(rArrayVar);
			addCheck(cpInfo, *cIt, checks);
		}

		killOthers.insert(cpe);
	  }
	  
	  for (CPFlowSet::const_iterator kIt = killOthers.begin(), kEnd = killOthers.end(); kIt != kEnd; ++kIt)
	  {
			exitSet.erase(*kIt);
   	  }

	  if (exitSet.empty()) return; 
	  
	  
 
   // now process the rest ...
	CopyFlowSet defs;
	ArrayCopyAnalysis::getAllDefs(inSet, lArrayVar, defs);
	Expression::SymbolSet runtimeChecks;
	for (CopyFlowSet::const_iterator dIt = defs.begin(), dEnd = defs.end(); dIt != dEnd; ++dIt)
	{	
		const FlowEntry& fe = *dIt;
		ContxInsFlowEntry cfe(fe.arrayVar, fe.allocator);
		
		if (ArrayCopyAnalysis::isSharedArrayVar(inSet, fe, runtimeChecks))		// P_i is not empty
		{
			// find any copy for the current lhs ...
			CPFlowSet::const_iterator fIt = 
			std::find_if(exitSet.begin(), exitSet.end(), CPFinderByFlowEntry(cfe));
			
			if (fIt != exitSet.end())	// if a copy is found
			{
				// print the checks
				for(Expression::SymbolSet::const_iterator it = runtimeChecks.begin(), iEnd = runtimeChecks.end(); it != iEnd; ++it)
				{
					  std::cout << "RUNTIME CHECKS REQUIRED!!!: " << ((*it)->toString()) <<  "\n";
				}
				
				addCheck(cpInfo, *fIt, runtimeChecks);
				exitSet.erase(fIt);
			}
		 }
	 } 
		
	 for (CopyFlowSet::const_iterator gIt = genSet.begin(), gEnd = genSet.end(); gIt != gEnd; ++gIt)
	 {	 
		const FlowEntry& fe = *gIt;
		ContxInsFlowEntry cfe(fe.arrayVar, fe.allocator);
		ContxInsFlowEntry cfeRHS(rArrayVar, fe.allocator);
		if (moveCopy(inSet, cpInfo, pAssignStmt, index, exitSet, cfe, runtimeChecks, fcPair.first)) continue;
		
		if (!rArrayVar)  continue; // try with the rhs

		moveCopy(inSet, cpInfo, pAssignStmt, index, exitSet, cfeRHS, runtimeChecks, fcPair.first);	
	 }
}

/*******************************************************
* Class   : ArrayCopyPlacement::moveCopy()
* Purpose : relocate a copy entry
* Initial : Nurudeen A. Lameed on Decemeber 26, 2009
********************************************************
Revisions and bug fixes:
*/
bool ArrayCopyPlacement::moveCopy(const CopyFlowSet& inSet,	CPMap& cpInfo,
								  const AssignStmt* pAssignStmt, size_t index,
								  CPFlowSet& fs, const ContxInsFlowEntry&  fe,
								  const Expression::SymbolSet& checks,
								  FlowContext flowContext)
{
	if (!fs.empty())
	{
		CPFlowSet::const_iterator iter = 
		std::find_if(fs.begin(), fs.end(), CPFinderByFlowEntry(fe));
		if (iter != fs.end())
		{
			//std::cout << "MOVING!!! to ... " << pAssignStmt->toString() 
			//		  << " " << fe.first->toString() 
			//		  << " " << fe.second->toString() << "\n"; 
			CPEntry cpe(pAssignStmt, fe, index, flowContext);	// generate a copy
			removeCopy(cpInfo, *iter);							// remove from the old place
			addCopy(cpInfo, cpe, checks);						// add it to the result (for this statement)
			fs.erase(iter);										// place it permanently
			return true;
		}
	}

	return false;
}

/*******************************************************
* Class   : ArrayCopyPlacement::hasBackDependency()
* Purpose : checks whether there is a backward dependency in the loop
* Initial : Nurudeen A. Lameed on Decemeber 26, 2009
********************************************************
Revisions and bug fixes:
*/
bool ArrayCopyPlacement::hasBackDependency(const CopyFlowSet& inSetAtIter1, 
										 const CopyFlowSet& inSet, const FlowEntry& cp)
{
  for (CopyFlowSet::const_iterator it = inSet.begin(), iEnd = inSet.end(), 
	   i1Begin = inSetAtIter1.begin(), i1End = inSetAtIter1.end(); it != iEnd; ++it)
	{
	  const FlowEntry& fe = *it;
	  if (fe.allocator != cp.allocator || fe.context != cp.context) continue;
	  
	  if (std::find(i1Begin, i1End, fe) == i1End) return true;
	}

  return   false;
}

/*******************************************************
* Class   : ArrayCopyPlacement::addCopy()
* Purpose : add a copy symbol and checks to the current result
* Initial : Nurudeen A. Lameed on Decemeber 26, 2009
********************************************************
Revisions and bug fixes:
*/
void ArrayCopyPlacement::addCopy(CPMap& cpInfo, const CPEntry& cpe,
								 const Expression::SymbolSet& checks, bool isLoopHeader) 
{	
	CopyInfo copyInfo(cpe.flowEntry.first, checks);
	Statement::StmtType typ = cpe.pStmt->getStmtType();
	switch (typ) 
	{
		case Statement::ASSIGN:
		case Statement::IF_ELSE: 
		{
			CPMap::iterator it = cpInfo.find(cpe.pStmt);
			if (it != cpInfo.end()) 
			{
				// get the copy vectors for this block
				BlockCopyVecs& blockVecs = it->second;
	
				// get the copy vector for this statement
				StmtCopyVec& cpVec = blockVecs[0];
				if (typ == Statement::IF_ELSE) 
				{
					cpVec.push_back(copyInfo);
					StmtCopyVec::iterator fIt = 
					std::find_if(cpVec.begin(), cpVec.end(), EntryFinder(copyInfo.first));
					if (fIt != cpVec.end())
					{
						CopyInfo& cp = *fIt;
						cp.second = copyInfo.second;
					}
					else
					{
						cpVec.push_back(copyInfo);
					}
				} 
				else 
				{
					assert(cpe.stmtIndex >= 0 && cpe.stmtIndex < cpVec.size());
					cpVec[cpe.stmtIndex] = copyInfo;
				}
			} 
			else 
			{
				StmtCopyVec cpVec;
				if (typ == Statement::ASSIGN) 
				{
					const AssignStmt* pAssignStmt = dynamic_cast<const AssignStmt*>(cpe.pStmt);	
					size_t size = pAssignStmt->getLeftExprs().size();
					assert(cpe.stmtIndex >= 0 && cpe.stmtIndex < size);
					StmtCopyVec acpVec(size, NullEntry);
					cpVec = acpVec;
					cpVec[cpe.stmtIndex] = copyInfo;
				} 
				else 
				{
					cpVec.push_back(copyInfo);
				}
	
				BlockCopyVecs blockVecs(1, cpVec);
				cpInfo[cpe.pStmt] = blockVecs;
			}
		}
		break;
	
		case Statement::LOOP:
		{
			CPMap::iterator it = cpInfo.find(cpe.pStmt);
			if (it != cpInfo.end()) 
			{
				// get the copy vectors for this block
				BlockCopyVecs& blockVecs = it->second;

				// get the copy vector for this statement
				StmtCopyVec& cpVec = isLoopHeader ? blockVecs[1] : blockVecs[0];
				
				StmtCopyVec::iterator fIter = 
				std::find_if(cpVec.begin(), cpVec.end(), EntryFinder(copyInfo.first));
				if (fIter != cpVec.end())
				{
					CopyInfo& cp = *fIter;
					cp.second = copyInfo.second;
				}
				else
				{
					cpVec.push_back(copyInfo);
				}
			} 
			else 
			{
				StmtCopyVec cpVec;
				cpVec.push_back(copyInfo);
				BlockCopyVecs blockVecs(2);

				if (isLoopHeader) 
				{
					blockVecs[0] = StmtCopyVec();
					blockVecs[1] = cpVec;
				} 
				else 
				{
					blockVecs[0] = cpVec;
					blockVecs[1] = StmtCopyVec();
				}

				cpInfo[cpe.pStmt] = blockVecs;
			}
		}
		break;
	
		default:{}
	}
}

/*******************************************************
* Class   : ArrayCopyPlacement::addCheck()
* Purpose : Add check(s) to an existing copy
* Initial : Nurudeen A. Lameed on Decemeber 26, 2009
********************************************************
Revisions and bug fixes:
*/
inline void ArrayCopyPlacement::addCheck(CPMap& cpInfo, const CPEntry& cpe,
										 const Expression::SymbolSet& checks) 
{
	CopyInfo* entry = 0;
	if (findCopy(cpInfo, cpe, entry))
	{
		// update the value of the current checks
		Expression::SymbolSet& curChecks = entry->second;
		curChecks.insert(checks.begin(), checks.end());
	}
}

/*******************************************************
* Class   : ArrayCopyPlacement::findCopy()
* Purpose : locate a copy entry for a given statement
* Initial : Nurudeen A. Lameed on Decemeber 26, 2009
********************************************************
Revisions and bug fixes:
*/
bool ArrayCopyPlacement::findCopy(CPMap& cpInfo, const CPEntry& cpe, CopyInfo*& result)
{
	CPMap::iterator it = cpInfo.find(cpe.pStmt);
	if (it != cpInfo.end()) 
	{
		// get the copy vectors for this block
		BlockCopyVecs& blockVecs = it->second;

		// must be greater than 0
		assert(blockVecs.size() > 0);

		// get the copy vector for this statement
		StmtCopyVec& cpVec = blockVecs[0];
		
		if (cpe.pStmt->getStmtType() != Statement::ASSIGN)
		{
			// find it;
			StmtCopyVec::iterator fIter = 
			std::find_if(cpVec.begin(), cpVec.end(), EntryFinder(cpe.flowEntry.first));
			if (fIter != cpVec.end())
			{
				result = &(cpVec[fIter - cpVec.begin()]);
				return true;
			}
		}
		else
		{
			// a check on the index
			assert(cpe.stmtIndex >= 0 && cpe.stmtIndex < cpVec.size());		
			result = &cpVec[cpe.stmtIndex];
			return true;	// found;
		}
	}
	
	return false;
}

/*******************************************************
* Class   : ArrayCopyPlacement::removeCopy()
* Purpose : remove a copy for this statement
* Initial : Nurudeen A. Lameed on Decemeber 26, 2009
********************************************************
Revisions and bug fixes:
*/
inline bool ArrayCopyPlacement::removeCopy(CPMap& cpInfo, const CPEntry& cpe) 
{
	CopyInfo* entry = 0;
	if (findCopy(cpInfo, cpe, entry))
	{
		if (entry->second.empty())	// remove only those without checks
		{			
			entry->first = 0;
			return true;	// 'removed'
		}
	}
	
	return false;
}

/*******************************************************
* Class   : computeArrayCopy
* Purpose : compute array copy analysis result
* Initial : Nurudeen A. Lameed on Decemeber 26, 2009
********************************************************
Revisions and bug fixes:
*/
AnalysisInfo* ArrayCopyElim::computeArrayCopyElim(const ProgFunction* pFunction,
													  const StmtSequence* pFuncBody,
													  const TypeSetString& inArgTypes,
													  bool returnBot)
{
	
	//std::cout << "Performing array copy analysis on: " << pFunction->getFuncName() << "\n";
	//std::cout <<  pFunction->toString() << "\n";

	// clock_t t1 = clock();
	return  ArrayCopyPlacement::doCopyPlacement(
				pFunction, 
				ArrayCopyAnalysis::doAnalysis(pFunction, inArgTypes, returnBot)
			);
	
	//AnalysisInfo *info = ArrayCopyPlacement::doCopyPlacement(
		//			pFunction, 
			//		ArrayCopyAnalysis::doAnalysis(pFunction, inArgTypes, returnBot)
				//);
	//clock_t t2 = clock();
	
	//std::cout << "Total analysis time: " << ((double)(t2 - t1)/CLOCKS_PER_SEC) << std::endl;
	// return info;
}
