#include "util/random.hh"

#define BOOST_TEST_MODULE RandomTest
#include <boost/test/unit_test.hpp>

namespace util
{
namespace
{

BOOST_AUTO_TEST_CASE(returns_different_consecutive_numbers)
{
  rand_int_init(99);
  const int first = rand_int(), second = rand_int(), third = rand_int();
  // Sometimes you'll get the same number twice in a row, but generally the
  // randomizer returns different numbers.
  BOOST_CHECK(second != first || third != first);
}

BOOST_AUTO_TEST_CASE(returns_different_numbers_for_different_seeds)
{
  rand_int_init(1);
  const int one1 = rand_int(), one2 = rand_int();
  rand_int_init(2);
  const int two1 = rand_int(), two2 = rand_int();
  BOOST_CHECK(two1 != one1 || two2 != two1);
}

BOOST_AUTO_TEST_CASE(returns_same_sequence_for_same_seed)
{
  rand_int_init(1);
  const int first = rand_int();
  rand_int_init(1);
  const int second = rand_int();
  BOOST_CHECK_EQUAL(first, second);
}

} // namespace
} // namespace util
