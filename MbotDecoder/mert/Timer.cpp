#include "Timer.h"
#include "Util.h"

double Timer::elapsed_time()
{
  time_t now;
  time(&now);
  return difftime(now, start_time);
}

double Timer::get_elapsed_time()
{
  return elapsed_time();
}

void Timer::start(const char* msg)
{
  // Print an optional message, something like "Starting timer t";
  if (msg) TRACE_ERR( msg << std::endl);

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
/*
inline void Timer::restart(const char* msg)
{
  // Print an optional message, something like "Restarting timer t";
  if (msg) TRACE_ERR( msg << std::endl;

  // Set the timer status to running
  running = true;

  // Set the accumulated time to 0 and the start time to now
  acc_time = 0;
  start_clock = clock();
  start_time = time(0);
}
*/

/***
 * Stop the timer and print an optional message.
 */
/*
inline void Timer::stop(const char* msg)
{
  // Print an optional message, something like "Stopping timer t";
  check(msg);

  // Recalculate and store the total accumulated time up until now
  if (running) acc_time += elapsed_time();

  running = false;
}
*/

void Timer::check(const char* msg)
{
  // Print an optional message, something like "Checking timer t";
  if (msg) TRACE_ERR( msg << " : ");

//  TRACE_ERR( "[" << std::setiosflags(std::ios::fixed) << std::setprecision(2) << (running ? elapsed_time() : 0) << "] seconds\n");
  TRACE_ERR( "[" << (running ? elapsed_time() : 0) << "] seconds\n");
}
