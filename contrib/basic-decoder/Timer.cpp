#include <iostream>
#include "Timer.h"

using namespace std;

namespace Moses
{

void Timer::start(const char* msg)
{
  // Print an optional message, something like "Starting timer t";
  if (msg) {
    cerr << msg << std::endl;
  }

  // Return immediately if the timer is already running
  if (running) return;

  // Change timer status to running
  running = true;

  // Set the start time;
  time(&start_time);
}

void Timer::check(const char* msg)
{
  // Print an optional message, something like "Checking timer t";
  if (msg) {
    cerr << msg << " : ";
  }

  //  TRACE_ERR( "[" << std::setiosflags(std::ios::fixed) << std::setprecision(2) << (running ? elapsed_time() : 0) << "] seconds\n");
  cerr << "[" << (running ? elapsed_time() : 0) << "] seconds\n";
}

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

}

