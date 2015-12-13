#ifndef SEARCH_RULE__
#define SEARCH_RULE__

#include "lm/left.hh"
#include "lm/word_index.hh"
#include "search/types.hh"

#include <vector>

namespace search {

const lm::WordIndex kNonTerminal = lm::kMaxWordIndex;

struct ScoreRuleRet {
  Score prob;
  unsigned int oov;
};

// Pass <s> and </s> normally.
// Indicate non-terminals with kNonTerminal.
template <class Model> ScoreRuleRet ScoreRule(const Model &model, const std::vector<lm::WordIndex> &words, lm::ngram::ChartState *state_out);

} // namespace search

#endif // SEARCH_RULE__
