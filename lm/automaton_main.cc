#ifndef LM_AUTOMATON_H
#define LM_AUTOMATON_H

#include "lm/word_index.hh"
#include "lm/state.hh"

#include <iostream>
#include <string>
#include <utility>

namespace lm {

enum Signal {STOP, CONTINUE};

namespace {

class SimpleAutomaton {
  public:
    typedef std::pair<std::string, unsigned int> Task;

    SimpleAutomaton() : state_(0) {}

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
    explicit Queue() : curr_(automata_) {}

    void Add(const Task task) {
      while (curr_->Poke() != STOP) {
        Next();
      }
      curr_->SetTask(task);
      Next();
    }

    void Next() {
      curr_ = (curr_ + 1 - automata_) % Size + automata_;
    }

    void Drain() {
      std::size_t drained = 0;
      while (drained != Size) {
        while (curr_->Poke() != STOP) {}
        Next();
        ++drained;
      }
    }

  private:
    Automaton automata_[Size];
    Automaton *curr_;
};

namespace ngram {

class NGramAutomaton {
  public:
    struct NGramTask {
      State in_state;
      WordIndex new_word;
      State out_state;
    };
    typedef NGramTask Task;

    NGramAutomaton() : state_(0) {}

    Signal Poke() {
      switch(state_) {
        case 0: 
          std::cout << "Should do callback now" << std::endl;
          return STOP;
        case 1:
          //Prefetch unigram
          return CONTINUE;
        case 2:
          //Get unigram
          return CONTINUE;
        case 3:
          //Prefetch middle
          return CONTINUE;
        case 4:
          //Get middle
          return CONTINUE;
        case 5:
          //Prefetch longest
          return CONTINUE;
        default:
          std::cerr << "Error!" << std::endl;
      }
    }

    void SetTask(const Task& task) {
      task_ = task;
      state_ = 1;
    }
    
  private:
    std::size_t state_;
    Task task_;
};

} // namespace ngram
} // namespace lm

int main(){
  std::cout << "It is working!!\n";
  lm::Queue<lm::SimpleAutomaton, 2> q;
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
