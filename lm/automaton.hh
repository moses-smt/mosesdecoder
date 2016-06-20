#ifndef LM_AUTOMATON_H
#define LM_AUTOMATON_H


#include "lm/word_index.hh"
#include "lm/state.hh"
#include "lm/return.hh"
#include "lm/search_hashed.hh"
#include "lm/value.hh"
#include "lm/blank.hh"
#include "lm/config.hh"
#include "lm/model.hh"
#include "util/exception.hh"

#include <vector>
#include <assert.h>

namespace lm {

enum class Status {Done, Working};

namespace ngram {

template <typename Value, typename Callback> class NGramAutomaton {
    public:
        struct NGramTask {
            NGramAutomaton<Value, Callback>* const pred;
            const WordIndex new_word;
            const State* const context_state; // unused if pred != nullptr
        };

        struct NGramConstruct {
            detail::HashedSearch<Value>& search;
            Callback callback;
            unsigned char max_order;
        };


        using Task = NGramTask;
        using Construct = NGramConstruct;
        using Status = lm::Status;

        explicit NGramAutomaton(Construct construct) :
            callback_(construct.callback),
            ngram_order_(0),
            status_(Status::Done),
            search_(construct.search),
            node_(0),
            pred_finished_(false), 
            succ_finished_(false),
            pred_(nullptr),
            succ_(nullptr),
            in_state_(),
            succ_data_(),
            new_word_(),
            ret_(),
            MAX_ORDER(search_.Order()){}

        Status Step() {
            if (status_ == Status::Done || ngram_order_ > MAX_ORDER){
                return Status::Done;
            }

            switch(ngram_order_) {
                case 0:
                    //prefetch unigram only
                    search_.PrefetchUnigram(new_word_);
                    break;
                case 1:
                    GetUnigramPrefetchNext();
                    break;
                default:
                    if (ngram_order_ == MAX_ORDER) GetLongest();
                    else GetMiddlePrefetchNext();
                    break;
            }
            ngram_order_ += 1;
            return status_;

        }

        void SetTask(const Task& task) {
            //InitialPredecessorCheck must be called first because pred_==this might be true
            InitialPredecessorCheck(task);

            new_word_ = task.new_word;
            ngram_order_ = 0;
            node_ = 0;
            out_state_.length = std::min(in_state_.length + 1, MAX_ORDER - 1);
            ret_ = FullScoreReturn();
            succ_ = nullptr;
            succ_finished_ = false;
            status_ = Status::Working;
        }

    private:
        struct SuccessorData {
            FullScoreReturn ret;
        };

        void InitialPredecessorCheck(const Task& task) {
            assert(task.pred == nullptr ^ task.context_state == nullptr); //either predecessor is set or context_state
            pred_ = task.pred;
            if (pred_) {
                CopyContextWordsFromPredecessor();
                in_state_.length = pred_->out_state_.length;

                if (pred_->Finished()) {
                    pred_finished_ = true;

                    //copy backoffs
                    std::copy(pred_->out_state_.backoff, pred_->out_state_.backoff + pred_->out_state_.length, in_state_.backoff);
                }
                else {
                    pred_finished_ = false;
                    pred_->SetSuccessor(this);
                }
            }
            else {
                //copy from context state
                pred_finished_ = true;
                std::copy(task.context_state->words, task.context_state->words + task.context_state->length, in_state_.words);
                std::copy(task.context_state->backoff, task.context_state->backoff + task.context_state->length, in_state_.backoff);
                in_state_.length = task.context_state->length;
            }
        }

        void CopyContextWordsFromPredecessor(){
            //pred_ might equal this, hence the copying order
            auto length = std::min(pred_->out_state_.length, static_cast<unsigned char>(MAX_ORDER - 2));

            auto from = pred_->in_state_.words + length - 1;
            auto to = in_state_.words + length;
            for(; from >= pred_->in_state_.words; --from, --to){*to = *from;}
            in_state_.words[0] = pred_->new_word_;
        }

        void CheckSuccessorFinished(){
            if (succ_finished_) {
                assert(succ_!=nullptr);
                // apply backoffs to fullscorereturn and call callback
                for(auto i = succ_data_.ret.ngram_length - 1; i < out_state_.length; i++){
                    succ_data_.ret.prob += out_state_.backoff[i];
                }
                succ_->callback_(succ_data_.ret);
            }
            else if (succ_) {
                // transfer backoffs to successor so he can apply them himself
                NotifySuccessorOfCompletion();
            }
        }

        void CheckPredecessorFinished(){
            if (pred_finished_) {
                // apply backoffs from predecessor and call callback
                for(auto i = ret_.ngram_length - 1; i < in_state_.length; i++){
                    ret_.prob += in_state_.backoff[i];
                }
                callback_(ret_);
            }
            else {
                // Give callback and FullScoreReturn to predecessor
                NotifyPredecessorOfCompletion();
            }
        }

        void NotifyPredecessorOfCompletion() {
            pred_->succ_finished_ = true;
            pred_->succ_data_.ret = ret_;
        }

        void NotifySuccessorOfCompletion() {
            succ_->pred_finished_ = true;
            std::copy(out_state_.backoff, out_state_.backoff + out_state_.length, succ_->in_state_.backoff);
            succ_->in_state_.length = out_state_.length;
        }

        bool Finished() {
            return status_ == Status::Done;
        }

        void SetSuccessor(NGramAutomaton<Value, Callback>* succ){
            succ_ = succ;
        }

        Status GetStatus() {
            return status_;
        }


        void GetUnigramPrefetchNext(){
            typename detail::HashedSearch<Value>::UnigramPointer uni(search_.LookupUnigram(new_word_, node_, ret_.independent_left, ret_.extend_left));
            WriteWordToSuccInState(new_word_);
            out_state_.backoff[0] = uni.Backoff(); //TODO: What if the word is not found?
            ret_.prob = uni.Prob();
            ret_.rest = uni.Rest();
            ret_.ngram_length = 1;
            if (!HasExtension(uni.Backoff())) {
                WriteOutLength(0);
            }
            if (in_state_.length == 0 || ret_.independent_left) {
                Finish();
            }
            else {
                if (MAX_ORDER == 2) {
                    //for bigrams we don't prefetch middle since there are none
                    search_.PrefetchLongest(in_state_.words[0], node_);
                }
                else {
                    search_.PrefetchMiddle(0, in_state_.words[0], node_);
                }
            }
        }

        void GetMiddlePrefetchNext(){
            typename detail::HashedSearch<Value>::MiddlePointer pointer(search_.LookupMiddleFromNode(ngram_order_ - 2, node_, ret_.independent_left, ret_.extend_left));
            if (!pointer.Found()) {
                Finish();
                return;
            }
            WriteWordToSuccInState(in_state_.words[ngram_order_-2]);
            out_state_.backoff[ngram_order_-1] = pointer.Backoff();
            ret_.prob = pointer.Prob();
            ret_.rest = pointer.Rest();
            ret_.ngram_length = ngram_order_;

            if (!HasExtension(pointer.Backoff())){
                WriteOutLength(ngram_order_-1); 
            }

            if (ngram_order_ - 1 == in_state_.length || ret_.independent_left) {
                Finish();
                return;
            }

            if (ngram_order_ + 1 == MAX_ORDER){
                search_.PrefetchLongest(in_state_.words[ngram_order_ - 1], node_);
            }
            else {
                search_.PrefetchMiddle(ngram_order_ - 1, in_state_.words[ngram_order_ - 1], node_);
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
        
        void WriteWordToSuccInState(lm::WordIndex word){
            if (!succ_finished_ && succ_) {
                succ_->in_state_.words[ngram_order_-1];
            }
        }

        void WriteOutLength(const unsigned char length){
            if (length < out_state_.length) {
                out_state_.length = length;
                if (succ_ && !succ_finished_) {
                    succ_->in_state_.length = length;
                }
            }
        }

        void Finish(){
            WriteOutLength(std::min(ret_.ngram_length, static_cast<unsigned char>(MAX_ORDER - 1)));
            CheckPredecessorFinished();
            CheckSuccessorFinished();
            status_ = Status::Done;
        }


        Callback callback_;
        std::size_t ngram_order_;
        Status status_;
        detail::HashedSearch<Value> &search_;
        typename detail::HashedSearch<Value>::Node node_;
        bool pred_finished_;
        bool succ_finished_;
        NGramAutomaton<Value, Callback>* pred_;
        NGramAutomaton<Value, Callback>* succ_;
        SuccessorData succ_data_; 
        WordIndex new_word_;
        FullScoreReturn ret_;
        State in_state_;
        State out_state_;
        const unsigned short MAX_ORDER;
};

} // namespace ngram

template <class Automaton> class Queue {
    using Task = typename Automaton::Task;
    using Construct = typename Automaton::Construct;
    public:
        explicit Queue(std::size_t size, Construct construct) : size_(size), automata_(size, Automaton(construct)), curr_(automata_.begin()){}

        Automaton* Add(const Task task) {
            while (curr_->Step() != Status::Done) {
                Next();
            }
            curr_->SetTask(task);
            curr_->Step();
            auto& ret = *curr_;
            Next();
            return &ret;
        }

        void Next() {
            if (++curr_ == automata_.end()) curr_ = automata_.begin();
        }

        void Drain() {
            std::size_t drained = 0;
            while (drained != size_) {
                while (curr_->Step() != Status::Done) {}
                Next();
                ++drained;
            }
        }

    private:
        std::size_t size_;
        std::vector<Automaton> automata_;
        using It = decltype(automata_.begin());
        It curr_;
};


template<typename Callback>
class Pipeline {

    public:
        Pipeline(std::size_t queue_size, typename ngram::NGramAutomaton<ngram::BackoffValue, Callback>::Construct construct) : queue_(queue_size, construct) {}
        void FullScore(const lm::ngram::State& context_state, const WordIndex word) {
            pred_ = queue_.Add({nullptr, word, &context_state});
        }

        void AppendWord(const WordIndex word){
            assert(pred_);
            pred_ = queue_.Add({pred_, word, nullptr});
        }

        void Drain() {
            queue_.Drain();
        }


    private:
        using Task = typename ngram::NGramAutomaton<ngram::BackoffValue, Callback>::Task;

        Queue<ngram::NGramAutomaton<ngram::BackoffValue, Callback>> queue_;
        ngram::NGramAutomaton<ngram::BackoffValue, Callback>* pred_;

};

} // namespace lm
#endif //LM_AUTOMATON_H
