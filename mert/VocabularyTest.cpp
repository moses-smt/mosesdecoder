#include "Vocabulary.h"
#include "Singleton.h"

#define BOOST_TEST_MODULE MertVocabulary
#include <boost/test/unit_test.hpp>

using namespace MosesTuning;

namespace mert
{
namespace
{

void TearDown()
{
  Singleton<Vocabulary>::Delete();
}

} // namespace

BOOST_AUTO_TEST_CASE(vocab_basic)
{
  Vocabulary vocab;
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

BOOST_AUTO_TEST_CASE(vocab_factory_test)
{
  Vocabulary* vocab1 = VocabularyFactory::GetVocabulary();
  Vocabulary* vocab2 = VocabularyFactory::GetVocabulary();
  Vocabulary* vocab3 = VocabularyFactory::GetVocabulary();

  BOOST_REQUIRE(vocab1 != NULL);
  BOOST_CHECK(vocab1 == vocab2);
  BOOST_CHECK(vocab2 == vocab3);

  TearDown();
}
} // namespace mert
