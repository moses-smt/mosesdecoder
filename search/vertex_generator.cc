#include "search/vertex_generator.hh"

#include "lm/left.hh"
#include "search/context.hh"
#include "search/edge.hh"

#include <boost/unordered_map.hpp>
#include <boost/version.hpp>

#include <stdint.h>

namespace search {

VertexGenerator::VertexGenerator(ContextBase &context, Vertex &gen) : context_(context), gen_(gen) {
  gen.root_.InitRoot();
}

#if BOOST_VERSION > 104200
namespace {

const uint64_t kCompleteAdd = static_cast<uint64_t>(-1);

// Parallel structure to VertexNode.  
struct Trie {
  Trie() : under(NULL) {}

  VertexNode *under;
  boost::unordered_map<uint64_t, Trie> extend;
};

Trie &FindOrInsert(ContextBase &context, Trie &node, uint64_t added, const lm::ngram::ChartState &state, unsigned char left, bool left_full, unsigned char right, bool right_full) {
  Trie &next = node.extend[added];
  if (!next.under) {
    next.under = context.NewVertexNode();
    lm::ngram::ChartState &writing = next.under->MutableState();
    writing = state;
    writing.left.full &= left_full && state.left.full;
    next.under->MutableRightFull() = right_full && state.left.full;
    writing.left.length = left;
    writing.right.length = right;
    node.under->AddExtend(next.under);
  }
  return next;
}

void CompleteTransition(ContextBase &context, Trie &starter, PartialEdge partial) {
  Final final(context.FinalPool(), partial.GetScore(), partial.GetArity(), partial.GetNote());
  Final *child_out = final.Children();
  const PartialVertex *part = partial.NT();
  const PartialVertex *const part_end_loop = part + partial.GetArity();
  for (; part != part_end_loop; ++part, ++child_out)
    *child_out = part->End();

  starter.under->SetEnd(final);
}

void AddHypothesis(ContextBase &context, Trie &root, PartialEdge partial) {
  const lm::ngram::ChartState &state = partial.CompletedState();
  
  unsigned char left = 0, right = 0;
  Trie *node = &root;
  while (true) {
    if (left == state.left.length) {
      node = &FindOrInsert(context, *node, kCompleteAdd - state.left.full, state, left, true, right, false);
      for (; right < state.right.length; ++right) {
        node = &FindOrInsert(context, *node, state.right.words[right], state, left, true, right + 1, false);
      }
      break;
    }
    node = &FindOrInsert(context, *node, state.left.pointers[left], state, left + 1, false, right, false);
    left++;
    if (right == state.right.length) {
      node = &FindOrInsert(context, *node, kCompleteAdd - state.left.full, state, left, false, right, true);
      for (; left < state.left.length; ++left) {
        node = &FindOrInsert(context, *node, state.left.pointers[left], state, left + 1, false, right, true);
      }
      break;
    }
    node = &FindOrInsert(context, *node, state.right.words[right], state, left, false, right + 1, false);
    right++;
  }

  node = &FindOrInsert(context, *node, kCompleteAdd - state.left.full, state, state.left.length, true, state.right.length, true);
  CompleteTransition(context, *node, partial);
}

} // namespace

#else // BOOST_VERSION

struct Trie {
  VertexNode *under;
};

void AddHypothesis(ContextBase &context, Trie &root, PartialEdge partial) {
  UTIL_THROW(util::Exception, "Upgrade Boost to >= 1.42.0 to use incremental search.");
}

#endif // BOOST_VERSION

void VertexGenerator::FinishedSearch() {
  Trie root;
  root.under = &gen_.root_;
  for (Existing::const_iterator i(existing_.begin()); i != existing_.end(); ++i) {
    AddHypothesis(context_, root, i->second);
  }
  root.under->SortAndSet(context_, NULL);
}

} // namespace search
