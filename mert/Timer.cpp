#include "Timer.h"
#include "Util.h"

double Timer::elapsed_time()
{
  time_t now;
  time(&now);
  return difftime(now, m_start_time);
}

double Timer::get_elapsed_time()
{
  return elapsed_time();
}

void Timer::start(const char* msg)
{
  // Print an optional message, something like "Starting timer t";
  if (msg) TRACE_ERR( msg << std::endl);
  if (m_is_running) return;
  m_is_running = true;

  // Set the start time;
  time(&m_start_time);
}

void Timer::check(const char* msg)
{
  // Print an optional message, something like "Checking timer t";
  if (msg) TRACE_ERR( msg << " : ");
  TRACE_ERR( "[" << (m_is_running ? elapsed_time() : 0) << "] seconds\n");
}
