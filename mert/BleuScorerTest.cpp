#include "BleuScorer.h"

#define BOOST_TEST_MODULE MertBleuScorer
#include <boost/test/unit_test.hpp>

#include "Ngram.h"
#include "Vocabulary.h"
#include "Util.h"

namespace {

NgramCounts* g_counts = NULL;

NgramCounts* GetNgramCounts() {
  assert(g_counts);
  return g_counts;
}

void SetNgramCounts(NgramCounts* counts) {
  g_counts = counts;
}

struct Unigram {
  Unigram(const std::string& a) {
    instance.push_back(mert::VocabularyFactory::GetVocabulary()->Encode(a));
  }
  NgramCounts::Key instance;
};

struct Bigram {
  Bigram(const std::string& a, const std::string& b) {
    instance.push_back(mert::VocabularyFactory::GetVocabulary()->Encode(a));
    instance.push_back(mert::VocabularyFactory::GetVocabulary()->Encode(b));
  }
  NgramCounts::Key instance;
};

struct Trigram {
  Trigram(const std::string& a, const std::string& b, const std::string& c) {
    instance.push_back(mert::VocabularyFactory::GetVocabulary()->Encode(a));
    instance.push_back(mert::VocabularyFactory::GetVocabulary()->Encode(b));
    instance.push_back(mert::VocabularyFactory::GetVocabulary()->Encode(c));
  }
  NgramCounts::Key instance;
};

struct Fourgram {
  Fourgram(const std::string& a, const std::string& b,
           const std::string& c, const std::string& d) {
    instance.push_back(mert::VocabularyFactory::GetVocabulary()->Encode(a));
    instance.push_back(mert::VocabularyFactory::GetVocabulary()->Encode(b));
    instance.push_back(mert::VocabularyFactory::GetVocabulary()->Encode(c));
    instance.push_back(mert::VocabularyFactory::GetVocabulary()->Encode(d));
  }
  NgramCounts::Key instance;
};

bool CheckUnigram(const std::string& str) {
  Unigram unigram(str);
  NgramCounts::Value v;
  return GetNgramCounts()->Lookup(unigram.instance, &v);
}

bool CheckBigram(const std::string& a, const std::string& b) {
  Bigram bigram(a, b);
  NgramCounts::Value v;
  return GetNgramCounts()->Lookup(bigram.instance, &v);
}

bool CheckTrigram(const std::string& a, const std::string& b,
                  const std::string& c) {
  Trigram trigram(a, b, c);
  NgramCounts::Value v;
  return GetNgramCounts()->Lookup(trigram.instance, &v);
}

bool CheckFourgram(const std::string& a, const std::string& b,
                   const std::string& c, const std::string& d) {
  Fourgram fourgram(a, b, c, d);
  NgramCounts::Value v;
  return GetNgramCounts()->Lookup(fourgram.instance, &v);
}

} // namespace

BOOST_AUTO_TEST_CASE(bleu_reference_type) {
  BleuScorer scorer;
  // BleuScorer will use "closest" by default.
  BOOST_CHECK_EQUAL(scorer.GetReferenceLengthType(), BleuScorer::CLOSEST);

  scorer.SetReferenceLengthType(BleuScorer::AVERAGE);
  BOOST_CHECK_EQUAL(scorer.GetReferenceLengthType(), BleuScorer::AVERAGE);

  scorer.SetReferenceLengthType(BleuScorer::SHORTEST);
  BOOST_CHECK_EQUAL(scorer.GetReferenceLengthType(), BleuScorer::SHORTEST);
}

BOOST_AUTO_TEST_CASE(bleu_count_ngrams) {
  BleuScorer scorer;

  std::string line = "I saw a girl with a telescope .";
  // In the above string, we will get the 25 ngrams.
  //
  // unigram: "I", "saw", "a", "girl", "with", "telescope", "."
  // bigram:  "I saw", "saw a", "a girl", "girl with", "with a", "a telescope"
  //          "telescope ."
  // trigram: "I saw a", "saw a girl", "a girl with", "girl with a",
  //          "with a telescope", "a telescope ."
  // 4-gram:  "I saw a girl", "saw a girl with", "a girl with a",
  //          "girl with a telescope", "with a telescope ."
  NgramCounts counts;
  BOOST_REQUIRE(scorer.CountNgrams(line, counts, kBleuNgramOrder) == 8);
  BOOST_CHECK_EQUAL(25, counts.size());

  mert::Vocabulary* vocab = scorer.GetVocab();
  BOOST_CHECK_EQUAL(7, vocab->size());

  std::vector<std::string> res;
  Tokenize(line.c_str(), ' ', &res);
  std::vector<int> ids(res.size());
  for (std::size_t i = 0; i < res.size(); ++i) {
    BOOST_CHECK(vocab->Lookup(res[i], &ids[i]));
  }

  SetNgramCounts(&counts);

  // unigram
  for (std::size_t i = 0; i < res.size(); ++i) {
    BOOST_CHECK(CheckUnigram(res[i]));
  }

  // bigram
  BOOST_CHECK(CheckBigram("I", "saw"));
  BOOST_CHECK(CheckBigram("saw", "a"));
  BOOST_CHECK(CheckBigram("a", "girl"));
  BOOST_CHECK(CheckBigram("girl", "with"));
  BOOST_CHECK(CheckBigram("with", "a"));
  BOOST_CHECK(CheckBigram("a", "telescope"));
  BOOST_CHECK(CheckBigram("telescope", "."));

  // trigram
  BOOST_CHECK(CheckTrigram("I", "saw", "a"));
  BOOST_CHECK(CheckTrigram("saw", "a", "girl"));
  BOOST_CHECK(CheckTrigram("a", "girl", "with"));
  BOOST_CHECK(CheckTrigram("girl", "with", "a"));
  BOOST_CHECK(CheckTrigram("with", "a", "telescope"));
  BOOST_CHECK(CheckTrigram("a", "telescope", "."));

  // 4-gram
  BOOST_CHECK(CheckFourgram("I", "saw", "a", "girl"));
  BOOST_CHECK(CheckFourgram("saw", "a", "girl", "with"));
  BOOST_CHECK(CheckFourgram("a", "girl", "with", "a"));
  BOOST_CHECK(CheckFourgram("girl", "with", "a", "telescope"));
  BOOST_CHECK(CheckFourgram("with", "a", "telescope", "."));
}
