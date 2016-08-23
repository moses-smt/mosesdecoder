#ifndef SEARCH_TYPES__
#define SEARCH_TYPES__

#include <stdint.h>

namespace lm { namespace ngram { struct ChartState; } }

namespace search {

typedef float Score;

typedef uint32_t Arity;

union Note {
  const void *vp;
};

typedef void *History;

struct NBestComplete {
  NBestComplete(History in_history, const lm::ngram::ChartState &in_state, Score in_score)
    : history(in_history), state(&in_state), score(in_score) {}

  History history;
  const lm::ngram::ChartState *state;
  Score score;
};

} // namespace search

#endif // SEARCH_TYPES__
