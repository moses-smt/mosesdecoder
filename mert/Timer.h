#ifndef TIMER_H
#define TIMER_H

#include <ctime>
#include <iostream>
#include <iomanip>

class Timer
{
  /**
   * Allow timers to be printed to ostreams using the syntax 'os << t'
   * for an ostream 'os' and a timer 't'.  For example, "cout << t" will
   * print out the total amount of time 't' has been "running".
   */
  friend std::ostream& operator<<(std::ostream& os, Timer& t);

private:
  bool running;
  time_t start_time;

  /**
   * Return the total time that the timer has been in the "running"
   * state since it was first "started" or last "restarted".  For
   * "short" time periods (less than an hour), the actual cpu time
   * used is reported instead of the elapsed time.
   * TODO in seconds?
   */
  double elapsed_time();

public:
  /**
   * 'running' is initially false. A timer needs to be explicitly started
   * using 'start' or 'restart'.
   */
  Timer() : running(false), start_time(0) { }

  /**
   * Start a timer.  If it is already running, let it continue running.
   * Print an optional message.
   */
  void start(const char* msg = 0);
//  void restart(const char* msg = 0);
//  void stop(const char* msg = 0);

  /**
   * Print out an optional message followed by the current timer timing.
   */
  void check(const char* msg = 0);

  /**
   * Return the total time that the timer has been in the "running"
   * state since it was first "started" or last "restarted".  For
   * "short" time periods (less than an hour), the actual cpu time
   * used is reported instead of the elapsed time.
   * This function is the public version of elapsed_time()
   */
  double get_elapsed_time();
};

inline std::ostream& operator<<(std::ostream& os, Timer& t)
{
  //os << std::setprecision(2) << std::setiosflags(std::ios::fixed) << (t.running ? t.elapsed_time() : 0);
  os << (t.running ? t.elapsed_time() : 0);
  return os;
}

#endif  // TIMER_H
