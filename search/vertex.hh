#ifndef SEARCH_VERTEX__
#define SEARCH_VERTEX__

#include "lm/left.hh"
#include "search/types.hh"

#include <boost/unordered_set.hpp>

#include <queue>
#include <vector>

#include <math.h>
#include <stdint.h>

namespace search {

class ContextBase;

struct HypoState {
  History history;
  lm::ngram::ChartState state;
  Score score;
};

class VertexNode {
  public:
    VertexNode() {}

    void InitRoot() { hypos_.clear(); }

    /* The steps of building a VertexNode:
     * 1. Default construct.
     * 2. AppendHypothesis at least once, possibly multiple times.
     * 3. FinishAppending with the number of words on left and right guaranteed
     * to be common.
     * 4. If !Complete(), call BuildExtend to construct the extensions
     */
    // Must default construct, call AppendHypothesis 1 or more times then do FinishedAppending.
    void AppendHypothesis(const NBestComplete &best) {
      assert(hypos_.empty() || !(hypos_.front().state == *best.state));
      HypoState hypo;
      hypo.history = best.history;
      hypo.state = *best.state;
      hypo.score = best.score;
      hypos_.push_back(hypo);
    }
    void AppendHypothesis(const HypoState &hypo) {
      hypos_.push_back(hypo);
    }

    // Sort hypotheses for the root.
    void FinishRoot();

    void FinishedAppending(const unsigned char common_left, const unsigned char common_right);

    void BuildExtend();

    // Should only happen to a root node when the entire vertex is empty.   
    bool Empty() const {
      return hypos_.empty() && extend_.empty();
    }

    bool Complete() const {
      // HACK: prevent root from being complete.  TODO: allow root to be complete.
      return hypos_.size() == 1 && extend_.empty();
    }

    const lm::ngram::ChartState &State() const { return state_; }
    bool RightFull() const { return right_full_; }

    // Priority relative to other non-terminals.  0 is highest.
    unsigned char Niceness() const { return niceness_; }

    Score Bound() const {
      return bound_;
    }

    // Will be invalid unless this is a leaf.   
    const History End() const {
      assert(hypos_.size() == 1);
      return hypos_.front().history;
    }

    VertexNode &operator[](size_t index) {
      assert(!extend_.empty());
      return extend_[index];
    }

    size_t Size() const {
      return extend_.size();
    }

  private:
    // Hypotheses to be split.
    std::vector<HypoState> hypos_;

    std::vector<VertexNode> extend_;

    lm::ngram::ChartState state_;
    bool right_full_;

    unsigned char niceness_;

    unsigned char policy_;

    Score bound_;
};

class PartialVertex {
  public:
    PartialVertex() {}

    explicit PartialVertex(VertexNode &back) : back_(&back), index_(0) {}

    bool Empty() const { return back_->Empty(); }

    bool Complete() const { return back_->Complete(); }

    const lm::ngram::ChartState &State() const { return back_->State(); }
    bool RightFull() const { return back_->RightFull(); }

    Score Bound() const { return index_ ? (*back_)[index_].Bound() : back_->Bound(); }

    unsigned char Niceness() const { return back_->Niceness(); }

    // Split into continuation and alternative, rendering this the continuation.
    bool Split(PartialVertex &alternative) {
      assert(!Complete());
      back_->BuildExtend();
      bool ret;
      if (index_ + 1 < back_->Size()) {
        alternative.index_ = index_ + 1;
        alternative.back_ = back_;
        ret = true;
      } else {
        ret = false;
      }
      back_ = &((*back_)[index_]);
      index_ = 0;
      return ret;
    }

    const History End() const {
      return back_->End();
    }

  private:
    VertexNode *back_;
    unsigned int index_;
};

template <class Output> class VertexGenerator;

class Vertex {
  public:
    Vertex() {}

    //PartialVertex RootFirst() const { return PartialVertex(right_); }
    PartialVertex RootAlternate() { return PartialVertex(root_); }
    //PartialVertex RootLast() const { return PartialVertex(left_); }

    bool Empty() const {
      return root_.Empty();
    }

    Score Bound() const {
      return root_.Bound();
    }

    const History BestChild() {
      // left_ and right_ are not set at the root.
      PartialVertex top(RootAlternate());
      if (top.Empty()) {
        return History();
      } else {
        PartialVertex continuation;
        while (!top.Complete()) {
          top.Split(continuation);
        }
        return top.End();
      }
    }

  private:
    template <class Output> friend class VertexGenerator;
    template <class Output> friend class RootVertexGenerator;
    VertexNode root_;

    // These will not be set for the root vertex.
    // Branches only on left state.
    //VertexNode left_;
    // Branches only on right state.
    //VertexNode right_;
};

} // namespace search
#endif // SEARCH_VERTEX__
