#include "Util.h"

#define BOOST_TEST_MODULE UtilTest
#include <boost/test/unit_test.hpp>

using namespace MosesTuning;

BOOST_AUTO_TEST_CASE(util_get_next_pound_test)
{
  {
    std::string str("9 9 7 ");
    std::string substr;
    std::vector<std::string> res;

    while (!str.empty()) {
      getNextPound(str, substr);
      res.push_back(substr);
    }
    BOOST_REQUIRE(res.size() == 3);
    BOOST_CHECK_EQUAL("9", res[0]);
    BOOST_CHECK_EQUAL("9", res[1]);
    BOOST_CHECK_EQUAL("7", res[2]);
  }

  {
    std::string str("ref.0,ref.1,ref.2");
    std::string substr;
    std::vector<std::string> res;
    const std::string delim(",");

    while (!str.empty()) {
      getNextPound(str, substr, delim);
      res.push_back(substr);
    }
    BOOST_REQUIRE(res.size() == 3);
    BOOST_CHECK_EQUAL("ref.0", res[0]);
    BOOST_CHECK_EQUAL("ref.1", res[1]);
    BOOST_CHECK_EQUAL("ref.2", res[2]);
  }
}

BOOST_AUTO_TEST_CASE(util_tokenize_test)
{
  {
    std::vector<std::string> res;
    Tokenize("9 9 7", ' ', &res);
    BOOST_REQUIRE(res.size() == 3);
    BOOST_CHECK_EQUAL("9", res[0]);
    BOOST_CHECK_EQUAL("9", res[1]);
    BOOST_CHECK_EQUAL("7", res[2]);
  }

  {
    std::vector<std::string> res;
    Tokenize("9 8 7 ", ' ', &res);
    BOOST_REQUIRE(res.size() == 3);
    BOOST_CHECK_EQUAL("9", res[0]);
    BOOST_CHECK_EQUAL("8", res[1]);
    BOOST_CHECK_EQUAL("7", res[2]);
  }

  {
    std::vector<std::string> res;
    Tokenize("ref.0,ref.1,", ',', &res);
    BOOST_REQUIRE(res.size() == 2);
    BOOST_CHECK_EQUAL("ref.0", res[0]);
    BOOST_CHECK_EQUAL("ref.1", res[1]);
  }
}

BOOST_AUTO_TEST_CASE(util_ends_with_test)
{
  BOOST_CHECK(EndsWith("abc:", ":"));
  BOOST_CHECK(EndsWith("a b c:", ":"));
  BOOST_CHECK(!EndsWith("a", ":"));
  BOOST_CHECK(!EndsWith("a:b", ":"));

  BOOST_CHECK(EndsWith("ab ", " "));
  BOOST_CHECK(!EndsWith("ab", " "));
  BOOST_CHECK(!EndsWith("a b", " "));
}
