#include "search/vertex_generator.hh"

#include "lm/left.hh"
#include "search/context.hh"
#include "search/edge.hh"

#include <boost/unordered_map.hpp>
#include <boost/version.hpp>

#include <stdint.h>

namespace search {

#if BOOST_VERSION > 104200
namespace {

const uint64_t kCompleteAdd = static_cast<uint64_t>(-1);

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

} // namespace

void AddHypothesis(ContextBase &context, Trie &root, const NBestComplete &end) {
  const lm::ngram::ChartState &state = *end.state;
  
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
  node->under->SetEnd(end.history, end.score);
}

#endif // BOOST_VERSION

} // namespace search
