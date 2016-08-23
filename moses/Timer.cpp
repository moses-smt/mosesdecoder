#include <iostream>
#include <iomanip>
#include "Util.h"
#include "Timer.h"
#include "StaticData.h"

#include "util/usage.hh"

namespace Moses
{

//global variable
Timer g_timer;


void ResetUserTime()
{
  g_timer.start();
};

void PrintUserTime(const std::string &message)
{
  g_timer.check(message.c_str());
}

double GetUserTime()
{
  return g_timer.get_elapsed_time();
}


/***
 * Return the total wall time that the timer has been in the "running"
 * state since it was first "started".
 */
double Timer::get_elapsed_time() const
{
  if (stopped) {
    return stop_time - start_time;
  }
  if (running) {
    return util::WallTime() - start_time;
  }
  return 0;
}

/***
 * Start a timer.  If it is already running, let it continue running.
 * Print an optional message.
 */
void Timer::start(const char* msg)
{
  // Print an optional message, something like "Starting timer t";
  if (msg) VERBOSE(1, msg << std::endl);

  // Return immediately if the timer is already running
  if (running && !stopped) return;

  // If stopped, recompute start time
  if (stopped) {
    start_time = util::WallTime() - (stop_time - start_time);
    stopped = false;
  } else {
    start_time = util::WallTime();
    running = true;
  }
}

/***
 * Stop a timer.
 * Print an optional message.
 */
void Timer::stop(const char* msg)
{
  // Print an optional message, something like "Stopping timer t";
  if (msg) VERBOSE(1, msg << std::endl);

  // Return immediately if the timer is not running
  if (stopped || !running) return;

  // Record stopped time
  stop_time = util::WallTime();

  // Change timer status to running
  stopped = true;
}

/***
 * Print out an optional message followed by the current timer timing.
 */
void Timer::check(const char* msg)
{
  // Print an optional message, something like "Checking timer t";
  if (msg) VERBOSE(1, msg << " : ");

//  VERBOSE(1, "[" << std::setiosflags(std::ios::fixed) << std::setprecision(2) << (running ? elapsed_time() : 0) << "] seconds\n");
  VERBOSE(1, "[" << (running ? get_elapsed_time() : 0) << "] seconds\n");
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

