#ifndef SEARCH_EDGE__
#define SEARCH_EDGE__

#include "lm/state.hh"
#include "search/types.hh"
#include "search/vertex.hh"
#include "util/pool.hh"

#include <functional>

#include <stdint.h>

namespace search {

// Copyable, but the copy will be shallow.
class PartialEdge {
  public:
    // Allow default construction for STL.  
    PartialEdge() : base_(NULL) {}
    bool Valid() const { return base_; }

    Score GetScore() const {
      return *reinterpret_cast<const float*>(base_);
    }
    void SetScore(Score to) {
      *reinterpret_cast<float*>(base_) = to;
    }
    bool operator<(const PartialEdge &other) const {
      return GetScore() < other.GetScore();
    }

    Arity GetArity() const {
      return *reinterpret_cast<const Arity*>(base_ + sizeof(Score));
    }

    // Non-terminals
    const PartialVertex *NT() const {
      return reinterpret_cast<const PartialVertex*>(base_ + sizeof(Score) + sizeof(Arity));
    }
    PartialVertex *NT() {
      return reinterpret_cast<PartialVertex*>(base_ + sizeof(Score) + sizeof(Arity));
    }

    const lm::ngram::ChartState &CompletedState() const {
      return *Between();
    }
    const lm::ngram::ChartState *Between() const {
      return reinterpret_cast<const lm::ngram::ChartState*>(base_ + sizeof(Score) + sizeof(Arity) + GetArity() * sizeof(PartialVertex));
    }
    lm::ngram::ChartState *Between() {
      return reinterpret_cast<lm::ngram::ChartState*>(base_ + sizeof(Score) + sizeof(Arity) + GetArity() * sizeof(PartialVertex));
    }

  private:
    friend class PartialEdgePool;
    PartialEdge(void *base, Arity arity) : base_(static_cast<uint8_t*>(base)) {
      *reinterpret_cast<Arity*>(base_ + sizeof(Score)) = arity;
    }

    uint8_t *base_;
};

class PartialEdgePool {
  public:
    PartialEdge Allocate(Arity arity) {
      return PartialEdge(
          pool_.Allocate(sizeof(Score) + sizeof(Arity) + arity * sizeof(PartialVertex) + (arity + 1) * sizeof(lm::ngram::ChartState)),
          arity);
    }

  private:
    util::Pool pool_;
};


} // namespace search
#endif // SEARCH_EDGE__
