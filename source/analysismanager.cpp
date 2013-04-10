// =========================================================================== //
//                                                                             //
// Copyright [yyyy] Maxime Chevalier-Boisvert and McGill University.           //
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

// Header files
#include <iostream>
#include <cassert>
#include "analysismanager.h"
#include "configmanager.h"
#include "functions.h"

// Static analysis info cache map object
AnalysisManager::CacheMap AnalysisManager::s_cacheMap;

/***************************************************************
* Function: AnalysisManager::requestInfo()
* Purpose : Request information from an analysis 
* Initial : Maxime Chevalier-Boisvert on May 4, 2009
****************************************************************
Revisions and bug fixes:
*/
const AnalysisInfo* AnalysisManager::requestInfo(
	AnalysisFunc pAnalysis,
	const ProgFunction* pFunction,
	const StmtSequence* pFuncBody,
	const TypeSetString& inArgTypes
)
{
	// If we are in verbose mode
	if (ConfigManager::s_verboseVar)
	{	
		// Log the function name
		std::cout << "Entering AnalysisManager::requestInfo()" << std::endl;
		std::cout << "Analyzing function: \"" << pFunction->getFuncName() << "\"" << std::endl;
	}
		
	// Create a cache key object from the input parameters
	CacheKey cacheKey(pAnalysis, pFunction, pFuncBody, inArgTypes);
	
	// Attempt to find a corresponding entry in the cache
	CacheMap::iterator cacheItr = s_cacheMap.find(cacheKey);
	
	// If there is no corresponding cache entry
	if (cacheItr == s_cacheMap.end())
	{
		// Create a cached info entry and get a reference to it
		CachedInfo& cachedInfo = s_cacheMap[cacheKey];
		
		// Set the analysis running flag to true
		cachedInfo.running = true;
		
		// Run the analysis and store the information in the cache
		cachedInfo.pInfo = pAnalysis(pFunction, pFuncBody, inArgTypes, false);
		
		// Set the analysis running flag to false
		cachedInfo.running = false;

		// If we are in verbose mode, log that we are returning a computed result
		if (ConfigManager::s_verboseVar)
			std::cout << "Returning computed result" << std::endl;
		
		// Return the analysis information
		return cachedInfo.pInfo;
	}
	
	// Get a reference to the cached info entry
	CachedInfo& cachedInfo = cacheItr->second;
		
	// If the analysis is already running
	if (cachedInfo.running)
	{
		// If we are in verbose mode, log that we are returning bottom
		if (ConfigManager::s_verboseVar)
			std::cout << "Returning bottom" << std::endl;
		
		// Return the bottom element for this analysis
		return pAnalysis(pFunction, pFuncBody, inArgTypes, true);
	}

	// If we are in verbose mode, log that we are returning a cached result
	if (ConfigManager::s_verboseVar)
		std::cout << "Returning cached result" << std::endl;
	
	// Return the cached analysis information
	return cachedInfo.pInfo;
}

/***************************************************************
* Function: AnalysisManager::clearCache()
* Purpose : Clear the analysis info cache
* Initial : Maxime Chevalier-Boisvert on May 4, 2009
****************************************************************
Revisions and bug fixes:
*/
void AnalysisManager::clearCache()
{
	// Clear the analysis info cache
	s_cacheMap.clear();
}
