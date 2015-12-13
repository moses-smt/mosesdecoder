#include "util/tokenize.hh"

#define BOOST_TEST_MODULE TokenizeTest
#include <boost/test/unit_test.hpp>

namespace util
{
namespace
{

BOOST_AUTO_TEST_CASE(empty_text_yields_empty_vector)
{
  const std::vector<std::string> tokens = util::tokenize("");
  BOOST_CHECK_EQUAL(tokens.size(), 0);
}

BOOST_AUTO_TEST_CASE(whitespace_only_yields_empty_vector)
{
  const std::vector<std::string> tokens = util::tokenize(" ");
  BOOST_CHECK_EQUAL(tokens.size(), 0);
}

BOOST_AUTO_TEST_CASE(parses_single_token)
{
  const std::vector<std::string> tokens = util::tokenize("mytoken");
  BOOST_CHECK_EQUAL(tokens.size(), 1);
  BOOST_CHECK_EQUAL(tokens[0], "mytoken");
}

BOOST_AUTO_TEST_CASE(ignores_leading_whitespace)
{
  const std::vector<std::string> tokens = util::tokenize(" \t mytoken");
  BOOST_CHECK_EQUAL(tokens.size(), 1);
  BOOST_CHECK_EQUAL(tokens[0], "mytoken");
}

BOOST_AUTO_TEST_CASE(ignores_trailing_whitespace)
{
  const std::vector<std::string> tokens = util::tokenize("mytoken \t ");
  BOOST_CHECK_EQUAL(tokens.size(), 1);
  BOOST_CHECK_EQUAL(tokens[0], "mytoken");
}

BOOST_AUTO_TEST_CASE(splits_tokens_on_tabs)
{
  const std::vector<std::string> tokens = util::tokenize("one\ttwo");
  BOOST_CHECK_EQUAL(tokens.size(), 2);
  BOOST_CHECK_EQUAL(tokens[0], "one");
  BOOST_CHECK_EQUAL(tokens[1], "two");
}

BOOST_AUTO_TEST_CASE(splits_tokens_on_spaces)
{
  const std::vector<std::string> tokens = util::tokenize("one two");
  BOOST_CHECK_EQUAL(tokens.size(), 2);
  BOOST_CHECK_EQUAL(tokens[0], "one");
  BOOST_CHECK_EQUAL(tokens[1], "two");
}

BOOST_AUTO_TEST_CASE(treats_sequence_of_space_as_one_space)
{
  const std::vector<std::string> tokens = util::tokenize("one\t  \ttwo");
  BOOST_CHECK_EQUAL(tokens.size(), 2);
  BOOST_CHECK_EQUAL(tokens[0], "one");
  BOOST_CHECK_EQUAL(tokens[1], "two");
}

} // namespace
} // namespace util
