#ifndef SEARCH_VERTEX_GENERATOR__
#define SEARCH_VERTEX_GENERATOR__

#include "search/note.hh"
#include "search/vertex.hh"

#include <boost/unordered_map.hpp>

#include <queue>

namespace lm {
namespace ngram {
class ChartState;
} // namespace ngram
} // namespace lm

namespace search {

class ContextBase;
class Final;
struct PartialEdge;

class VertexGenerator {
  public:
    VertexGenerator(ContextBase &context, Vertex &gen);

    void NewHypothesis(const PartialEdge &partial, Note note);

    void FinishedSearch() {
      root_.under->SortAndSet(context_, NULL);
    }

    const Vertex &Generating() const { return gen_; }

  private:
    // Parallel structure to VertexNode.  
    struct Trie {
      Trie() : under(NULL) {}

      VertexNode *under;
      boost::unordered_map<uint64_t, Trie> extend;
    };

    Trie &FindOrInsert(Trie &node, uint64_t added, const lm::ngram::ChartState &state, unsigned char left, bool left_full, unsigned char right, bool right_full);

    Final *CompleteTransition(Trie &node, const lm::ngram::ChartState &state, Note note, const PartialEdge &partial);

    ContextBase &context_;

    Vertex &gen_;

    Trie root_;

    typedef boost::unordered_map<uint64_t, Final*> Existing;
    Existing existing_;
};

} // namespace search
#endif // SEARCH_VERTEX_GENERATOR__
