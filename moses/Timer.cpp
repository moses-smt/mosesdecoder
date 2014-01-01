#include <iostream>
#include <iomanip>
#include "Util.h"
#include "Timer.h"

#include "util/usage.hh"

namespace Moses
{

/***
 * Return the total wall time that the timer has been in the "running"
 * state since it was first "started".  
 */
double Timer::get_elapsed_time()
{
  return util::WallTime() - start_time;
}

/***
 * Start a timer.  If it is already running, let it continue running.
 * Print an optional message.
 */
void Timer::start(const char* msg)
{
  // Print an optional message, something like "Starting timer t";
  if (msg) TRACE_ERR( msg << std::endl);

  // Return immediately if the timer is already running
  if (running) return;

  // Change timer status to running
  running = true;

  start_time = util::WallTime();
}

/***
 * Print out an optional message followed by the current timer timing.
 */
void Timer::check(const char* msg)
{
  // Print an optional message, something like "Checking timer t";
  if (msg) TRACE_ERR( msg << " : ");

//  TRACE_ERR( "[" << std::setiosflags(std::ios::fixed) << std::setprecision(2) << (running ? elapsed_time() : 0) << "] seconds\n");
  TRACE_ERR( "[" << (running ? get_elapsed_time() : 0) << "] seconds\n");
}

/***
 * Allow timers to be printed to ostreams using the syntax 'os << t'
 * for an ostream 'os' and a timer 't'.  For example, "cout << t" will
 * print out the total amount of time 't' has been "running".
 */
std::ostream& operator<<(std::ostream& os, Timer& t)
{
  //os << std::setprecision(2) << std::setiosflags(std::ios::fixed) << (t.running ? t.elapsed_time() : 0);
  os << (t.running ? t.get_elapsed_time() : 0);
  return os;
}

}

