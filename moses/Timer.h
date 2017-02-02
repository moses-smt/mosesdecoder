#ifndef moses_Time_H
#define moses_Time_H

#include <ctime>
#include <iostream>
#include <iomanip>
#include "Util.h"

namespace Moses
{

// A couple of utilities to measure decoding time

/** Start global timer. */
void ResetUserTime();

/** Print out an optional message followed by the current global timer timing. */
void PrintUserTime(const std::string &message);

/**
 * Total wall time that the global timer has been in the "running"
 * state since it was first "started".
 */
double GetUserTime();


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
  bool stopped;
  double start_time;
  double stop_time;

public:
  /***
   * 'running' is initially false.  A timer needs to be explicitly started
   * using 'start'
   */
  Timer() : running(false),stopped(false) {
    start_time = 0;
  }

  void start(const char* msg = 0);
  void stop(const char* msg = 0);
  void check(const char* msg = 0);
  double get_elapsed_time() const;
};

}

#endif
