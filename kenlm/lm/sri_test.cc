#include "lm/sri.hh"

#include <stdlib.h>

#define BOOST_TEST_MODULE SRITest
#include <boost/test/unit_test.hpp>

namespace lm {
namespace sri {
namespace {

#define StartTest(word, ngram, score) \
  ret = model.FullScore( \
      state, \
      model.GetVocabulary().Index(word), \
      out);\
  BOOST_CHECK_CLOSE(score, ret.prob, 0.001); \
  BOOST_CHECK_EQUAL(static_cast<unsigned int>(ngram), ret.ngram_length); \
  BOOST_CHECK_EQUAL(std::min<unsigned char>(ngram, 5 - 1), out.valid_length_);

#define AppendTest(word, ngram, score) \
  StartTest(word, ngram, score) \
  state = out;

template <class M> void Starters(M &model) {
  FullScoreReturn ret;
  Model::State state(model.BeginSentenceState());
  Model::State out;

  StartTest("looking", 2, -0.4846522);

  // , probability plus <s> backoff
  StartTest(",", 1, -1.383514 + -0.4149733);
  // <unk> probability plus <s> backoff
  StartTest("this_is_not_found", 0, -1.995635 + -0.4149733);
}

template <class M> void Continuation(M &model) {
  FullScoreReturn ret;
  Model::State state(model.BeginSentenceState());
  Model::State out;

  AppendTest("looking", 2, -0.484652);
  AppendTest("on", 3, -0.348837);
  AppendTest("a", 4, -0.0155266);
  AppendTest("little", 5, -0.00306122);
  State preserve = state;
  AppendTest("the", 1, -4.04005);
  AppendTest("biarritz", 1, -1.9889);
  AppendTest("not_found", 0, -2.29666);
  AppendTest("more", 1, -1.20632);
  AppendTest(".", 2, -0.51363);
  AppendTest("</s>", 3, -0.0191651);

  state = preserve;
  AppendTest("more", 5, -0.00181395);
  AppendTest("loin", 5, -0.0432557);
}

BOOST_AUTO_TEST_CASE(starters) { Model m("test.arpa", 5); Starters(m); }
BOOST_AUTO_TEST_CASE(continuation) { Model m("test.arpa", 5); Continuation(m); }

} // namespace
} // namespace sri
} // namespace lm
