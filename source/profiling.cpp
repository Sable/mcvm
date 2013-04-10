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

// Header files
#include <cassert>
#include <iostream>
#include <cstring>
#include <sys/time.h>
#include "profiling.h"
#include "interpreter.h"
#include "cellarrayobj.h"
#include "chararrayobj.h"

// Counter variable names
const std::string Profiler::COUNTER_VAR_NAMES[NUM_COUNTERS] =
{
	"matrices created",
	"matrix slice reads",
	"matrix mult ops",
	"env. lookup count",
	"functions loaded",
	"function call count",
	"functions compiled",
	"function versions",
	"num statements",
	"max loop depth",
	"num call sites",
	"num type sets",
	"num type sets empty",
	"num type sets unary",
	"num scalar found",
	"num scalars known",
	"num matrices found",
	"num mat. size known",
	"array copy count"
};

// Timer variable names
const std::string Profiler::TIMER_VAR_NAMES[NUM_TIMERS] =
{
	"total comp. time",
	"total analysis time"
};

// Library function to reset the profiling context
LibFunction Profiler::s_resetContextCmd("mcvm_reset_prof_context", Profiler::resetContextCmd);

// Library function to print profiling information
LibFunction Profiler::s_getInfoCmd("mcvm_get_prof_info", Profiler::getInfoCmd);

// Library function to print profiling information
LibFunction Profiler::s_printInfoCmd("mcvm_print_prof_info", Profiler::printInfoCmd);

// Current profiler context
Profiler::Context Profiler::s_curContext;

/***************************************************************
* Function: Profiler::initialize()
* Purpose : Initialize the profiler
* Initial : Maxime Chevalier-Boisvert on April 7, 2009
****************************************************************
Revisions and bug fixes:
*/
void Profiler::initialize()
{
	// Register the local library functions
	Interpreter::setBinding(s_resetContextCmd.getFuncName(), (DataObject*)&s_resetContextCmd);
	Interpreter::setBinding(s_getInfoCmd.getFuncName(), (DataObject*)&s_getInfoCmd);
	Interpreter::setBinding(s_printInfoCmd.getFuncName(), (DataObject*)&s_printInfoCmd);
}

/***************************************************************
* Function: Profiler::incrCounter()
* Purpose : Increment a counter variable
* Initial : Maxime Chevalier-Boisvert on April 7, 2009
****************************************************************
Revisions and bug fixes:
*/
void Profiler::incrCounter(CounterVar counterVar)
{
	// Ensure that the counter variable is valid
	assert (counterVar < NUM_COUNTERS);
	
	// Increment the desired counter by 1
	s_curContext.counters[counterVar] += 1;
}

/***************************************************************
* Function: Profiler::getCounter()
* Purpose : Get the value of a counter variable
* Initial : Maxime Chevalier-Boisvert on July 22, 2009
****************************************************************
Revisions and bug fixes:
*/
uint64 Profiler::getCounter(CounterVar counterVar)
{
	// Ensure that the counter variable is valid
	assert (counterVar < NUM_COUNTERS);
	
	// Return the counter's value
	return s_curContext.counters[counterVar];
}

/***************************************************************
* Function: Profiler::setCounter()
* Purpose : Set the value of a counter variable
* Initial : Maxime Chevalier-Boisvert on July 22, 2009
****************************************************************
Revisions and bug fixes:
*/
void Profiler::setCounter(CounterVar counterVar, uint64 value)
{
	// Ensure that the counter variable is valid
	assert (counterVar < NUM_COUNTERS);
	
	// Set the counter's value
	s_curContext.counters[counterVar] = value;
}

/***************************************************************
* Function: Profiler::startTimer()
* Purpose : Start a timer variable
* Initial : Maxime Chevalier-Boisvert on July 21, 2009
****************************************************************
Revisions and bug fixes:
*/
void Profiler::startTimer(TimerVar timerVar)
{
	// Ensure that the timer variable is valid
	assert (timerVar < NUM_TIMERS);
	
	// Increment the number of timer runs for this timer
	s_curContext.timerRuns[timerVar] += 1;
	
	// If this is the first timer run for this timer
	if (s_curContext.timerRuns[timerVar] == 1)
	{
		// Compute the time in seconds using microsecond information
		double timeSecs = getTimeSeconds();
	
		// Store the timer start time
		s_curContext.timerStart[timerVar] = timeSecs;
	}
}

/***************************************************************
* Function: Profiler::stopTimer()
* Purpose : Stop a timer variable
* Initial : Maxime Chevalier-Boisvert on July 21, 2009
****************************************************************
Revisions and bug fixes:
*/
void Profiler::stopTimer(TimerVar timerVar)
{
	// Ensure that the timer variable is valid
	assert (timerVar < NUM_TIMERS);
	
	// Ensure that the timer was started
	assert (s_curContext.timerRuns[timerVar] != 0);
	
	// If this is the first timer run for this timer
	if (s_curContext.timerRuns[timerVar] == 1)
	{	
		// Compute the time in seconds using microsecond information
		double timeSecs = getTimeSeconds();
		
		// Increment the timer by the difference from the last timer start
		s_curContext.timers[timerVar] += timeSecs - s_curContext.timerStart[timerVar];
	}
	
	// Decrement the number of timer runs for this timer
	s_curContext.timerRuns[timerVar] -= 1;
}

