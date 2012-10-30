#ifndef SEARCH_VERTEX_GENERATOR__
#define SEARCH_VERTEX_GENERATOR__

#include "search/edge.hh"
#include "search/vertex.hh"

#include <boost/unordered_map.hpp>

namespace lm {
namespace ngram {
class ChartState;
} // namespace ngram
} // namespace lm

namespace search {

class ContextBase;
class Final;

class VertexGenerator {
  public:
    VertexGenerator(ContextBase &context, Vertex &gen);

    void NewHypothesis(PartialEdge partial) {
      const lm::ngram::ChartState &state = partial.CompletedState();
      std::pair<Existing::iterator, bool> ret(existing_.insert(std::make_pair(hash_value(state), partial)));
      if (!ret.second && ret.first->second < partial) {
        ret.first->second = partial;
      }
    }

    void FinishedSearch();

    const Vertex &Generating() const { return gen_; }

  private:
    ContextBase &context_;

    Vertex &gen_;

    typedef boost::unordered_map<uint64_t, PartialEdge> Existing;
    Existing existing_;
};

} // namespace search
#endif // SEARCH_VERTEX_GENERATOR__
