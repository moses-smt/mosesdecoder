#ifndef SEARCH_EDGE_GENERATOR__
#define SEARCH_EDGE_GENERATOR__

#include "search/edge.hh"
#include "search/types.hh"

#include <queue>

namespace lm {
namespace ngram {
struct ChartState;
} // namespace ngram
} // namespace lm

namespace search {

template <class Model> class Context;

class EdgeGenerator {
  public:
    EdgeGenerator() {}

    PartialEdge AllocateEdge(Arity arity) {
      return PartialEdge(partial_edge_pool_, arity);
    }

    void AddEdge(PartialEdge edge) {
      generate_.push(edge);
    }

    bool Empty() const { return generate_.empty(); }

    // Pop.  If there's a complete hypothesis, return it.  Otherwise return an invalid PartialEdge.
    template <class Model> PartialEdge Pop(Context<Model> &context);

    template <class Model, class Output> void Search(Context<Model> &context, Output &output) {
      unsigned to_pop = context.PopLimit();
      while (to_pop > 0 && !generate_.empty()) {
        PartialEdge got(Pop(context));
        if (got.Valid()) {
          output.NewHypothesis(got);
          --to_pop;
        }
      }
      output.FinishedSearch();
    }

  private:
    util::Pool partial_edge_pool_;

    typedef std::priority_queue<PartialEdge> Generate;
    Generate generate_;
};

} // namespace search
#endif // SEARCH_EDGE_GENERATOR__
