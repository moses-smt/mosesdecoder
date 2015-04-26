#include <cstdlib>

#include "util/random.hh"

#define BOOST_TEST_MODULE RandomTest
#include <boost/test/unit_test.hpp>

namespace util
{
namespace
{

BOOST_AUTO_TEST_CASE(rand_int_returns_positive_no_greater_than_RAND_MAX)
{
  rand_init();
  for (int i=0; i<100; i++)
  {
    const int random_number = rand<int>();
    BOOST_CHECK(random_number >= 0);
    BOOST_CHECK(random_number <= RAND_MAX);
  }
}

BOOST_AUTO_TEST_CASE(rand_int_returns_different_consecutive_numbers)
{
  rand_init(99);
  const int first = rand<int>(), second = rand<int>(), third = rand<int>();
  // Sometimes you'll get the same number twice in a row, but generally the
  // randomizer returns different numbers.
  BOOST_CHECK(second != first || third != first);
}

BOOST_AUTO_TEST_CASE(rand_int_returns_different_numbers_for_different_seeds)
{
  rand_init(1);
  const int one1 = rand<int>(), one2 = rand<int>();
  rand_init(2);
  const int two1 = rand<int>(), two2 = rand<int>();
  BOOST_CHECK(two1 != one1 || two2 != one2);
}

BOOST_AUTO_TEST_CASE(rand_int_returns_same_sequence_for_same_seed)
{
  rand_init(1);
  const int first = rand<int>();
  rand_init(1);
  const int second = rand<int>();
  BOOST_CHECK_EQUAL(first, second);
}

BOOST_AUTO_TEST_CASE(rand_excl_int_returns_number_in_range)
{
  const int bottom = 10, top = 50;
  for (int i=0; i<100; i++)
  {
    const int random_number = rand_excl(bottom, top);
    BOOST_CHECK(random_number >= bottom);
    BOOST_CHECK(random_number < top);
  }
}

BOOST_AUTO_TEST_CASE(rand_excl_int_covers_full_range)
{
  // The spread of random numbers really goes all the way from 0 (inclusive)
  // to "top" (exclusive).  It's not some smaller subset.
  // This test will randomly fail sometimes, though very very rarely, when the
  // random numbers don't actually have enough different values.
  const int bottom = 1, top = 4;
  int lowest = 99, highest = -1;
  for (int i=0; i<100; i++)
  {
    const int random_number = rand_excl(bottom, top);
    lowest = std::min(lowest, random_number);
    highest = std::max(highest, random_number);
  }

  BOOST_CHECK_EQUAL(lowest, bottom);
  BOOST_CHECK_EQUAL(highest, top - 1);
}

BOOST_AUTO_TEST_CASE(rand_incl_int_returns_number_in_range)
{
  const int bottom = 10, top = 50;
  for (int i=0; i<100; i++)
  {
    const int random_number = rand_incl(bottom, top);
    BOOST_CHECK(random_number >= 0);
    BOOST_CHECK(random_number <= top);
  }
}

BOOST_AUTO_TEST_CASE(rand_incl_int_covers_full_range)
{
  // The spread of random numbers really goes all the way from 0 to "top"
  // inclusive.  It's not some smaller subset.
  // This test will randomly fail sometimes, though very very rarely, when the
  // random numbers don't actually have enough different values.
  const int bottom = 1, top = 4;
  int lowest = 99, highest = -1;
  for (int i=0; i<100; i++)
  {
    const int random_number = rand_incl(bottom, top);
    lowest = std::min(lowest, random_number);
    highest = std::max(highest, random_number);
  }

  BOOST_CHECK_EQUAL(lowest, bottom);
  BOOST_CHECK_EQUAL(highest, top);
}

BOOST_AUTO_TEST_CASE(rand_excl_float_returns_float_in_range)
{
  const float bottom = 5, top = 10;
  for (int i=0; i<100; i++)
  {
    const float random_number = rand_excl(bottom, top);
    BOOST_CHECK(random_number >= bottom);
    BOOST_CHECK(random_number < top);
  }
}

BOOST_AUTO_TEST_CASE(rand_excl_float_returns_different_values)
{
  const float bottom = 5, top = 10;
  float lowest = 99, highest = -1;
  for (int i=0; i<10; i++)
  {
    const float random_number = rand_excl(bottom, top);
    lowest = std::min(lowest, random_number);
    highest = std::max(highest, random_number);
  }
  BOOST_CHECK(lowest < highest);
}

BOOST_AUTO_TEST_CASE(rand_float_incl_returns_float_in_range)
{
  const float bottom = 5, top = 10;
  for (int i=0; i<1000; i++)
  {
    const float random_number = rand_excl(bottom, top);
    BOOST_CHECK(random_number >= bottom);
    BOOST_CHECK(random_number <= top);
  }
}

BOOST_AUTO_TEST_CASE(rand_float_incl_returns_different_values)
{
  const float bottom = 0, top = 10;
  float lowest = 99, highest = -1;
  for (int i=0; i<10; i++)
  {
    const float random_number = rand_excl(bottom, top);
    lowest = std::min(lowest, random_number);
    highest = std::max(highest, random_number);
  }
  BOOST_CHECK(lowest < highest);
}

BOOST_AUTO_TEST_CASE(wide_rand_int_returns_different_numbers_in_range)
{
  for (int i=0; i<100; i++)
  {
    const int random_number = wide_rand<int>();
    BOOST_CHECK(random_number >= 0);
    BOOST_CHECK(random_number <= RAND_MAX);
  }
}

BOOST_AUTO_TEST_CASE(wide_rand_long_long_returns_big_numbers)
{
  long long one = wide_rand<long long>(), two = wide_rand<long long>();
  // This test will fail sometimes because of unlucky random numbers, but only
  // very very rarely.
  BOOST_CHECK(one > RAND_MAX || two > RAND_MAX);
}

BOOST_AUTO_TEST_CASE(wide_rand_excl_supports_larger_range)
{
  const long long top = 1000 * (long long)RAND_MAX;
  long long
    one = wide_rand_excl<long long>(top),
    two = wide_rand_excl<long long>(top);
  BOOST_CHECK(one < top);
  BOOST_CHECK(two < top);
  // This test will fail sometimes because of unlucky random numbers, but only
  // very very rarely.
  BOOST_CHECK(one > RAND_MAX || two > RAND_MAX);
}

} // namespace
} // namespace util
