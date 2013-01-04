#ifndef SEARCH_VERTEX_GENERATOR__
#define SEARCH_VERTEX_GENERATOR__

#include "search/edge.hh"
#include "search/types.hh"
#include "search/vertex.hh"

#include <boost/unordered_map.hpp>
#include <boost/version.hpp>

#if BOOST_VERSION <= 104200
#include "util/exception.hh"
#endif

namespace lm {
namespace ngram {
class ChartState;
} // namespace ngram
} // namespace lm

namespace search {

class ContextBase;

#if BOOST_VERSION > 104200
// Parallel structure to VertexNode.  
struct Trie {
  Trie() : under(NULL) {}

  VertexNode *under;
  boost::unordered_map<uint64_t, Trie> extend;
};

void AddHypothesis(ContextBase &context, Trie &root, const NBestComplete &end);

#endif // BOOST_VERSION

// Output makes the single-best or n-best list.   
template <class Output> class VertexGenerator {
  public:
    VertexGenerator(ContextBase &context, Vertex &gen, Output &nbest) : context_(context), gen_(gen), nbest_(nbest) {
      gen.root_.InitRoot();
    }

    void NewHypothesis(PartialEdge partial) {
      nbest_.Add(existing_[hash_value(partial.CompletedState())], partial);
    }

    void FinishedSearch() {
#if BOOST_VERSION > 104200
      Trie root;
      root.under = &gen_.root_;
      for (typename Existing::iterator i(existing_.begin()); i != existing_.end(); ++i) {
        AddHypothesis(context_, root, nbest_.Complete(i->second));
      }
      existing_.clear();
      root.under->SortAndSet(context_);
#else
      UTIL_THROW(util::Exception, "Upgrade Boost to >= 1.42.0 to use incremental search.");
#endif
    }

    const Vertex &Generating() const { return gen_; }

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
      NBestComplete completed(out_.Complete(combine_));
      gen_.root_.SetEnd(completed.history, completed.score);
    }

  private:
    Vertex &gen_;
    
    typename Output::Combine combine_;
    Output &out_;
};

} // namespace search
#endif // SEARCH_VERTEX_GENERATOR__
