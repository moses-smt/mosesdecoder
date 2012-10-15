#ifndef SEARCH_EDGE__
#define SEARCH_EDGE__

#include "lm/state.hh"
#include "search/arity.hh"
#include "search/rule.hh"
#include "search/types.hh"
#include "search/vertex.hh"

#include <queue>

namespace search {

struct PartialEdge {
  Score score;
  // Terminals
  lm::ngram::ChartState between[kMaxArity + 1];
  // Non-terminals
  PartialVertex nt[kMaxArity];

  const lm::ngram::ChartState &CompletedState() const {
    return between[0];
  }

  bool operator<(const PartialEdge &other) const {
    return score < other.score;
  }
};

} // namespace search
#endif // SEARCH_EDGE__
