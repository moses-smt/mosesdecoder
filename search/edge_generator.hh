#ifndef SEARCH_EDGE_GENERATOR__
#define SEARCH_EDGE_GENERATOR__

#include "search/edge.hh"
#include "search/note.hh"

#include <boost/pool/pool.hpp>
#include <boost/unordered_map.hpp>

#include <functional>
#include <queue>

namespace lm {
namespace ngram {
class ChartState;
} // namespace ngram
} // namespace lm

namespace search {

template <class Model> class Context;

class VertexGenerator;

struct PartialEdgePointerLess : std::binary_function<const PartialEdge *, const PartialEdge *, bool> {
  bool operator()(const PartialEdge *first, const PartialEdge *second) const {
    return *first < *second;
  }
};

class EdgeGenerator {
  public:
    EdgeGenerator(PartialEdge &root, unsigned char arity, Note note);

    Score TopScore() const {
      return top_score_;
    }

    Note GetNote() const {
      return note_;
    }

    // Pop.  If there's a complete hypothesis, return it.  Otherwise return NULL.  
    template <class Model> PartialEdge *Pop(Context<Model> &context, boost::pool<> &partial_edge_pool);

  private:
    Score top_score_;

    unsigned char arity_;

    typedef std::priority_queue<PartialEdge*, std::vector<PartialEdge*>, PartialEdgePointerLess> Generate;
    Generate generate_;

    Note note_;
};

} // namespace search
#endif // SEARCH_EDGE_GENERATOR__
