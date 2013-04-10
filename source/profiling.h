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
#ifndef PROFILING_H_
#define PROFILING_H_

// Header files
#include <string>
#include "platform.h"
#include "spreadsheet.h"
#include "configmanager.h"

// Define the preprocessor variable below to disable profiling code
//#define MCVM_DISABLE_PROFILING

// Profiler macro definitions
#ifndef MCVM_DISABLE_PROFILING
	#define PROF_INCR_COUNTER(COUNTER_VAR) (Profiler::incrCounter(COUNTER_VAR))
	#define PROF_GET_COUNTER(COUNTER_VAR) (Profiler::getCounter(COUNTER_VAR))
	#define PROF_SET_COUNTER(COUNTER_VAR, VALUE) (Profiler::setCounter(COUNTER_VAR, VALUE))
	#define PROF_START_TIMER(TIMER_VAR) (Profiler::startTimer(TIMER_VAR))
	#define PROF_STOP_TIMER(TIMER_VAR) (Profiler::stopTimer(TIMER_VAR))
#else
	#define PROF_INCR_COUNTER()
	#define PROF_GET_COUNTER()
	#define PROF_SET_COUNTER()
	#define PROF_START_TIMER()
	#define PROF_STOP_TIMER()
#endif

/***************************************************************
* Class   : Profiler
* Purpose : Gather and manage profiling data
* Initial : Maxime Chevalier-Boisvert on April 7, 2009
****************************************************************
Revisions and bug fixes:
*/
class Profiler
{
public:
	
	// Counter variables
	enum CounterVar
	{
		MATRIX_CONSTR_COUNT,
		MATRIX_GETSLICE_COUNT,
		MATRIX_MULT_COUNT,
		ENV_LOOKUP_COUNT,
		FUNC_LOAD_COUNT,
		FUNC_CALL_COUNT,
		FUNC_COMP_COUNT,
		FUNC_VERS_COUNT,
		METRIC_NUM_STMTS,
		METRIC_MAX_LOOP_DEPTH,
		METRIC_NUM_CALL_SITES,
		TYPE_NUM_TYPE_SETS,
		TYPE_NUM_EMPTY_SETS,
		TYPE_NUM_UNARY_SETS,
		TYPE_NUM_SCALARS,
		TYPE_NUM_KNOWN_SCALARS,
		TYPE_NUM_MATRICES,
		TYPE_NUM_KNOWN_SIZE,
		ARRAY_COPY_COUNT,
		NUM_COUNTERS
	};

	// Timer variables
	enum TimerVar
	{
		COMP_TIME_TOTAL,
		ANA_TIME_TOTAL,
		NUM_TIMERS
	};
	
	// Counter variable names
	const static std::string COUNTER_VAR_NAMES[NUM_COUNTERS];

	// Timer variable names
	const static std::string TIMER_VAR_NAMES[NUM_TIMERS];
	
	// Method to initialize the profiler
	static void initialize();
	
	// Method to increment a counter variable
	static void incrCounter(CounterVar counterVar);
	
	// Methods to get and set a counter variable
	static uint64 getCounter(CounterVar counterVar);
	static void setCounter(CounterVar counterVar, uint64 value);

	// Methods to start and stop a timer variable
	static void startTimer(TimerVar timerVar);
	static void stopTimer(TimerVar timerVar);
	
	// Method to get the current time in seconds
	static double getTimeSeconds();
	
	// Library function to reset the profiling context
	static ArrayObj* resetContextCmd(ArrayObj* pArguments);
	static LibFunction s_resetContextCmd;
	
	// Library function to get the profiling info as a cell array
	static ArrayObj* getInfoCmd(ArrayObj* pArguments);
	static LibFunction s_getInfoCmd;	
	
	// Library function to print profiling information
	static ArrayObj* printInfoCmd(ArrayObj* pArguments);
	static LibFunction s_printInfoCmd;
	
private:
	
	// Profiling context information class
	class Context
	{
	public:
		
		// Constructor
		Context();
		
		// Counter variables
		uint64 counters[NUM_COUNTERS];
		
		// Timer variables
		float64 timers[NUM_TIMERS];
		
		// Timer start times
		float64 timerStart[NUM_TIMERS];
		
		// Number of timer runs
		uint32 timerRuns[NUM_TIMERS];
	};
	
	// Current profiling context
	static Context s_curContext;
};

#endif // #ifndef PROFILING_H_
