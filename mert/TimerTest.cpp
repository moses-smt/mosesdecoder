#include "Timer.h"

#define BOOST_TEST_MODULE TimerTest
#include <boost/test/unit_test.hpp>

#include <string>
#include <iostream>

BOOST_AUTO_TEST_CASE(timer_basic_test) {
  Timer timer;
  timer.start();
  BOOST_REQUIRE(timer.is_running());
  BOOST_REQUIRE(timer.get_elapsed_cpu_time() > 0.0);
  BOOST_REQUIRE(timer.get_elapsed_cpu_time_microseconds() > 0);
  BOOST_REQUIRE(timer.get_elapsed_wall_time() > 0.0);
  BOOST_REQUIRE(timer.get_elapsed_wall_time_microseconds() > 0);

  timer.restart();
  BOOST_REQUIRE(timer.is_running());
  BOOST_REQUIRE(timer.get_elapsed_cpu_time() > 0.0);
  BOOST_REQUIRE(timer.get_elapsed_cpu_time_microseconds() > 0);
  BOOST_REQUIRE(timer.get_elapsed_wall_time() > 0.0);
  BOOST_REQUIRE(timer.get_elapsed_wall_time_microseconds() > 0);

  const std::string s = timer.ToString();
  BOOST_REQUIRE(!s.empty());
}
