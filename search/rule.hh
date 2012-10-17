#ifndef SEARCH_RULE__
#define SEARCH_RULE__

#include "lm/left.hh"
#include "lm/word_index.hh"
#include "search/types.hh"

#include <vector>

namespace search {

template <class Model> class Context;

const lm::WordIndex kNonTerminal = lm::kMaxWordIndex;

template <class Model> float ScoreRule(const Context<Model> &context, const std::vector<lm::WordIndex> &words, bool prepend_bos, lm::ngram::ChartState *state_out);

} // namespace search

#endif // SEARCH_RULE__
