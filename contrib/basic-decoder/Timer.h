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
  time_t start_time;

  // in seconds
  double elapsed_time();

public:
  /***
   * 'running' is initially false.  A timer needs to be explicitly started
   * using 'start' or 'restart'
   */
  Timer() : running(false) {
    start_time = 0;
  }

  void start(const char* msg = 0);
//  void restart(const char* msg = 0);
//  void stop(const char* msg = 0);
  void check(const char* msg = 0);
  double get_elapsed_time();

};

}

#endif