/***************************************************************
* Function: Profiler::getTimeSeconds()
* Purpose : Get the current time in seconds
* Initial : Maxime Chevalier-Boisvert on September 22, 2009
****************************************************************
Revisions and bug fixes:
*/
double Profiler::getTimeSeconds()
{
	// Create a timeval struct to store the time info
	struct timeval timeVal;
	
	// Get the current time
	gettimeofday(&timeVal, NULL);

	// Compute the time in seconds using microsecond information
	double timeSecs = timeVal.tv_sec + timeVal.tv_usec * 1.0e-6;
	
	// Return the time in seconds
	return timeSecs;
}

/***************************************************************
* Function: static Profiler::resetContextCmd()
* Purpose : Library function to reset the profiling context
* Initial : Maxime Chevalier-Boisvert on July 12, 2009
****************************************************************
Revisions and bug fixes:
*/
ArrayObj* Profiler::resetContextCmd(ArrayObj* pArguments)
{
	// Ensure that the argument count is valid
	if (pArguments->getSize() != 0)
		throw RunError("too many arguments");
	
	// If profiling is disabled, let the user know and exit
	#ifdef MCVM_DISABLE_PROFILING
	std::cout << "Profiling is currently disabled" << std::endl;
	return new ArrayObj();
	#endif
	
	// Replace the current profiling context, clearing all counters
	s_curContext = Context();
	
	// Return no output
	return new ArrayObj();
}

/***************************************************************
* Function: static Profiler::getInfoCmd()
* Purpose : Lib. function to get the prof. info as a cell array
* Initial : Maxime Chevalier-Boisvert on July 12, 2009
****************************************************************
Revisions and bug fixes:
*/
ArrayObj* Profiler::getInfoCmd(ArrayObj* pArguments)
{
	// Ensure that the argument count is valid
	if (pArguments->getSize() != 0)
		throw RunError("too many arguments");
	
	// If profiling is disabled, let the user know and exit
	#ifdef MCVM_DISABLE_PROFILING
	std::cout << "Profiling is currently disabled" << std::endl;
	return new ArrayObj();
	#endif
	
	// Create a cell array with two columns to store the profiling info
	CellArrayObj* pOutGrid = new CellArrayObj(NUM_COUNTERS + NUM_TIMERS, 2);	
	
	// For each counter variable
	for (size_t i = 0; i < NUM_COUNTERS; ++i)
	{
		// Write the counter variable name
		pOutGrid->setElem2D(i+1, 1, new CharArrayObj(COUNTER_VAR_NAMES[i]));
		
		// Write the counter value
		pOutGrid->setElem2D(i+1, 2, new MatrixF64Obj(s_curContext.counters[i]));
	}
	
	// For each counter variable
	for (size_t i = 0; i < NUM_TIMERS; ++i)
	{
		// Write the timer variable name
		pOutGrid->setElem2D(NUM_COUNTERS + i + 1, 1, new CharArrayObj(TIMER_VAR_NAMES[i] + " (s)"));
		
		// Write the timer value
		pOutGrid->setElem2D(NUM_COUNTERS + i + 1, 2, new MatrixF64Obj(s_curContext.timers[i]));
	}
	
	// Return the cell array
	return new ArrayObj(pOutGrid);
}

/***************************************************************
* Function: static Profiler::printInfoCmd()
* Purpose : Library function to print profiling information
* Initial : Maxime Chevalier-Boisvert on April 7, 2009
****************************************************************
Revisions and bug fixes:
*/
ArrayObj* Profiler::printInfoCmd(ArrayObj* pArguments)
{
	// Ensure that the argument count is valid
	if (pArguments->getSize() != 0)
		throw RunError("too many arguments");
	
	// If profiling is disabled, let the user know and exit
	#ifdef MCVM_DISABLE_PROFILING
	std::cout << "Profiling is currently disabled" << std::endl;
	return new ArrayObj();
	#endif
	
	// Log that we are about to print the counter variables
	std::cout << "Counter variables: " << std::endl;
	
	// For each counter variable
	for (size_t i = 0; i < NUM_COUNTERS; ++i)
	{	
		// Print the name and value of this counter variable
		std::cout << COUNTER_VAR_NAMES[i] << ": " << s_curContext.counters[i] << std::endl;
	}
	
	// For each timer variable
	for (size_t i = 0; i < NUM_TIMERS; ++i)
	{	
		// Print the name and value of this timer variable
		std::cout << TIMER_VAR_NAMES[i] << ": " << s_curContext.timers[i] << " s" << std::endl;
	}
	
	// Return no output
	return new ArrayObj();
}

/***************************************************************
* Function: Profiler::Context::Context()
* Purpose : Constructor for profiling context class
* Initial : Maxime Chevalier-Boisvert on April 7, 2009
****************************************************************
Revisions and bug fixes:
*/
Profiler::Context::Context()
{
	// Set all counters to 0
	memset(&counters, 0, sizeof(counters));
	
	// Set all timers to 0
	memset(&timers, 0, sizeof(timers));
	memset(&timerStart, 0, sizeof(timerStart));
	memset(&timerRuns, 0, sizeof(timerRuns));
}
