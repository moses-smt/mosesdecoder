#include "search/vertex.hh"

#include "search/context.hh"

#include <boost/unordered_map.hpp>

#include <algorithm>
#include <functional>
#include <cassert>

namespace search {

namespace {

const uint64_t kCompleteAdd = static_cast<uint64_t>(-1);

class DivideLeft {
  public:
    explicit DivideLeft(unsigned char index)
      : index_(index) {}

    uint64_t operator()(const lm::ngram::ChartState &state) const {
      return (index_ < state.left.length) ?
        state.left.pointers[index_] :
        (kCompleteAdd - state.left.full);
    }

  private:
    unsigned char index_;
};

class DivideRight {
  public:
    explicit DivideRight(unsigned char index)
      : index_(index) {}

    uint64_t operator()(const lm::ngram::ChartState &state) const {
      return (index_ < state.right.length) ?
        static_cast<uint64_t>(state.right.words[index_]) :
        (kCompleteAdd - state.left.full);
    }

  private:
    unsigned char index_;
};

template <class Divider> void Split(const Divider &divider, const std::vector<HypoState> &hypos, std::vector<VertexNode> &extend) {
  // Map from divider to index in extend.
  typedef boost::unordered_map<uint64_t, std::size_t> Lookup;
  Lookup lookup;
  for (std::vector<HypoState>::const_iterator i = hypos.begin(); i != hypos.end(); ++i) {
    uint64_t key = divider(i->state);
    std::pair<Lookup::iterator, bool> res(lookup.insert(std::make_pair(key, extend.size())));
    if (res.second) {
      extend.resize(extend.size() + 1);
      extend.back().AppendHypothesis(*i);
    } else {
      extend[res.first->second].AppendHypothesis(*i);
    }
  }
  //assert((extend.size() != 1) || (hypos.size() == 1));
}

lm::WordIndex Identify(const lm::ngram::Right &right, unsigned char index) {
  return right.words[index];
}

uint64_t Identify(const lm::ngram::Left &left, unsigned char index) {
  return left.pointers[index];
}

template <class Side> class DetermineSame {
  public:
    DetermineSame(const Side &side, unsigned char guaranteed)
      : side_(side), guaranteed_(guaranteed), shared_(side.length), complete_(true) {}

    void Consider(const Side &other) {
      if (shared_ != other.length) {
        complete_ = false;
        if (shared_ > other.length)
          shared_ = other.length;
      }
      for (unsigned char i = guaranteed_; i < shared_; ++i) {
        if (Identify(side_, i) != Identify(other, i)) {
          shared_ = i;
          complete_ = false;
          return;
        }
      }
    }

    unsigned char Shared() const { return shared_; }

    bool Complete() const { return complete_; }

  private:
    const Side &side_;
    unsigned char guaranteed_, shared_;
    bool complete_;
};

// Custom enum to save memory: valid values of policy_.
// Alternate and there is still alternation to do.
const unsigned char kPolicyAlternate = 0;
// Branch based on left state only, because right ran out or this is a left tree.
const unsigned char kPolicyOneLeft = 1;
// Branch based on right state only.
const unsigned char kPolicyOneRight = 2;
// Reveal everything in the next branch.  Used to terminate the left/right policies.
//    static const unsigned char kPolicyEverything = 3;

} // namespace

namespace {
struct GreaterByScore : public std::binary_function<const HypoState &, const HypoState &, bool> {
  bool operator()(const HypoState &first, const HypoState &second) const {
    return first.score > second.score;
  }
};
} // namespace

void VertexNode::FinishRoot() {
  std::sort(hypos_.begin(), hypos_.end(), GreaterByScore());
  extend_.clear();
  // HACK: extend to one hypo so that root can be blank.
  state_.left.full = false;
  state_.left.length = 0;
  state_.right.length = 0;
  right_full_ = false;
  niceness_ = 0;
  policy_ = kPolicyAlternate;
  if (hypos_.size() == 1) {
    extend_.resize(1);
    extend_.front().AppendHypothesis(hypos_.front());
    extend_.front().FinishedAppending(0, 0);
  }
  if (hypos_.empty()) {
    bound_ = -INFINITY;
  } else {
    bound_ = hypos_.front().score;
  }
}

void VertexNode::FinishedAppending(const unsigned char common_left, const unsigned char common_right) {
  assert(!hypos_.empty());
  assert(extend_.empty());
  bound_ = hypos_.front().score;
  state_ = hypos_.front().state;
  bool all_full = state_.left.full;
  bool all_non_full = !state_.left.full;
  DetermineSame<lm::ngram::Left> left(state_.left, common_left);
  DetermineSame<lm::ngram::Right> right(state_.right, common_right);
  for (std::vector<HypoState>::const_iterator i = hypos_.begin() + 1; i != hypos_.end(); ++i) {
    all_full &= i->state.left.full;
    all_non_full &= !i->state.left.full;
    left.Consider(i->state.left);
    right.Consider(i->state.right);
  }
  state_.left.full = all_full && left.Complete();
  right_full_ = all_full && right.Complete();
  state_.left.length = left.Shared();
  state_.right.length = right.Shared();

  if (!all_full && !all_non_full) {
    policy_ = kPolicyAlternate;
  } else if (left.Complete()) {
    policy_ = kPolicyOneRight;
  } else if (right.Complete()) {
    policy_ = kPolicyOneLeft;
  } else {
    policy_ = kPolicyAlternate;
  }
  niceness_ = state_.left.length + state_.right.length;
}

void VertexNode::BuildExtend() {
  // Already built.
  if (!extend_.empty()) return;
  // Nothing to build since this is a leaf.
  if (hypos_.size() <= 1) return;
  bool left_branch = true;
  switch (policy_) {
    case kPolicyAlternate:
      left_branch = (state_.left.length <= state_.right.length);
      break;
    case kPolicyOneLeft:
      left_branch = true;
      break;
    case kPolicyOneRight:
      left_branch = false;
      break;
  }
  if (left_branch) {
    Split(DivideLeft(state_.left.length), hypos_, extend_);
  } else {
    Split(DivideRight(state_.right.length), hypos_, extend_);
  }
  for (std::vector<VertexNode>::iterator i = extend_.begin(); i != extend_.end(); ++i) {
    // TODO: provide more here for branching?
    i->FinishedAppending(state_.left.length, state_.right.length);
  }
}

} // namespace search
