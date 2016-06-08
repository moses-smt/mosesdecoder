#ifndef LM_AUTOMATON_H
#define LM_AUTOMATON_H

#include "lm/word_index.hh"
#include "lm/state.hh"
#include "lm/return.hh"
#include "lm/search_hashed.hh"
#include "lm/value.hh"
#include "lm/blank.hh"

#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <array>

namespace lm {

enum class State {Done, Working};

namespace {

class SimpleAutomaton {
    public:
        using Task = std::pair<std::string, unsigned int>;
        using Construct = int;

        SimpleAutomaton(int x = 0) : repeat_(0) {}

        State Step() {
            if (repeat_ <= 0) return State::Done;
            std::cout << word_ << std::endl;
            --repeat_;
            return State::Working;
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

namespace ngram {

template <class Value> class NGramAutomaton {
    public:
        struct NGramTask {
            NGramAutomaton<Value>* pred;
            const WordIndex* const context_rbegin;
            const WordIndex* const context_rend;
            const WordIndex new_word;
        };

        using Task = NGramTask;
        using Construct = detail::HashedSearch<Value>&;
        using State = lm::State;

        explicit NGramAutomaton(Construct construct) : ngram_order_(0),
        state_(State::Done),
        search_(construct),
        node_(0),
        pred_(nullptr),
        succ_(nullptr),
        out_length_(0),
        out_backoffs_(),
        pred_finished_(false), 
        pred_data_(),
        succ_data_(),
        new_word_(),
        context_rbegin_(nullptr),
        context_rend_(nullptr) {
            std::cout << "Creating NGramAutomaton\n";
        }

        State Step() {
            if (state_ == State::Done || ngram_order_ > KENLM_MAX_ORDER){
                return State::Done;
            }

            switch(ngram_order_) {
                case 0:
                    //prefetch unigram only
                    search_.PrefetchUnigram(new_word_);
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

        void SetTask(const Task& task) {

            context_rbegin_ = task.context_rbegin;
            context_rend_ = task.context_rend;
            new_word_ = task.new_word;

            pred_ = task.pred;
            pred_data_.length = std::min(static_cast<std::size_t>(context_rend_ - context_rbegin_), static_cast<std::size_t>(KENLM_MAX_ORDER - 1));
            InitialPredecessorCheck();

            ngram_order_ = 0;
            node_ = 0;
            succ_ = nullptr;
            out_length_ = 0;
            state_ = State::Working;
        }

    private:
        struct SuccessorData {
            FullScoreReturn ret;
            std::function<void()> callback;
        };

        struct PredecessorData {
            std::array<float, KENLM_MAX_ORDER - 1> backoffs;
            unsigned char length;
        };

        void InitialPredecessorCheck() {
            if (pred_) {
                if (pred_->Finished() || this == pred_) {
                    // the second condition (this == pred_) does not need to be there
                    // if we assume this->state_ == State::Done when this function is called
                    pred_finished_ = true;
                    CopyStateFromPredecessorWithoutCheck();
                }
                else {
                    pred_finished_ = false;
                    NotifyPredecessorWithoutCheck();
                }
            }
        }

        void CopyStateFromPredecessorWithoutCheck() {
            pred_data_.backoffs = pred_->GetOutBackoffs();
            pred_data_.length = pred_->GetOutLength();
        }

        void NotifyPredecessorWithoutCheck(){
            pred_->SetSuccessor(this);
        }

        void CheckSuccessorFinished(){
            if (succ_) {
                if (succ_finished_) {
                    // apply backoffs to fullscorereturn and call callback
                    for(auto i = succ_data_.ret.ngram_length - 1; i < out_length_; i++){
                        succ_data_.ret.prob += out_backoffs_[i];
                    }
                    succ_data_.callback();
                }
                else {
                    // transfer backoffs to successor so he can apply them himself
                    NotifySuccessorOfCompletion();
                }
            }
        }

        void CheckPredecessorFinished(){
            if (pred_) {
                if (pred_finished_) {
                    // apply backoffs from predecessor and call callback
                    for(auto i = ret_.ngram_length - 1; i < pred_data_.length; i++){
                        ret_.prob += pred_data_.backoffs[i];
                    }
                    callback_();
                }
                else {
                    // Give callback and fullscorereturn to predecessor
                    NotifyPredecessorOfCompletion();
                }
            }
        }

        void NotifyPredecessorOfCompletion() {
            pred_->succ_finished_ = true;
            pred_->succ_data_.callback = callback_;
            pred_->succ_data_.ret = ret_;
        }

        void NotifySuccessorOfCompletion() {
            succ_->pred_finished_ = true;
            succ_->pred_data_. backoffs = out_backoffs_;
            succ_->pred_data_.length = out_length_;
        }

        bool Finished() {
            return state_ == State::Done;
        }

        const std::array<float, KENLM_MAX_ORDER - 1>& GetOutBackoffs() {
            return out_backoffs_;
        }

        unsigned char GetOutLength() {
            return out_length_;
        }

        void SetSuccessor(NGramAutomaton<Value>* succ){
            succ_ = succ;
        }

        State GetState() {
            return state_;
        }


        void GetUnigramPrefetchNext(){
            typename detail::HashedSearch<Value>::UnigramPointer uni(search_.LookupUnigram(new_word_, node_, ret_.independent_left, ret_.extend_left));
            //TODO: What if the word is not found?
            out_backoffs_[0] = uni.Backoff();
            ret_.prob = uni.Prob();
            ret_.rest = uni.Rest();
            ret_.ngram_length = 1;
            if (!HasExtension(uni.Backoff())) {
                WriteOutLength(0);
            }
            if (pred_data_.length == 0 || ret_.independent_left) {
                Finish();
            }
            else {
                if (KENLM_MAX_ORDER == 2) {
                    //for bigrams we don't prefetch middle since there are none
                    search_.PrefetchLongest(context_rbegin_[ngram_order_ - 1], node_);
                }
                else {
                    search_.PrefetchMiddle(0, context_rbegin_[0], node_);
                }
            }

        }

        void GetMiddlePrefetchNext(){
            typename detail::HashedSearch<Value>::MiddlePointer pointer(search_.LookupMiddleFromNode(ngram_order_ - 2, node_, ret_.independent_left, ret_.extend_left));
            if (!pointer.Found()) {
                Finish();
                return;
            }
            out_backoffs_[ngram_order_-1] = pointer.Backoff();
            ret_.prob = pointer.Prob();
            ret_.rest = pointer.Rest();
            ret_.ngram_length = ngram_order_;

            if (!HasExtension(pointer.Backoff())){
                //TODO: could possibly make things faster if the condition is 'if(!out_length_is_written_ && !HasExtension(...))
                WriteOutLength(ngram_order_-1); 
            }

            if (ngram_order_ - 1 == pred_data_.length || ret_.independent_left) {
                Finish();
                return;
            }

            if (ngram_order_ + 1 == KENLM_MAX_ORDER){
                search_.PrefetchLongest(context_rbegin_[ngram_order_ - 1], node_);
            }
            else {
                search_.PrefetchMiddle(ngram_order_ - 1, context_rbegin_[ngram_order_ - 1], node_);
            }
        }

        void GetLongest(){
            ret_.independent_left = true;
            typename detail::HashedSearch<Value>::LongestPointer longest(search_.LookupLongestFromNode(node_));
            if (longest.Found()) {
                ret_.prob = longest.Prob();
                ret_.rest = ret_.prob;
                ret_.ngram_length = ngram_order_;
            }
            Finish();
        }

        void WriteOutLength(const unsigned char out_length){
            if (succ_ && !succ_finished_ && succ_->pred_data_.length > out_length) {
                succ_->pred_data_.length = out_length;
            }
        }

        void Finish(){
            WriteOutLength(std::min(ret_.ngram_length, static_cast<unsigned char>(KENLM_MAX_ORDER - 1)));
            CheckPredecessorFinished();
            CheckSuccessorFinished();
            state_ = State::Done;
        }


        std::size_t ngram_order_;
        bool out_length_is_written_;
        State state_;
        detail::HashedSearch<Value> &search_;
        typename detail::HashedSearch<Value>::Node node_;
        std::function<void()> callback_ = [](){std::cout << "In callback\n";};
        unsigned char out_length_;
        std::array<float, KENLM_MAX_ORDER - 1> out_backoffs_;
        bool pred_finished_;
        bool succ_finished_;
        NGramAutomaton<Value>* pred_;
        NGramAutomaton<Value>* succ_;
        PredecessorData pred_data_;
        SuccessorData succ_data_; 
        WordIndex new_word_;
        const WordIndex* context_rbegin_;
        const WordIndex* context_rend_;
        FullScoreReturn ret_;
};

} // namespace ngram

template <class Automaton> class Queue {
    using Task = typename Automaton::Task;
    using Construct = typename Automaton::Construct;
    public:
        explicit Queue(std::size_t size, Construct construct) : size_(size), curr_(0), automata_(size, Automaton(construct)) {
            std::cout << "Creating queue\n"; 
        }

        Automaton* Add(const Task task) {
            while (automata_[curr_].Step() != State::Done) {
                Next();
            }
            automata_[curr_].SetTask(task);
            //TODO: What about single step automata?
            automata_[curr_].Step();
            auto& ret = automata_[curr_];
            Next();
            return &ret;
        }

        void Next() {
            curr_ = (curr_ + 1) % size_;
        }

        void Drain() {
            std::size_t drained = 0;
            while (drained != size_) {
                while (automata_[curr_].Step() != State::Done) {}
                Next();
                ++drained;
            }
        }

    private:
        std::size_t size_;
        std::size_t curr_;
        std::vector<Automaton> automata_;
};


class Pipeline {

    public:
        Pipeline(std::size_t queue_size, ngram::detail::HashedSearch<ngram::BackoffValue>& search) : queue_(queue_size, search) {}
        void Add(const WordIndex* const r_context_begin, std::size_t context_length, const WordIndex* const new_words, std::size_t new_words_length) {
            auto context_begin = r_context_begin;
            ngram::NGramAutomaton<ngram::BackoffValue>* pred = nullptr;
            for (std::size_t i = 0; i < new_words_length; i++) {
                auto length = std::min<std::size_t>(KENLM_MAX_ORDER - 1, new_words_length - i);
                auto new_word = *new_words;
                auto context_end = context_begin + length;
                Task task_{pred, context_begin, context_end, new_word};
                pred = queue_.Add(task_);

                context_begin++;
            }


        }
        void Drain() {
            queue_.Drain();
        }


    private:
        using Task = ngram::NGramAutomaton<ngram::BackoffValue>::Task;

        Queue<ngram::NGramAutomaton<ngram::BackoffValue>> queue_;

};

} // namespace lm

int main(){
    typename lm::ngram::detail::HashedSearch<lm::ngram::BackoffValue> hs;
    lm::Pipeline pipeline(10, hs);


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
