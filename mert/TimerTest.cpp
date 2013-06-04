#include "Timer.h"

#define BOOST_TEST_MODULE TimerTest
#include <boost/test/unit_test.hpp>

#include <string>
#include <unistd.h>

using namespace MosesTuning;

BOOST_AUTO_TEST_CASE(timer_basic_test)
{
  Timer timer;
  const int sleep_time_microsec = 40; // ad-hoc microseconds to pass unit tests.

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
