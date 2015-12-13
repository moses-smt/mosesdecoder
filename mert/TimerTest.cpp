#include "Timer.h"

#define BOOST_TEST_MODULE TimerTest
#include <boost/test/unit_test.hpp>

#include <string>
#include <unistd.h>

using namespace MosesTuning;

BOOST_AUTO_TEST_CASE(timer_basic_test)
{
  Timer timer;

  // Sleep time.  The test will sleep for this number of microseconds, and
  // expect the elapsed time to be noticeable.
  // Keep this number low to avoid wasting test time sleeping, but at least as
  // high as the Boost timer's resolution.  Tests must pass consistently, not
  // just on lucky runs.
#if defined(WIN32)
  // Timer resolution on Windows seems to be a millisecond.  Anything less and
  // the test fails consistently.
  const int sleep_time_microsec = 1000;
#else
  // Unix-like systems seem to have more fine-grained clocks.
  const int sleep_time_microsec = 40;
#endif

  timer.start();
  BOOST_REQUIRE(timer.is_running());
  BOOST_REQUIRE(usleep(sleep_time_microsec) == 0);
  BOOST_CHECK(timer.get_elapsed_wall_time() > 0.0);
  BOOST_CHECK(timer.get_elapsed_wall_time_microseconds() > 0);

  timer.restart();
  BOOST_REQUIRE(timer.is_running());
  BOOST_REQUIRE(usleep(sleep_time_microsec) == 0);
  BOOST_CHECK(timer.get_elapsed_wall_time() > 0.0);
  BOOST_CHECK(timer.get_elapsed_wall_time_microseconds() > 0);

  const std::string s = timer.ToString();
  BOOST_CHECK(!s.empty());
}
