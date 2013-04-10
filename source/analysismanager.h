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
#ifndef ANALYSISMANAGER_H_
#define ANALYSISMANAGER_H_

// Header files
#include <utility>
#include <map>
#include <gc_cpp.h>
#include <gc/gc_allocator.h>
#include "typeinfer.h"

// Forward declarations
class ProgFunction;
class StmtSequence;

/***************************************************************
* Class   : AnalysisInfo
* Purpose : Base class for analysis info storage classes
* Initial : Maxime Chevalier-Boisvert on May 4, 2009
****************************************************************
Revisions and bug fixes:
*/
class AnalysisInfo : public gc
{
public:
	
	// Constructor and destructor
	AnalysisInfo() {}
	virtual ~AnalysisInfo() {}
};

// Analysis function type definition
typedef AnalysisInfo* (*AnalysisFunc)(
	const ProgFunction* pFunction,
	const StmtSequence* pFuncBody,
	const TypeSetString& inArgTypes,
	bool returnBottom
);

/***************************************************************
* Class   : AnalysisManager
* Purpose : Manage analyses
* Initial : Maxime Chevalier-Boisvert on May 4, 2009
****************************************************************
Revisions and bug fixes:
*/
class AnalysisManager
{
public:
	
	// Method to request information from an analysis
	static const AnalysisInfo* requestInfo(
		AnalysisFunc pAnalysis,
		const ProgFunction* pFunction,
		const StmtSequence* pFuncBody,
		const TypeSetString& inArgTypes
	);
	
	// Method to clear the analysis info cache
	static void clearCache();
	
private:
	
	// Cache key class definition
	class CacheKey
	{
	public:
		
		// Constructor
		CacheKey(
			AnalysisFunc pAnaFunc,
			const ProgFunction* pFunc,
			const StmtSequence* pBody,
			const TypeSetString& inTypes
		) : pAnalysis(pAnaFunc), 
			pFunction(pFunc),
		  	pFuncBody(pBody),
		  	inArgTypes(inTypes)
		{}
		
		// Less-than comparison operator (for sorting)
		bool operator < (const CacheKey& other) const
		{
			// Compare the key elements
			if (pAnalysis < other.pAnalysis) 		return true;
			else if (pAnalysis > other.pAnalysis)	return false;
			else if (pFunction < other.pFunction)	return true;
			else if (pFunction > other.pFunction)	return false;
			else if (pFuncBody < other.pFuncBody)	return true;
			else if (pFuncBody > other.pFuncBody)	return false;
			else if (inArgTypes < other.inArgTypes)	return true;
			else return false;
		}
			
		// Key elements
		AnalysisFunc pAnalysis;
		const ProgFunction* pFunction;
		const StmtSequence* pFuncBody;
		TypeSetString inArgTypes;
	};
	
	// Cached analysis info class definition
	class CachedInfo
	{
	public:

		// Analysis information object
		AnalysisInfo* pInfo;
		
		// Flag to indicate that the analysis is currently running
		bool running;
	};

	// Analysis info cache map type definition 
	typedef std::map<CacheKey, CachedInfo, std::less<CacheKey>, gc_allocator<std::pair<CacheKey, CachedInfo> > > CacheMap;
	
	// Analysis info cache map object
	static CacheMap s_cacheMap;
};

#endif // #ifndef ANALYSISMANAGER_H_
