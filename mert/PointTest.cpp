#include "Point.h"

#define BOOST_TEST_MODULE MertPoint
#include <boost/test/unit_test.hpp>

#include "Optimizer.h"
#include "Util.h"

using namespace std;
using namespace MosesTuning;

BOOST_AUTO_TEST_CASE(point_operators)
{
  const unsigned int dim = 5;
  vector<float> init(dim);
  init[0] = 1.0f;
  init[1] = 1.0f;
  init[2] = 0.3f;
  init[3] = 0.2f;
  init[4] = 0.3f;

  vector<float> min(dim, 0.0f);
  vector<float> max(dim, 0.0f);

  Point::setdim(dim);
  BOOST_REQUIRE(dim == Point::getdim());

  // Test operator '+'
  {
    Point p1(init, min, max);
    Point p2(init, min, max);
    Point p3 = p1 + p2;
    for (size_t i = 0; i < p3.size(); ++i) {
      BOOST_CHECK(IsAlmostEqual(init[i] * 2.0f, p3[i]));
    }
    BOOST_CHECK_EQUAL(p3.GetScore(), kMaxFloat);
  }

  // Test operator '+='
  {
    Point p1(init, min, max);
    Point p2(init, min, max);
    p1 += p2;

    for (size_t i = 0; i < p1.size(); ++i) {
      BOOST_CHECK(IsAlmostEqual(init[i] * 2.0f, p1[i]));
    }
    BOOST_CHECK_EQUAL(p1.GetScore(), kMaxFloat);
  }

  // Test operator '*'
  {
    Point p1(init, min, max);
    const Point p2 = p1 * 2.0;

    BOOST_REQUIRE(p1.size() == p2.size());
    for (size_t i = 0; i < p2.size(); ++i) {
      BOOST_CHECK(IsAlmostEqual(init[i] * 2.0f, p2[i]));
    }
    BOOST_CHECK_EQUAL(p2.GetScore(), kMaxFloat);
  }
}
