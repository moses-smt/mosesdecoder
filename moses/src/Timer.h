#ifndef moses_Time_H
#define moses_Time_H

#include <ctime>
#include <iostream>
#include <iomanip>
#include "Util.h"

namespace Moses
{

/** Wrapper around time_t to time how long things have been running
 *  according to walltime. We avoid CPU time since it is less reliable
 *  in a multi-threaded environment and can spuriously include clock cycles
 *  used by other threads in the same process.
 */
class Timer
{
  friend std::ostream& operator<<(std::ostream& os, Timer& t);

private:
  bool running;
  // note: this only has the resolution of seconds, we'd often like better resolution
  // we make our best effort to do this on a system-by-system basis
#ifdef CLOCK_MONOTONIC
  struct timespec start_time;
#else
  time_t start_time;
#endif

  // in seconds
  double elapsed_time();

public:
  /***
   * 'running' is initially false.  A timer needs to be explicitly started
   * using 'start' or 'restart'
   */
  Timer() : running(false) {
#ifdef CLOCK_MONOTONIC
  start_time.tv_sec = 0;
  start_time.tv_nsec = 0;
#else
  start_time = 0;
#endif
  }

  void start(const char* msg = 0);
//  void restart(const char* msg = 0);
//  void stop(const char* msg = 0);
  void check(const char* msg = 0);
  double get_elapsed_time();

};

}

#endif
