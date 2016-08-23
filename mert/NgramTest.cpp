#include "Ngram.h"

#define BOOST_TEST_MODULE MertNgram
#include <boost/test/unit_test.hpp>

using namespace MosesTuning;

BOOST_AUTO_TEST_CASE(ngram_basic)
{
  NgramCounts counts;
  NgramCounts::Key key;
  key.push_back(1);
  key.push_back(2);
  key.push_back(4);
  counts.Add(key);

  BOOST_REQUIRE(!counts.empty());
  BOOST_CHECK_EQUAL(counts.size(), 1);

  NgramCounts::const_iterator it = counts.find(key);
  BOOST_CHECK(it != counts.end());
  BOOST_CHECK_EQUAL(it->first.size(), key.size());
  for (std::size_t i = 0; i < key.size(); ++i) {
    BOOST_CHECK_EQUAL(it->first[i], key[i]);
  }
  BOOST_CHECK_EQUAL(it->second, 1);
}

BOOST_AUTO_TEST_CASE(ngram_Add)
{
  NgramCounts counts;
  NgramCounts::Key key;
  key.push_back(1);
  key.push_back(2);
  counts.Add(key);
  BOOST_REQUIRE(!counts.empty());
  BOOST_CHECK_EQUAL(counts[key], counts.get_default_count());

  NgramCounts::Key key2;
  key2.push_back(1);
  key2.push_back(2);
  counts.Add(key2);
  BOOST_CHECK_EQUAL(counts.size(), 1);
  BOOST_CHECK_EQUAL(counts[key], counts.get_default_count() + 1);
  BOOST_CHECK_EQUAL(counts[key2], counts.get_default_count() + 1);

  NgramCounts::Key key3;
  key3.push_back(10);
  counts.Add(key3);
  BOOST_CHECK_EQUAL(counts.size(), 2);
  BOOST_CHECK_EQUAL(counts[key3], counts.get_default_count());
}

BOOST_AUTO_TEST_CASE(ngram_lookup)
{
  NgramCounts counts;
  NgramCounts::Key key;
  key.push_back(1);
  key.push_back(2);
  key.push_back(4);
  counts.Add(key);

  {
    NgramCounts::Value v;
    BOOST_REQUIRE(counts.Lookup(key, &v));
    BOOST_CHECK_EQUAL(v, 1);
  }

  // the case the key is not found.
  {
    NgramCounts::Key key2;
    key2.push_back(0);
    key2.push_back(4);
    NgramCounts::Value v;
    // We only check the return value;
    // we don't check the value of "v" because it makes sense
    // to check the value when the specified ngram is found.
    BOOST_REQUIRE(!counts.Lookup(key2, &v));
  }

  // test after clear
  counts.clear();
  BOOST_CHECK(counts.empty());
  {
    NgramCounts::Value v;
    BOOST_CHECK(!counts.Lookup(key, &v));
  }
}
