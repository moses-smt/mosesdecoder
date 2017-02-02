#include "Reference.h"

#define BOOST_TEST_MODULE MertReference
#include <boost/test/unit_test.hpp>

using namespace MosesTuning;

BOOST_AUTO_TEST_CASE(refernece_count)
{
  Reference ref;
  BOOST_CHECK(ref.get_counts() != NULL);
}

BOOST_AUTO_TEST_CASE(refernece_length_iterator)
{
  Reference ref;
  ref.push_back(4);
  ref.push_back(2);
  BOOST_REQUIRE(ref.num_references() == 2);

  Reference::iterator it = ref.begin();
  BOOST_CHECK_EQUAL(*it, 4);
  ++it;
  BOOST_CHECK_EQUAL(*it, 2);
  ++it;
  BOOST_CHECK(it == ref.end());
}

BOOST_AUTO_TEST_CASE(refernece_length_average)
{
  {
    Reference ref;
    ref.push_back(4);
    ref.push_back(1);
    BOOST_CHECK_EQUAL(2, ref.CalcAverage());
  }

  {
    Reference ref;
    ref.push_back(4);
    ref.push_back(3);
    BOOST_CHECK_EQUAL(3, ref.CalcAverage());
  }

  {
    Reference ref;
    ref.push_back(4);
    ref.push_back(3);
    ref.push_back(4);
    ref.push_back(5);
    BOOST_CHECK_EQUAL(4, ref.CalcAverage());
  }
}

BOOST_AUTO_TEST_CASE(refernece_length_closest)
{
  {
    Reference ref;
    ref.push_back(4);
    ref.push_back(1);
    BOOST_REQUIRE(ref.num_references() == 2);

    BOOST_CHECK_EQUAL(1, ref.CalcClosest(2));
    BOOST_CHECK_EQUAL(1, ref.CalcClosest(1));
    BOOST_CHECK_EQUAL(4, ref.CalcClosest(3));
    BOOST_CHECK_EQUAL(4, ref.CalcClosest(4));
    BOOST_CHECK_EQUAL(4, ref.CalcClosest(5));
  }

  {
    Reference ref;
    ref.push_back(4);
    ref.push_back(3);
    BOOST_REQUIRE(ref.num_references() == 2);

    BOOST_CHECK_EQUAL(3, ref.CalcClosest(1));
    BOOST_CHECK_EQUAL(3, ref.CalcClosest(2));
    BOOST_CHECK_EQUAL(3, ref.CalcClosest(3));
    BOOST_CHECK_EQUAL(4, ref.CalcClosest(4));
    BOOST_CHECK_EQUAL(4, ref.CalcClosest(5));
  }

  {
    Reference ref;
    ref.push_back(4);
    ref.push_back(3);
    ref.push_back(4);
    ref.push_back(5);
    BOOST_REQUIRE(ref.num_references() == 4);

    BOOST_CHECK_EQUAL(3, ref.CalcClosest(1));
    BOOST_CHECK_EQUAL(3, ref.CalcClosest(2));
    BOOST_CHECK_EQUAL(3, ref.CalcClosest(3));
    BOOST_CHECK_EQUAL(4, ref.CalcClosest(4));
    BOOST_CHECK_EQUAL(5, ref.CalcClosest(5));
  }
}

BOOST_AUTO_TEST_CASE(refernece_length_shortest)
{
  {
    Reference ref;
    ref.push_back(4);
    ref.push_back(1);
    BOOST_CHECK_EQUAL(1, ref.CalcShortest());
  }

  {
    Reference ref;
    ref.push_back(4);
    ref.push_back(3);
    BOOST_CHECK_EQUAL(3, ref.CalcShortest());
  }

  {
    Reference ref;
    ref.push_back(4);
    ref.push_back(3);
    ref.push_back(4);
    ref.push_back(5);
    BOOST_CHECK_EQUAL(3, ref.CalcShortest());
  }
}
