#ifndef LM_AUTOMATON_H
#define LM_AUTOMATON_H

#undef NDEBUG

#include "lm/word_index.hh"
#include "lm/state.hh"
#include "lm/return.hh"
#include "lm/search_hashed.hh"
#include "lm/value.hh"
#include "lm/blank.hh"
#include "lm/config.hh"
#include "lm/model.hh"

#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <array>
#include <assert.h>
#include <string>

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

        struct NGramConstruct {
            detail::HashedSearch<Value>& search;
            unsigned short max_order;
        };


        using Task = NGramTask;
        using Construct = NGramConstruct;
        using State = lm::State;

        explicit NGramAutomaton(Construct construct) :
            ngram_order_(0),
            state_(State::Done),
            search_(construct.search),
            max_order_(construct.max_order),
            node_(0),
            out_length_(0),
            out_backoffs_(),
            pred_finished_(false), 
            succ_finished_(false),
            pred_(nullptr),
            succ_(nullptr),
            pred_data_(),
            succ_data_(),
            new_word_(),
            context_rbegin_(nullptr),
            context_rend_(nullptr),
            ret_(){}

        State Step() {
            if (state_ == State::Done || ngram_order_ > max_order_){
                return State::Done;
            }
            std::cout << this << " step " << (int) ngram_order_ <<  std::endl;

            switch(ngram_order_) {
                case 0:
                    //prefetch unigram only
                    search_.PrefetchUnigram(new_word_);
                    break;
                case 1:
                    GetUnigramPrefetchNext();
                    break;
                default:
                    if (ngram_order_ == max_order_) GetLongest();
                    else GetMiddlePrefetchNext();
                    break;
            }
            ngram_order_ += 1;
            return state_;

        }

        void SetTask(const Task& task) {
            std::cout << this << " start" << std::endl;

            context_rbegin_ = task.context_rbegin;
            context_rend_ = task.context_rend;
            new_word_ = task.new_word;
            pred_ = task.pred;

            pred_data_.length = std::min(static_cast<std::size_t>(context_rend_ - context_rbegin_), static_cast<std::size_t>(max_order_ - 1));
            InitialPredecessorCheck();

            ngram_order_ = 0;
            node_ = 0;
            out_length_ = max_order_ - 1 ;
            ret_ = FullScoreReturn(); //TODO: is this necessary?
            succ_ = nullptr;
            succ_finished_ = false;
            state_ = State::Working;
        }

    private:
        struct SuccessorData {
            FullScoreReturn ret;
            std::function<void(FullScoreReturn&)> callback;
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
            if (succ_finished_) {
                assert(succ_!=nullptr);
                // apply backoffs to fullscorereturn and call callback
                for(auto i = succ_data_.ret.ngram_length - 1; i < out_length_; i++){
                    succ_data_.ret.prob += out_backoffs_[i];
                }
                std::cout<< this << " handling callback from " << succ_ << std::endl;
                succ_data_.callback(succ_data_.ret);
            }
            else if (succ_) {
                // transfer backoffs to successor so he can apply them himself
                NotifySuccessorOfCompletion();
            }
        }

        void CheckPredecessorFinished(){
            if (pred_finished_) {
                assert(pred_ != nullptr);
                // apply backoffs from predecessor and call callback
                for(auto i = ret_.ngram_length - 1; i < pred_data_.length; i++){
                    ret_.prob += pred_data_.backoffs[i];
                }
                std::cout << this << " callback" <<std::endl;
                callback_(ret_);
            }
            else if (pred_){
                // Give callback and fullscorereturn to predecessor
                std::cout << this << " callback left for " << pred_ << std::endl;
                NotifyPredecessorOfCompletion();
            }
            else {
                std::cout << this << "callback - no backoffs" <<std::endl;
                callback_(ret_);
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
            out_backoffs_[0] = uni.Backoff(); //TODO: What if the word is not found?
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
                if (max_order_ == 2) {
                    //for bigrams we don't prefetch middle since there are none
                    search_.PrefetchLongest(context_rbegin_[0], node_);
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
                WriteOutLength(ngram_order_-1); 
            }
            std::cout << ngram_order_ << " " << (int) pred_data_.length << std::endl;

            if (ngram_order_ - 1 == pred_data_.length || ret_.independent_left) {

                Finish();
                return;
            }

            if (ngram_order_ + 1 == max_order_){
                search_.PrefetchLongest(context_rbegin_[ngram_order_ - 1], node_);
            }
            else {
                search_.PrefetchMiddle(ngram_order_ - 1, context_rbegin_[ngram_order_ - 1], node_);
            }
        }

        void GetLongest(){
            WriteOutLength(ngram_order_-1);
            ret_.independent_left = true;
            typename detail::HashedSearch<Value>::LongestPointer longest(search_.LookupLongestFromNode(node_));
            if (longest.Found()) {
                ret_.prob = longest.Prob();
                ret_.rest = ret_.prob;
                ret_.ngram_length = ngram_order_;
            }
            Finish();
        }

        void WriteOutLength(const unsigned char length){
            if (length < out_length_) {
                out_length_ = length;
                if (succ_ && !succ_finished_) {
                    succ_->pred_data_.length = length;
                }
            }
        }

        void Finish(){
            std::cout << this << " finish" << std::endl;
            WriteOutLength(std::min(ret_.ngram_length, static_cast<unsigned char>(max_order_ - 1)));
            CheckPredecessorFinished();
            CheckSuccessorFinished();
            state_ = State::Done;
        }


        std::function<void(FullScoreReturn&)> callback_ = [](FullScoreReturn& r){std::cout << "In callback, prob: " << r.prob << " ngram_length: " << (int)r.ngram_length << std::endl;};
        std::size_t ngram_order_;
        State state_;
        detail::HashedSearch<Value> &search_;
        unsigned short max_order_;
        typename detail::HashedSearch<Value>::Node node_;
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
        explicit Queue(std::size_t size, Construct construct) : size_(size), curr_(0), automata_(size, Automaton(construct)) {}

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
                while (automata_[curr_].Step() != State::Done) {std::cout <<"step" <<std::endl;}
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
        Pipeline(std::size_t queue_size, ngram::NGramAutomaton<ngram::BackoffValue>::Construct construct) : queue_(queue_size, construct) {}
        void Add(const WordIndex* const r_context_begin, const WordIndex* const r_context_end, const WordIndex* const new_words, std::size_t new_words_count) {
            auto context_begin = r_context_begin;
            auto new_word = new_words;
            ngram::NGramAutomaton<ngram::BackoffValue>* pred = nullptr;
            for (std::size_t i = 0; i < new_words_count; i++) {
                Task task_{pred, context_begin, r_context_end, *new_word};
                pred = queue_.Add(task_);
                --new_word;
                --context_begin;
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

int main(int argc, char* argv[]){
    int pipeline_size = std::stoi(std::string(argv[1]));
    lm::ngram::Config config;
    config.arpa_complain = lm::ngram::Config::NONE;
    config.messages = nullptr;
    config.probing_multiplier = 2.0;
    lm::ngram::ProbingModel model("test.arpa", config);
    std::cout << "ORDER: " << (int)model.Order() << std::endl;
    std::cout << "PIPELINE SIZE: " << pipeline_size << std::endl;
    //typename lm::ngram::detail::HashedSearch<lm::ngram::BackoffValue> hs;
    lm::Pipeline pipeline(pipeline_size, {model.GetSearch(), model.Order()});
    const char *words[] = {"<s>", "looking", "on", "a", "little", "the", "biarritz", "not_found", "more", ".", "</s>"};
    const size_t num_words = sizeof(words) / sizeof(const char*);
    // Silience "array subscript is above array bounds" when extracting end pointer.
    lm::WordIndex indices[num_words + 1];
    for (unsigned int i = 0; i < num_words; ++i) {
        indices[num_words - 1 - i] = model.GetVocabulary().Index(words[i]);
    }
    pipeline.Add(indices + num_words - 1, indices + num_words, indices + num_words - 2, num_words-1);
    pipeline.Drain();
}
#endif //LM_AUTOMATON_H
