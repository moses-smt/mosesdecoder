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

template <class Automaton, unsigned Size> class Queue {
  typedef typename Automaton::Task Task;
  public:
    template <class Construct> explicit Queue(Construct automaton_construct) : curr_(0) {
      for (std::size_t i = 0; i < Size; ++i) {
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
      curr_ = (curr_ + 1) % Size;
    }

    void Drain() {
      std::size_t drained = 0;
      while (drained != Size) {
        while (automata_[curr_].Poke() != STOP) {}
        Next();
        ++drained;
      }
    }

  private:
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

    enum State {DONE, PREFETCH_UNIGRAM, GET_UNIGRAM, PREFETCH_MIDDLE, GET_MIDDLE, PREFETCH_LONGEST, GET_LONGEST};

    NGramAutomaton(detail::HashedSearch<Value> &search) : state_(0), search_(search), middle_(0) {}

    Signal Poke() {
      switch(state_) {
        case DONE: 
          std::cout << "Should do callback now" << std::endl;
          return STOP;
        case PREFETCH_UNIGRAM:
          //Prefetch unigram
          search_.PrefetchUnigram(task_.new_word);
          state_ = GET_UNIGRAM;
          return CONTINUE;
        case GET_UNIGRAM:
          //Get unigram
          typename detail::HashedSearch<Value>::Node node;
          typename detail::HashedSearch<Value>::UnigramPointer uni(search_.LookupUnigram(task_.new_word, node, ret_.independent_left, ret_.extend_left));
          task_.out_state.backoff[0] = uni.Backoff();
          ret_.prob = uni.Prob();
          ret_.rest = uni.Rest();
          ret_.ngram_length = 1;
          task_.out_state.length = HasExtension(task_.out_state.backoff[0]) ? 1 : 0;
          task_.out_state.words[0] = task_.new_word;
          if (task_.in_state.length == 0) state_ = DONE;
          else state_ = PREFETCH_MIDDLE;
          return CONTINUE;
        case PREFETCH_MIDDLE:
          //Prefetch middle
          return CONTINUE;
        case GET_MIDDLE:
          //Get middle
          return CONTINUE;
        case PREFETCH_LONGEST:
          //Prefetch longest
          return CONTINUE;
        case GET_LONGEST:
          //Get longest
          return CONTINUE;
        default:
          std::cerr << "Error!" << std::endl;
      }
    }

    void SetTask(const Task& task) {
      task_ = task;
      ret_ = FullScoreReturn();
      state_ = 1;
      middle_ = 0;
    }
    
  private:
    State state_;
    Task task_;
    FullScoreReturn ret_;
    detail::HashedSearch<Value> &search_;
    unsigned short middle_;
};

} // namespace ngram
} // namespace lm

int main(){
  std::cout << "It is working\n";
  lm::Queue<lm::SimpleAutomaton, 2> q(10);
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
