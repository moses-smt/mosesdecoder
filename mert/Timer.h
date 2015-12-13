#ifndef MERT_TIMER_H_
#define MERT_TIMER_H_

#include <ostream>
#include <string>
#include <stdint.h>

namespace MosesTuning
{


class Timer
{
private:
  // Time values are stored in microseconds.
  struct CPUTime {
    uint64_t user_time;                 // user CPU time
    uint64_t sys_time;                  // system CPU time

    CPUTime() : user_time(0), sys_time(0) { }
  };

  void GetCPUTimeMicroSeconds(CPUTime* cpu_time) const;

  bool m_is_running;
  uint64_t m_wall;                      // wall-clock time in microseconds
  CPUTime m_start_time;

  // No copying allowed
  Timer(const Timer&);
  void operator=(const Timer&);

public:
  /**
   * 'm_is_running' is initially false. A timer needs to be explicitly started
   * using 'start'.
   */
  Timer()
    : m_is_running(false),
      m_wall(0),
      m_start_time() {}

  ~Timer() {}

  /**
   * Start a timer.  If it is already running, let it continue running.
   * Print an optional message.
   */
  void start(const char* msg = 0);

  /**
   * Restart the timer iff the timer is already running.
   * if the timer is not running, just start the timer.
   */
  void restart(const char* msg = 0);

  /**
   * Print out an optional message followed by the current timer timing.
   */
  void check(const char* msg = 0);

  /**
   */
  bool is_running() const {
    return m_is_running;
  }

  /**
   * Return the total time in seconds that the timer has been in the
   * "running" state since it was first "started" or last "restarted".
   * For "short" time periods (less than an hour), the actual cpu time
   * used is reported instead of the elapsed time.
   */
  double get_elapsed_cpu_time() const;

  /**
   * Return the total time in microseconds.
   */
  uint64_t get_elapsed_cpu_time_microseconds() const;

  /**
   * Get elapsed wall-clock time in seconds.
   */
  double get_elapsed_wall_time() const;

  /**
   * Get elapsed wall-clock time in microseconds.
   */
  uint64_t get_elapsed_wall_time_microseconds() const;

  /**
   * Return a string that has the user CPU time, system time, and total time.
   */
  std::string ToString() const;
};

/**
 * Allow timers to be printed to ostreams using the syntax 'os << t'
 * for an ostream 'os' and a timer 't'.  For example, "cout << t" will
 * print out the total amount of time 't' has been "running".
 */
inline std::ostream& operator<<(std::ostream& os, const Timer& t)
{
  if (t.is_running()) {
    os << t.ToString();
  } else {
    os << "timer is not running.";
  }
  return os;
}

}


#endif  // MERT_TIMER_H_
