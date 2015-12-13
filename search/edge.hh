#ifndef SEARCH_EDGE__
#define SEARCH_EDGE__

#include "lm/state.hh"
#include "search/header.hh"
#include "search/types.hh"
#include "search/vertex.hh"
#include "util/pool.hh"

#include <functional>

#include <stdint.h>

namespace search {

// Copyable, but the copy will be shallow.
class PartialEdge : public Header {
  public:
    // Allow default construction for STL.
    PartialEdge() {}

    PartialEdge(util::Pool &pool, Arity arity)
      : Header(pool.Allocate(Size(arity, arity + 1)), arity) {}

    PartialEdge(util::Pool &pool, Arity arity, Arity chart_states)
      : Header(pool.Allocate(Size(arity, chart_states)), arity) {}

    // Non-terminals
    const PartialVertex *NT() const {
      return reinterpret_cast<const PartialVertex*>(After());
    }
    PartialVertex *NT() {
      return reinterpret_cast<PartialVertex*>(After());
    }

    const lm::ngram::ChartState &CompletedState() const {
      return *Between();
    }
    const lm::ngram::ChartState *Between() const {
      return reinterpret_cast<const lm::ngram::ChartState*>(After() + GetArity() * sizeof(PartialVertex));
    }
    lm::ngram::ChartState *Between() {
      return reinterpret_cast<lm::ngram::ChartState*>(After() + GetArity() * sizeof(PartialVertex));
    }

  private:
    static std::size_t Size(Arity arity, Arity chart_states) {
      return kHeaderSize + arity * sizeof(PartialVertex) + chart_states * sizeof(lm::ngram::ChartState);
    }
};


} // namespace search
#endif // SEARCH_EDGE__
