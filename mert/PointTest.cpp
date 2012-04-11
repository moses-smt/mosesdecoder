#include "Point.h"

#define BOOST_TEST_MODULE MertPoint
#include <boost/test/unit_test.hpp>

#include "Optimizer.h"

BOOST_AUTO_TEST_CASE(point_operators) {
  // Test operator '+'
  {
    Point p1, p2;
    Point p3 = p1 + p2;
    BOOST_CHECK_EQUAL(p3.GetScore(), kMaxFloat);
  }

  // Test operator '+='
  {
    Point p1, p2;
    p1 += p2;
    BOOST_CHECK_EQUAL(p1.GetScore(), kMaxFloat);
  }

  // Test operator '*'
  {
    Point p1;
    const Point p2 = p1 * 2.0;
    BOOST_CHECK_EQUAL(p2.GetScore(), kMaxFloat);
  }
}
