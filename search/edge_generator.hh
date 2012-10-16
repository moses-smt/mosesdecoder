#ifndef SEARCH_EDGE_GENERATOR__
#define SEARCH_EDGE_GENERATOR__

#include "search/edge.hh"
#include "search/note.hh"
#include "search/types.hh"

#include <queue>

namespace lm {
namespace ngram {
class ChartState;
} // namespace ngram
} // namespace lm

namespace search {

template <class Model> class Context;
class PartialEdgePool;

class EdgeGenerator {
  public:
    EdgeGenerator(PartialEdge root, Note note);

    Score TopScore() const {
      return top_score_;
    }

    Note GetNote() const {
      return note_;
    }

    // Pop.  If there's a complete hypothesis, return it.  Otherwise return NULL.  
    template <class Model> PartialEdge Pop(Context<Model> &context, PartialEdgePool &partial_edge_pool);

  private:
    Score top_score_;

    typedef std::priority_queue<PartialEdge> Generate;
    Generate generate_;

    Arity arity_;

    Note note_;
};

} // namespace search
#endif // SEARCH_EDGE_GENERATOR__
