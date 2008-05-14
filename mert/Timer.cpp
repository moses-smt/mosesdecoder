/*
 *  Timer.cpp
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 8/31/06.
 *  Copyright 2006 ITC-irst. All rights reserved.
 *
 */

#include "Timer.h"

/***
* Return the total time that the timer has been in the "running"
* state since it was first "started" or last "restarted".  For
* "short" time periods (less than an hour), the actual cpu time
* used is reported instead of the elapsed time.
*/

double Timer::elapsed_time()
{
  time_t now;
  time(&now);
  return difftime(now, start_time);
}

/***
* Start a timer.
* If it is already running, let it continue running.
* Print an optional message.
*/
void Timer::start(const char* msg)
{
  // Print an optional message, something like "Starting timer t";
  if (msg) TRACE_ERR(msg << std::endl);
	
  // Return immediately if the timer is already running
  if (running) return;
	
  // Change timer status to running
  running = true;
	
  // Set the start time;
  time(&start_time);
}

/***
* Turn the timer off and start it again from 0.  Print an optional message.
*/
void Timer::restart(const char* msg)
{
	// Print an optional message, something like "Restarting timer t";
	 if (msg) TRACE_ERR(msg << std::endl);
	 
	 // Set the timer status to running
	 running = true;
	 
	 // Re-Set the start time;
  time(&start_time);
}

/***
* Stop the timer and print an optional message.
*/
void Timer::stop(const char* msg)
{
	 // Print an optional message, something like "Stopping timer t";
	 check(msg);
	 
	 running = false;
}

/***
* Print out an optional message followed by the current timer timing.
*/
void Timer::check(const char* msg)
{
  // Print an optional message, something like "Checking timer t";
  if (msg) TRACE_ERR(msg << " : ");
	
  TRACE_ERR("[" << std::setiosflags(std::ios::fixed) << std::setprecision(2) << (running ? elapsed_time() : 0) << "] seconds\n\n");
}


/***
* Allow timers to be printed to ostreams using the syntax 'os << t'
* for an ostream 'os' and a timer 't'.  For example, "cout << t" will
* print out the total amount of time 't' has been "running".
*/
std::ostream& operator<<(std::ostream& os, Timer& t)
{
#ifdef TRACE_ENABLE
  os << std::setprecision(2) << std::setiosflags(std::ios::fixed) << (t.running ? t.elapsed_time() : 0);
#endif
  return os;
}
