#include "BleuScorer.h"

#define BOOST_TEST_MODULE MertBleuScorer
#include <boost/test/unit_test.hpp>

#include <cmath>
#include "Ngram.h"
#include "Vocabulary.h"
#include "Util.h"

using namespace MosesTuning;

namespace
{

NgramCounts* g_counts = NULL;

NgramCounts* GetNgramCounts()
{
  assert(g_counts);
  return g_counts;
}

void SetNgramCounts(NgramCounts* counts)
{
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

bool CheckUnigram(const std::string& str)
{
  Unigram unigram(str);
  NgramCounts::Value v;
  return GetNgramCounts()->Lookup(unigram.instance, &v);
}

bool CheckBigram(const std::string& a, const std::string& b)
{
  Bigram bigram(a, b);
  NgramCounts::Value v;
  return GetNgramCounts()->Lookup(bigram.instance, &v);
}

bool CheckTrigram(const std::string& a, const std::string& b,
                  const std::string& c)
{
  Trigram trigram(a, b, c);
  NgramCounts::Value v;
  return GetNgramCounts()->Lookup(trigram.instance, &v);
}

bool CheckFourgram(const std::string& a, const std::string& b,
                   const std::string& c, const std::string& d)
{
  Fourgram fourgram(a, b, c, d);
  NgramCounts::Value v;
  return GetNgramCounts()->Lookup(fourgram.instance, &v);
}

void SetUpReferences(BleuScorer& scorer)
{
  // The following examples are taken from Koehn, "Statistical Machine Translation",
  // Cambridge University Press, 2010.
  {
    std::stringstream ref1;
    ref1 << "israeli officials are responsible for airport security" << std::endl;
    BOOST_CHECK(scorer.OpenReferenceStream(&ref1, 0));
  }

  {
    std::stringstream ref2;
    ref2 << "israel is in charge of the security at this airport" << std::endl;
    BOOST_CHECK(scorer.OpenReferenceStream(&ref2, 1));
  }

  {
    std::stringstream ref3;
    ref3 << "the security work for this airport is the responsibility of the israel government"
         << std::endl;
    BOOST_CHECK(scorer.OpenReferenceStream(&ref3, 2));
  }

  {
    std::stringstream ref4;
    ref4 << "israli side was in charge of the security of this airport" << std::endl;
    BOOST_CHECK(scorer.OpenReferenceStream(&ref4, 3));
  }
}

} // namespace

BOOST_AUTO_TEST_CASE(bleu_reference_type)
{
  BleuScorer scorer;
  // BleuScorer will use "closest" by default.
  BOOST_CHECK_EQUAL(scorer.GetReferenceLengthType(), BleuScorer::CLOSEST);

  scorer.SetReferenceLengthType(BleuScorer::AVERAGE);
  BOOST_CHECK_EQUAL(scorer.GetReferenceLengthType(), BleuScorer::AVERAGE);

  scorer.SetReferenceLengthType(BleuScorer::SHORTEST);
  BOOST_CHECK_EQUAL(scorer.GetReferenceLengthType(), BleuScorer::SHORTEST);
}

BOOST_AUTO_TEST_CASE(bleu_reference_type_with_config)
{
  {
    BleuScorer scorer("reflen:average");
    BOOST_CHECK_EQUAL(scorer.GetReferenceLengthType(), BleuScorer::AVERAGE);
  }

  {
    BleuScorer scorer("reflen:shortest");
    BOOST_CHECK_EQUAL(scorer.GetReferenceLengthType(), BleuScorer::SHORTEST);
  }
}

BOOST_AUTO_TEST_CASE(bleu_count_ngrams)
{
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
  BOOST_CHECK_EQUAL((std::size_t)25, counts.size());

  mert::Vocabulary* vocab = scorer.GetVocab();
  BOOST_CHECK_EQUAL((std::size_t)7, vocab->size());

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

BOOST_AUTO_TEST_CASE(bleu_clipped_counts)
{
  BleuScorer scorer;
  SetUpReferences(scorer);
  std::string line("israeli officials responsibility of airport safety");
  ScoreStats entry;
  scorer.prepareStats(0, line, entry);

  BOOST_CHECK_EQUAL(entry.size(), (std::size_t)(2 * kBleuNgramOrder + 1));

  // Test hypothesis ngram counts
  BOOST_CHECK_EQUAL(entry.get(0), 5);  // unigram
  BOOST_CHECK_EQUAL(entry.get(2), 2);  // bigram
  BOOST_CHECK_EQUAL(entry.get(4), 0);  // trigram
  BOOST_CHECK_EQUAL(entry.get(6), 0);  // fourgram

  // Test reference ngram counts.
  BOOST_CHECK_EQUAL(entry.get(1), 6);  // unigram
  BOOST_CHECK_EQUAL(entry.get(3), 5);  // bigram
  BOOST_CHECK_EQUAL(entry.get(5), 4);  // trigram
  BOOST_CHECK_EQUAL(entry.get(7), 3);  // fourgram
}

BOOST_AUTO_TEST_CASE(calculate_actual_score)
{
  BOOST_REQUIRE(4 == kBleuNgramOrder);
  std::vector<ScoreStatsType> stats(2 * kBleuNgramOrder + 1);
  BleuScorer scorer;

  // unigram
  stats[0] = 6;
  stats[1] = 6;

  // bigram
  stats[2] = 4;
  stats[3] = 5;

  // trigram
  stats[4] = 2;
  stats[5] = 4;

  // fourgram
  stats[6] = 1;
  stats[7] = 3;

  // reference-length
  stats[8] = 7;

  BOOST_CHECK_CLOSE(0.5115f, scorer.calculateScore(stats), 0.01);
}

BOOST_AUTO_TEST_CASE(sentence_level_bleu)
{
  BOOST_REQUIRE(4 == kBleuNgramOrder);
  std::vector<float> stats(2 * kBleuNgramOrder + 1);

  // unigram
  stats[0] = 6.0;
  stats[1] = 6.0;

  // bigram
  stats[2] = 4.0;
  stats[3] = 5.0;

  // trigram
  stats[4] = 2.0;
  stats[5] = 4.0;

  // fourgram
  stats[6] = 1.0;
  stats[7] = 3.0;

  // reference-length
  stats[8] = 7.0;

  BOOST_CHECK_CLOSE(0.5985f, smoothedSentenceBleu(stats), 0.01);
  BOOST_CHECK_CLOSE(0.5624f, smoothedSentenceBleu(stats, 0.5), 0.01 );
  BOOST_CHECK_CLOSE(0.5067f, smoothedSentenceBleu(stats, 1.0, true), 0.01);
}
