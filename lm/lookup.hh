#ifndef LM_LOOKUP_H
#define LM_LOOKUP_H

#include "model.hh"

namespace lm {
namespace ngram {
namespace detail {

void CopyRemainingHistory(const WordIndex *from, State &out_state);


/**
 * State-machine setting off queries for a succession of n-gram orders
 * of a single n-gram. Replacement for FullScore(), if you like.
 *
 * (Keeping state for pipelined single hashtable queries: what was in
 *  - ScoreExceptBackoff()
 *  - ResumeScore()
 * before.)
 *
 * Can be used for several queries, just call Init() before each one.
 *
 * Then, keep calling RunState() until it returns false.
 *  The idea is to do useful work in between calls to RunState(),
 *  either brief other computation or RunState() of different lookups
 *  in a round-robin fashion (4 queries in flight seem fastest).
 *
 * Finally, obtain the results with GetOutState() and FullScore().
 *
 */
template <class Search, class VocabularyT>
class PrefetchLookup {
public:
  // dummy, do not use if you can help it.
  PrefetchLookup(): vocab_(NULL), search_(NULL), order_(0) { }

  PrefetchLookup(const VocabularyT *vocab, const Search *search, unsigned char order): vocab_(vocab), search_(search), order_(order) {
    in_state.length = 0;
  }

  /**
   * Single-time call constructor replacement.
   */
  void Construct(const VocabularyT *vocab, const Search *search, unsigned char order) {
    vocab_ = vocab;
    search_ = search;
    order_ = order;
  }

  /**
   * Prepare for the PrefetchLookup of a single n-gram continuing in_state with new_word.
   * in_state may be our previous out_state, since it is copied.
   */
  void Init(const State& in_state, const WordIndex new_word) {
    this->in_state = in_state; // copy
    Init(this->in_state.words, this->in_state.words + this->in_state.length, new_word);
  }

  /**
   * Prepare for the lookup of a single n-gram continuing the given (backward) context with new_word.
   */
  void Init(const WordIndex *const context_rbegin, const WordIndex *const context_rend, const WordIndex new_word) {
    order_minus_2 = 0;

    this->context_rbegin = context_rbegin;
    this->context_rend = context_rend;
    this->node = typename Search::Node();

    assert(new_word < vocab_->Bound());
    // ret.ngram_length contains the last known non-blank ngram length.
    ret.ngram_length = 1;

    typename Search::UnigramPointer uni(search_->LookupUnigram(new_word, node, ret.independent_left, ret.extend_left));
    out_state.backoff[0] = uni.Backoff();
    ret.prob = uni.Prob();
    ret.rest = uni.Rest();

    // This is the length of the context that should be used for continuation to the right.
    out_state.length = HasExtension(out_state.backoff[0]) ? 1 : 0;
    // We'll write the word anyway since it will probably be used and does no harm being there.
    out_state.words[0] = new_word;

    hist_iter = context_rbegin;
    backoff_out = out_state.backoff + 1;

    // prefetch first address, if necessary
    if (context_rbegin == context_rend)
      return;

    // prefetch
    it = search_->MiddleAdvance(order_minus_2, *hist_iter, node);
    __builtin_prefetch(it, 0, 0);

    if(Order() == 2) {
      // in this case, instead of address calculation in RunState()
      itl = search_->LongestAdvance(*hist_iter, node);
      __builtin_prefetch(itl, 0, 0);
    }
  }

  /** Returns true if still needs to run. */
  bool RunState() {
    if (hist_iter == context_rend) return Final();
    if (ret.independent_left) return Final();
    if (order_minus_2 == Order() - 2) {
      Longest();
      return Final();
    }

    typename Search::MiddlePointer pointer(search_->LookupMiddleFromIterator(order_minus_2, node, ret.independent_left, ret.extend_left, it));
    if (!pointer.Found()) return Final();
    *backoff_out = pointer.Backoff();
    ret.prob = pointer.Prob();
    ret.rest = pointer.Rest();
    ret.ngram_length = order_minus_2 + 2;
    if (HasExtension(*backoff_out)) {
      out_state.length = ret.ngram_length;
    }

    ++order_minus_2, ++hist_iter, ++backoff_out;

    // prefetch
    if (hist_iter != context_rend) {
      if (order_minus_2 != Order() - 2) {
        it = search_->MiddleAdvance(order_minus_2, *hist_iter, node);
        __builtin_prefetch(it, 0, 0);
      } else {
        itl = search_->LongestAdvance(*hist_iter, node);
        __builtin_prefetch(itl, 0, 0);
      }
    }

    return true;
  }

  State &GetOutState() { return out_state; }

  // this score does not yet include backoffs
  FullScoreReturn &GetRet() { return ret; }

  // MUST have provided in_state, i.e. use Init(const State& in_state, const WordIndex new_word)
  FullScoreReturn FullScore() {
    assert(in_state.length != 0); // make sure user used the correct Init()

    FullScoreReturn ret_full = ret;
    for (const float *i = in_state.backoff + ret.ngram_length - 1; i < in_state.backoff + in_state.length; ++i) {
      ret_full.prob += *i;
    }
    return ret_full;
  }

  unsigned char Order() { return order_; }

private:
  void Longest() {
    ret.independent_left = true;
    typename Search::LongestPointer longest(search_->LookupLongestFromIterator(node, itl));
    if (longest.Found()) {
      ret.prob = longest.Prob();
      ret.rest = ret.prob;
      // There is no blank in longest_.
      ret.ngram_length = Order();
    }
  }

  bool Final() {
    CopyRemainingHistory(context_rbegin, out_state);
    in_state.length = 0;
    return false;
  }

  unsigned char order_minus_2;
  FullScoreReturn ret;
  typename Search::Node node;
  typename Search::Middle::ConstIterator it;
  typename Search::Longest::ConstIterator itl;

  const WordIndex *hist_iter;
  float *backoff_out;

  const WordIndex *context_rbegin;
  const WordIndex *context_rend;
  State in_state;
  State out_state;

  // from GenericModel
  const VocabularyT *vocab_;
  const Search *search_;
  unsigned char order_;
};


} // namespace detail
} // namespace ngram
} // namespace lm

#endif /* LM_LOOKUP_H */
