#ifndef SEARCH_VERTEX_GENERATOR__
#define SEARCH_VERTEX_GENERATOR__

#include "search/edge.hh"
#include "search/types.hh"
#include "search/vertex.hh"

namespace lm {
namespace ngram {
struct ChartState;
} // namespace ngram
} // namespace lm

namespace search {

class ContextBase;

// Output makes the single-best or n-best list.
template <class Output> class VertexGenerator {
  public:
    VertexGenerator(ContextBase &context, Vertex &gen, Output &nbest) : context_(context), gen_(gen), nbest_(nbest) {}

    void NewHypothesis(PartialEdge partial) {
      nbest_.Add(existing_[hash_value(partial.CompletedState())], partial);
    }

    void FinishedSearch() {
      gen_.root_.InitRoot();
      for (typename Existing::iterator i(existing_.begin()); i != existing_.end(); ++i) {
        gen_.root_.AppendHypothesis(nbest_.Complete(i->second));
      }
      existing_.clear();
      gen_.root_.FinishRoot();
    }

    Vertex &Generating() { return gen_; }

  private:
    ContextBase &context_;

    Vertex &gen_;

    typedef boost::unordered_map<uint64_t, typename Output::Combine> Existing;
    Existing existing_;

    Output &nbest_;
};

// Special case for root vertex: everything should come together into the root
// node.  In theory, this should happen naturally due to state collapsing with
// <s> and </s>.  If that's the case, VertexGenerator is fine, though it will
// make one connection.
template <class Output> class RootVertexGenerator {
  public:
    RootVertexGenerator(Vertex &gen, Output &out) : gen_(gen), out_(out) {}

    void NewHypothesis(PartialEdge partial) {
      out_.Add(combine_, partial);
    }

    void FinishedSearch() {
      gen_.root_.InitRoot();
      gen_.root_.AppendHypothesis(out_.Complete(combine_));
      gen_.root_.FinishRoot();
    }

  private:
    Vertex &gen_;

    typename Output::Combine combine_;
    Output &out_;
};

} // namespace search
#endif // SEARCH_VERTEX_GENERATOR__
