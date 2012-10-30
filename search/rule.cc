#include "search/rule.hh"

#include "search/context.hh"
#include "search/final.hh"

#include <ostream>

#include <math.h>

namespace search {

template <class Model> float ScoreRule(const Context<Model> &context, const std::vector<lm::WordIndex> &words, bool prepend_bos, lm::ngram::ChartState *writing) {
  unsigned int oov_count = 0;
  float prob = 0.0;
  const Model &model = context.LanguageModel();
  const lm::WordIndex oov = model.GetVocabulary().NotFound();
  for (std::vector<lm::WordIndex>::const_iterator word = words.begin(); ; ++word) {
    lm::ngram::RuleScore<Model> scorer(model, *(writing++));
    // TODO: optimize
    if (prepend_bos && (word == words.begin())) {
      scorer.BeginSentence();
    }
    for (; ; ++word) {
      if (word == words.end()) {
        prob += scorer.Finish();
        return static_cast<float>(oov_count) * context.GetWeights().OOV() + prob * context.GetWeights().LM();
      }
      if (*word == kNonTerminal) break;
      if (*word == oov) ++oov_count;
      scorer.Terminal(*word);
    }
    prob += scorer.Finish();
  }
}

template float ScoreRule(const Context<lm::ngram::RestProbingModel> &model, const std::vector<lm::WordIndex> &words, bool prepend_bos, lm::ngram::ChartState *writing);
template float ScoreRule(const Context<lm::ngram::ProbingModel> &model, const std::vector<lm::WordIndex> &words, bool prepend_bos, lm::ngram::ChartState *writing);
template float ScoreRule(const Context<lm::ngram::TrieModel> &model, const std::vector<lm::WordIndex> &words, bool prepend_bos, lm::ngram::ChartState *writing);
template float ScoreRule(const Context<lm::ngram::QuantTrieModel> &model, const std::vector<lm::WordIndex> &words, bool prepend_bos, lm::ngram::ChartState *writing);
template float ScoreRule(const Context<lm::ngram::ArrayTrieModel> &model, const std::vector<lm::WordIndex> &words, bool prepend_bos, lm::ngram::ChartState *writing);
template float ScoreRule(const Context<lm::ngram::QuantArrayTrieModel> &model, const std::vector<lm::WordIndex> &words, bool prepend_bos, lm::ngram::ChartState *writing);

} // namespace search
