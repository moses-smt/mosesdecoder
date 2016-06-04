#ifndef LM_AUTOMATON_H
#define LM_AUTOMATON_H

#include "lm/word_index.hh"
#include "lm/state.hh"
#include "lm/return.hh"
#include "lm/search_hashed.hh"
#include "lm/value.hh"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace lm {

    enum class State {Stop, Continue};

    namespace {

        class SimpleAutomaton {
            public:
                using Task = std::pair<std::string, unsigned int>;
                using Construct = int;

                SimpleAutomaton(int x = 0) : repeat_(0) {}

                State Step() {
                    if (repeat_ <= 0) return State::Stop;
                    std::cout << word_ << std::endl;
                    --repeat_;
                    return State::Continue;
                }

                void SetTask(const Task& task){
                    repeat_ = task.second;
                    word_ = task.first;
                }

            private:
                std::size_t repeat_;
                std::string word_;
        };

    } // namespace

    class Pipeline {

        public:
            void Add(const std::vector<const WordIndex>& context, const std::vector<const WordIndex>& words) {
                // Put appropriate automata into the Queue
                // Who will apply the backoffs? The last automaton? 
            }
            void Drain() {}
    };


    template <class Automaton> class Queue {
        using Task = typename Automaton::Task;
        using Construct = typename Automaton::Construct;
        public:
        explicit Queue(std::size_t size, Construct construct) : size_(size), curr_(0), automata_(size, Automaton(construct)) {
            std::cout << "Creating queue\n"; 
        }

        void Add(const Task task) {
            while (automata_[curr_].Step() != State::Stop) {
                Next();
            }
            automata_[curr_].SetTask(task);
            //TODO: What about single step automata?
            automata_[curr_].Step();
            Next();
        }

        void Next() {
            curr_ = (curr_ + 1) % size_;
        }

        void Drain() {
            // TODO: what if the Queue was not filled before draining?
            // an automaton without a task should be in State::Stop
            std::size_t drained = 0;
            while (drained != size_) {
                while (automata_[curr_].Step() != State::Stop) {}
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
                    const WordIndex *context_rbegin;
                    const WordIndex new_word;
                    const unsigned char* in_length;
                    unsigned char* out_length;
                    float backoffs[KENLM_MAX_ORDER - 1];
                    FullScoreReturn ret;
                };

                using Task = NGramTask;
                using Construct = detail::HashedSearch<Value>&;
                using State = lm::State;

                explicit NGramAutomaton(Construct construct) : ngram_order_(0), state_(State::Stop), task_(nullptr), search_(construct), node_(0), out_length_is_written_(false) {
                    std::cout << "Creating automaton\n";
                }

                State Poke() {
                    if (state_ == State::Stop || ngram_order_ > KENLM_MAX_ORDER){
                        return State::Stop;
                    }

                    switch(ngram_order_) {
                        case 0:
                            //prefetch unigram only
                            search_.PrefetchUnigram(*(task_->new_word));
                            break;
                        case 1:
                            GetUnigramPrefetchNext();
                            break;
                        case KENLM_MAX_ORDER:
                            GetLongest();
                            break;
                        default:
                            GetMiddlePrefetchNext();
                            break;
                    }
                    ngram_order_ += 1;
                    return state_;
                    
                }

                void SetTask(Task* task) {
                    task_ = task;
                    ngram_order_ = 0;
                    out_length_is_written_ = false;
                    state_ = State::Continue;
                    node_ = 0;
                }

            private:

                void GetUnigramPrefetchNext(){
                    typename detail::HashedSearch<Value>::UnigramPointer uni(search_.LookupUnigram(*(task_->new_word), node_, task_->ret.independent_left, task_->ret.extend_left));
                    //TODO: What if the word is not found?
                    (task_->backoffs)[0] = uni.Backoff();
                    task_->ret.prob = uni.Prob();
                    task_->ret.rest = uni.Rest();
                    task_->ret.ngram_length = 1;
                    if (!HasExtension(uni.Backoff())) {
                        WriteOutLength(0);
                    }
                    if (*(task_->in_length) == 0 || task_->ret.independent_left) {
                        Stop();
                    }
                    else {
                        if (KENLM_MAX_ORDER == 2) {
                            //for bigrams we don't prefetch middle since there are none
                            search_.PrefetchLongest();
                        }
                        else {
                            search_.PrefetchMiddle(0, (task_->context_rbegin)[0]);
                        }
                    }

                }

                void GetMiddlePrefetchNext(){
                    typename detail::HashedSearch<Value>::MiddlePointer pointer(search_.LookupMiddleFromNode(ngram_order_ - 2, node_, task_->ret.independent_left, task_->ret.extend_left));
                    if (!pointer.Found()) {
                        Stop();
                        return;
                    }
                    (task_->backoffs)[ngram_order_-1] = pointer.Backoff();
                    task_->ret.prob = pointer.Prob();
                    task_->ret.rest = pointer.Rest();
                    task_->ret.ngram_length = ngram_order_;

                    if (!HasExtension(pointer.Backoff())){
                        //TODO: could possibly make things faster if the condition is 'if(!out_length_is_written_ && !HasExtension(...))
                        WriteOutLength(ngram_order_-1); 
                    }

                    if (ngram_order_ - 1 == *(task_->in_length) || task_->ret.independent_left) {
                        Stop();
                        return;
                    }

                    if (ngram_order_ + 1 == KENLM_MAX_ORDER){
                        search_.PrefetchLongest((task_->context_rbegin)[ngram_order_ - 1], node_);
                    }
                    else {
                        search_.PrefetchMiddle(ngram_order_ - 1, (task_->context_rbegin)[ngram_order_ - 1], node_);
                    }
                }

                void GetLongest(){
                    task_->ret.independent_left = true;
                    typename detail::HashedSearch<Value>::LongestPointer longest(search_.LookupLongestFromNode(node_));
                    if (longest.Found()) {
                        task_->ret.prob = longest.Prob();
                        task_->ret.rest = task_->ret.prob;
                        task_->ret.ngram_length = ngram_order_;
                    }
                    Stop();
                }

                void WriteOutLength(const unsigned char out_length){
                    if (!out_length_is_written_) {
                        out_length_is_written_ = true;
                        *(task_->out_length) = out_length;
                    }
                }

                void Stop(){
                    WriteOutLength(std::min(task_->ret.ngram_length, static_cast<unsigned char>(KENLM_MAX_ORDER - 1)));
                    state_ = State::Stop;
                }


                std::size_t ngram_order_;
                bool out_length_is_written_;
                State state_;
                Task* task_;
                detail::HashedSearch<Value> &search_;
                typename detail::HashedSearch<Value>::Node node_;
        };

    } // namespace ngram
} // namespace lm

int main(){
    auto x = 10;
    std::cout << x << std::endl;
    auto f = [&x](int y){return x+y;};
    std::cout << f(32) << std::endl;
    int* y = nullptr;
    std::cout << y;
    if (y == 0) {
        std::cout << "really?!\n";
    }
    typename lm::ngram::detail::HashedSearch<lm::ngram::BackoffValue> hs;
    lm::ngram::NGramAutomaton<lm::ngram::BackoffValue> test(hs);
    std::cout<<"just created an ngram automaton\n"; 
    lm::Queue<lm::ngram::NGramAutomaton<lm::ngram::BackoffValue>> queue(20, hs);
    std::cout << "It is working...\n";
    lm::Queue<lm::SimpleAutomaton> q(20, 10);
    std::cout << "Add hello"<<std::endl;
    q.Add(std::make_pair("Hello", 3));
    std::cout << "Add Bye"<<std::endl;
    q.Add(std::make_pair("Bye", 5));
    std::cout << "Add C U"<<std::endl;
    q.Add(std::make_pair("C U", 3));
    std::cout << "Drain"<<std::endl;

    q.Drain();
}
#endif //LM_AUTOMATON_H
