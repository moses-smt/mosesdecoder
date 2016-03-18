#ifndef LM_AUTOMATON_H
#define LM_AUTOMATON_H

#include "lm/word_index.hh"
#include "lm/state.hh"
#include "lm/return.hh"
#include "lm/search_hashed.hh"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace lm {

enum Signal {STOP, CONTINUE};

namespace {

class SimpleAutomaton {
  public:
    typedef std::pair<std::string, unsigned int> Task;

    SimpleAutomaton(int) : state_(0) {}

    Signal Poke() {
      if (state_ <= 0) return STOP;
      std::cout << word_ << std::endl;
      --state_;
      return CONTINUE;
    }
    
    void SetTask(const Task& task){
      state_ = task.second;
      word_ = task.first;
    }

  private:
    std::size_t state_;
    std::string word_;
};

} // namespace

template <class Automaton> class Queue {
  typedef typename Automaton::Task Task;
  public:
    template <class Construct> explicit Queue(std::size_t size, Construct automaton_construct) : size_(size), curr_(0) {
      for (std::size_t i = 0; i < size_; ++i) {
        automata_.push_back(Automaton(automaton_construct));
      }
    }

    void Add(const Task task) {
      while (automata_[curr_].Poke() != STOP) {
        Next();
      }
      automata_[curr_].SetTask(task);
      Next();
    }

    void Next() {
      curr_ = (curr_ + 1) % size_;
    }

    void Drain() {
      std::size_t drained = 0;
      while (drained != size_) {
        while (automata_[curr_].Poke() != STOP) {}
        Next();
        ++drained;
      }
    }

  private:
    std::size_t size_;
    std::size_t curr_;
    std::vector<Automaton> automata_;
};

namespace ngram {

template <class Value> class NGramAutomaton {
  public:
    struct NGramTask {
      State in_state;
      WordIndex new_word;
      State out_state;
    };
    typedef NGramTask Task;

    struct Construct {
      unsigned short order;
      detail::HashedSearch<Value> *search;
    };

    enum State {DONE, PREFETCH_UNIGRAM, GET_UNIGRAM, GET_MIDDLE, GET_LONGEST};

    explicit NGramAutomaton(Construct construct) : state_(0), search_(*construct.search), middle_(0), order_(construct.order) {}

    Signal Poke() {
      switch(state_) {
        case DONE: 
          std::cout << "This should not be happenning!" << std::endl;
          return STOP;
        case PREFETCH_UNIGRAM:
          //Prefetch unigram
          search_.PrefetchUnigram(task_.new_word);
          state_ = GET_UNIGRAM;
          return CONTINUE;
        case GET_UNIGRAM:
          //Get unigram
          GetUnigramPrefetchMiddle();
          break;
        case GET_MIDDLE:
          //Get middle
          GetMiddlePrefetchNext();
          break;
        case GET_LONGEST:
          //Get longest
          GetLongest();
          break;
        default:
          std::cerr << "Error!" << std::endl;
      }

      if (state_ == DONE){
        // copy remaining context words - assuming new word was copied already
        WordIndex *out = task_.out_state.words + 1;
        const WordIndex *in_end = task_.in_state.words + static_cast<ptrdiff_t>(task_.out_state.length) - 1;
        for (const WordIndex *in = task_.in_state.words; in < in_end; ++in, ++out) *out = *in;

        // should add the backoffs here - refactor model.cc and have just one method call or copy/paste :-(
        for (const float *i = task_.in_state.backoff + ret_.ngram_length - 1; i < task_.in_state.backoff + task_.in_state.length; ++i) {
          ret_.prob += *i;
        }
        std::cout << "Should do callback now" << std::endl;
        return STOP;
      }
      return CONTINUE;
    }

    void SetTask(const Task& task) {
      task_ = task;
      ret_ = FullScoreReturn();
      state_ = PREFETCH_UNIGRAM;
      middle_ = 0;
    }

  private:
    void GetUnigramPrefetchMiddle(){
      typename detail::HashedSearch<Value>::UnigramPointer uni(search_.LookupUnigram(task_.new_word, node_, ret_.independent_left, ret_.extend_left));
      task_.out_state.backoff[0] = uni.Backoff();
      ret_.prob = uni.Prob();
      ret_.rest = uni.Rest();
      ret_.ngram_length = 1;
      task_.out_state.length = HasExtension(task_.out_state.backoff[0]) ? 1 : 0;
      task_.out_state.words[0] = task_.new_word;
      state_ = task_.in_state.length == 0 || ret_.independent_left ? DONE : GET_MIDDLE;

      //Prefetch bigram - this might be wasteful if state is DONE, but cheaper than if statement?
      search_.PrefetchMiddle(0, task_.in_state.words[0], node_);
    }

    void GetMiddlePrefetchNext(){
      // TODO: this will break on bigram language models
      typename detail::HashedSearch<Value>::MiddlePointer pointer(search_.LookupMiddleFromNode(middle_, node_, ret_.independent_left, ret_.extend_left));
      if (!pointer.Found()) return;
      task_.out_state.backoff[middle_+1] = pointer.Backoff();
      ret_.prob = pointer.Prob();
      ret_.rest = pointer.Rest();
      ret_.ngram_length = middle_ + 2;
      if (HasExtension(pointer.Backoff())) task_.out_state.length = ret_.ngram_length;

      ++middle_;
      if (middle_ == task_.in_state.length || ret_.independent_left) {
        state_ = DONE;
        return;
      }
      if (middle_ == order_ - 2){
        state_ = GET_LONGEST;
        search_.PrefetchLongest(task_.in_state.words[middle_], node_);
      }
      else {
        search_.PrefetchMiddle(middle_, task_.in_state.words[middle_], node_);
      }
    }

    void GetLongest(){
      ret_.independent_left = true;
      typename detail::HashedSearch<Value>::LongestPointer longest(search_.LookupLongestFromNode(node_));
      if (longest.Found()) {
        ret_.prob = longest.Prob();
        ret_.rest = ret_.prob;
        ret_.ngram_length = order_;
      }
      state_ = DONE;
    }

    
    State state_;
    Task task_;
    FullScoreReturn ret_;
    detail::HashedSearch<Value> &search_;
    unsigned short middle_;
    typename detail::HashedSearch<Value>::Node node_;
    unsigned short order_;
};

} // namespace ngram
} // namespace lm

int main(){
  std::cout << "It is working...\n";
  lm::Queue<lm::SimpleAutomaton> q(2, 10);
  std::cout << "Add hello"<<std::endl;
  q.Add(std::make_pair("Hello", 3));
  std::cout << "Add Bye"<<std::endl;
  q.Add(std::make_pair("Bye", 1));
  std::cout << "Add C U"<<std::endl;
  q.Add(std::make_pair("C U", 3));
  std::cout << "Drain"<<std::endl;

  q.Drain();
}
#endif //LM_AUTOMATON_H
