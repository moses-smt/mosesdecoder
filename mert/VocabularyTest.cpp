#include "Vocabulary.h"

#define BOOST_TEST_MODULE MertVocabulary
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(vocab_basic) {
  mert::Vocabulary vocab;
  BOOST_REQUIRE(vocab.empty());
  vocab.clear();

  BOOST_CHECK_EQUAL(0, vocab.Encode("hello"));
  BOOST_CHECK_EQUAL(0, vocab.Encode("hello"));
  BOOST_CHECK_EQUAL(1, vocab.Encode("world"));

  BOOST_CHECK_EQUAL(2, vocab.size());

  int v;
  BOOST_CHECK(vocab.Lookup("hello", &v));
  BOOST_CHECK_EQUAL(0, v);
  BOOST_CHECK(vocab.Lookup("world", &v));
  BOOST_CHECK_EQUAL(1, v);

  BOOST_CHECK(!vocab.Lookup("java", &v));

  vocab.clear();
  BOOST_CHECK(!vocab.Lookup("hello", &v));
  BOOST_CHECK(!vocab.Lookup("world", &v));
}
