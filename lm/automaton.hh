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

namespace ngram {

template <typename Value, typename Callback> class NGramAutomaton {
    public:
        struct NGramTask {
            NGramAutomaton<Value, Callback>* const pred;
            const WordIndex new_word;
            const State* const context_state; // unused if pred != NULL
            Callback& callback;
        };

        struct NGramConstruct {
            detail::HashedSearch<Value>& search;
            //Callback callback;
            NGramConstruct(detail::HashedSearch<Value>& s) : search(s) {};
        };

        typedef NGramTask Task;
        typedef NGramConstruct Construct;

        explicit NGramAutomaton(Construct construct) :
            callback_(),
            ngram_order_(0),
            running_(false),
            search_(construct.search),
            node_(0),
            pred_finished_(false), 
            succ_finished_(false),
            pred_(NULL),
            succ_(NULL),
            succ_data_(),
            new_word_(),
            ret_(),
            in_state_(),
            next_action_(NONE),
            MAX_ORDER(search_.Order()){ 
              UTIL_THROW_IF2(MAX_ORDER < 3, "Smallest order supported is 3, you gave me: " << MAX_ORDER);
            }

        bool Step() {
            //TODO would keeping a function pointer be faster?
            switch(next_action_) {
                case NONE:
                    return false;
                case GET_MIDDLE_PREFETCH_NEXT:
                    GetMiddlePrefetchNext();
                    break;
                case GET_UNIGRAM_PREFETCH_NEXT:
                    GetUnigramPrefetchNext();
                    break;
                case GET_LONGEST:
                    GetLongest();
            }
            ngram_order_ += 1;
            return running_;

        }

        void SetTask(const Task& task) {
            //InitialPredecessorCheck must be called first because pred_==this might be true
            InitialPredecessorCheck(task);

            callback_ = task.callback;
            new_word_ = task.new_word;
            node_ = 0;
            out_state_.length = 0; 
            ret_ = FullScoreReturn();
            succ_ = NULL;
            succ_finished_ = false;
            running_ = true;

            //prefetch unigram
            search_.PrefetchUnigram(new_word_);
            next_action_ = GET_UNIGRAM_PREFETCH_NEXT;
            ngram_order_ = 1;
        }

        bool Finished() {
            return running_ == false;
        }

    private:
        struct SuccessorData {
          FullScoreReturn ret;
          State out_state;
          Callback callback;
        };

        enum NextAction {NONE, GET_UNIGRAM_PREFETCH_NEXT, GET_MIDDLE_PREFETCH_NEXT, GET_LONGEST};
        void InitialPredecessorCheck(const Task& task) {
            assert(task.pred == NULL ^ task.context_state == NULL); //either predecessor is set or context_state
            pred_ = task.pred;
            if (pred_) {
                CopyContextWordsFromPredecessor();
                in_state_.length = pred_->out_state_.length;
                pred_finished_ = pred_->Finished();

                //TODO: This IF statement does not need to be here although if removed then backoffs are copied uselessly
                //The predecessor might be finished since to add to the queue some automaton must finish
                if (pred_finished_) {
                    //copy backoffs
                    std::copy(pred_->out_state_.backoff, pred_->out_state_.backoff + pred_->out_state_.length, in_state_.backoff);
                }
                else {
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
            unsigned short length = MAX_ORDER - 2;

            WordIndex* from = pred_->in_state_.words + length - 1;
            WordIndex* to = in_state_.words + length;
            for(; from >= pred_->in_state_.words; --from, --to){*to = *from;}
            in_state_.words[0] = pred_->new_word_;
        }

        void CheckSuccessorFinished(){
            if (succ_finished_) {
                assert(succ_!=NULL);
                // apply backoffs to fullscorereturn and call callback
                for(int i = succ_data_.ret.ngram_length - 1; i < out_state_.length; i++){
                    succ_data_.ret.prob += out_state_.backoff[i];
                }
                succ_data_.callback(succ_data_.ret, succ_data_.out_state);
            }
            else if (succ_) {
                // transfer backoffs to successor so he can apply them himself
                NotifySuccessorOfCompletion();
            }
        }

        void CheckPredecessorFinished(){
            if (pred_finished_) {
                // apply backoffs from predecessor and call callback
                for(int i = ret_.ngram_length - 1; i < in_state_.length; i++){
                    ret_.prob += in_state_.backoff[i];
                }
                callback_(ret_, out_state_);
            }
            else {
                // Give callback and FullScoreReturn to predecessor
                NotifyPredecessorOfCompletion();
            }
        }

        void NotifyPredecessorOfCompletion() {
            pred_->succ_finished_ = true;
            pred_->succ_data_.ret = ret_;
            pred_->succ_data_.out_state = out_state_;
            pred_->succ_data_.callback = callback_;
        }

        void NotifySuccessorOfCompletion() {
            succ_->pred_finished_ = true;
            std::copy(out_state_.backoff, out_state_.backoff + out_state_.length, succ_->in_state_.backoff);
            succ_->in_state_.length = out_state_.length;
        }


        void SetSuccessor(NGramAutomaton<Value, Callback>* succ){
            succ_ = succ;
        }


        void GetUnigramPrefetchNext(){
            typename detail::HashedSearch<Value>::UnigramPointer uni(search_.LookupUnigram(new_word_, node_, ret_.independent_left, ret_.extend_left));
            out_state_.backoff[0] = uni.Backoff(); //TODO: What if the word is not found?
            ret_.prob = uni.Prob();
            ret_.rest = uni.Rest();
            ret_.ngram_length = 1;

            if (HasExtension(uni.Backoff())) {
                    out_state_.length = 1;
                    //at this point successor can't be finished
                    if (succ_ != NULL) {
                        succ_->in_state_.length = 1;
                    }
            }
            if (in_state_.length == 0 || ret_.independent_left) {
                Finish();
            }
            else {
                //bigrams are not supported
                search_.PrefetchMiddle(0, in_state_.words[0], node_);
                next_action_ = GET_MIDDLE_PREFETCH_NEXT;
            }
        }

        void GetMiddlePrefetchNext(){
            typename detail::HashedSearch<Value>::MiddlePointer pointer(search_.LookupMiddleFromNode(ngram_order_ - 2, node_, ret_.independent_left, ret_.extend_left));
            if (!pointer.Found()) {
                Finish();
                return;
            }
            out_state_.backoff[ngram_order_-1] = pointer.Backoff();
            ret_.prob = pointer.Prob();
            ret_.rest = pointer.Rest();
            ret_.ngram_length = ngram_order_;

            if(HasExtension(pointer.Backoff())) {
                out_state_.length = ngram_order_;
                if (succ_ && !succ_finished_) {
                    succ_->in_state_.length = out_state_.length;
                }
            }
            if (ngram_order_ - 1 == in_state_.length || ret_.independent_left) {
                Finish();
                return;
            }

            if (ngram_order_ + 1 == MAX_ORDER){
                search_.PrefetchLongest(in_state_.words[ngram_order_ - 1], node_);
                next_action_ = GET_LONGEST;
            }
            else {
                search_.PrefetchMiddle(ngram_order_ - 1, in_state_.words[ngram_order_ - 1], node_);
                //next_action_ is already GET_MIDDLE_PREFETCH_NEXT
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
        void CopyWordsToOutState(){
          if (out_state_.length > 0) {
            out_state_.words[0] = new_word_;
            std::copy(in_state_.words, in_state_.words + out_state_.length - 1, out_state_.words + 1);
          }
        }
        
        void Finish(){
            CopyWordsToOutState();
            CheckPredecessorFinished();
            CheckSuccessorFinished();
            running_ = false;
            next_action_ = NONE;
        }


        //Callback to be called once the score is computed
        Callback callback_;
        //ngram_order_ is equal to the ordger of the ngram that will be looked-up at the next invocation of Step()
        //ngram_order_ is initialized to 0 because first invocation of Step() only prefetches a unigram and doesn't do any lookups
        std::size_t ngram_order_;
        bool running_;
        //search_ performs all the prefetches and lookups of ngrams
        detail::HashedSearch<Value> &search_;
        //node_ stores the intermediate hash so that it does not have to be recomputed at every invocation of Step()
        typename detail::HashedSearch<Value>::Node node_;
        //Indicates whether predecessor is finished
        bool pred_finished_;
        //Indicates whether successor is finished
        bool succ_finished_;
        //Pointer to predecessor, can be NULL only if context_state was provided in Task 
        NGramAutomaton<Value, Callback>* pred_;
        //Pointer to successor
        NGramAutomaton<Value, Callback>* succ_;
        //successor stores its data in its predecessor's succ_data_ so that once predecessor completes it can add backoffs
        SuccessorData succ_data_; 
        WordIndex new_word_;
        FullScoreReturn ret_;
        State in_state_;
        //out_state_ stores the backoffs for its successor
        //out_state_.length starts as an upper bound for the context_length but is correct at latest when the automaton finishes
        State out_state_;
        NextAction next_action_;
        //MAX_ORDER is the order of the language model being used by search_
        const unsigned short MAX_ORDER;
};

} // namespace ngram

template <class Automaton> class Queue {
    typedef typename Automaton::Task Task;
    typedef typename Automaton::Construct Construct;
    public:
        explicit Queue(std::size_t size, Construct construct) : size_(size), automata_(size, Automaton(construct)), curr_(automata_.begin()){}

        Automaton* Add(const Task task) {
            // Don't run Step on automaton that is finished
            while (curr_->Step()) {
                Next();
            }
            curr_->SetTask(task);
            Automaton& ret = *curr_;
            Next();
            return &ret;
        }

        void Next() {
            if (++curr_ == automata_.end()) curr_ = automata_.begin();
        }

        // The order in which Step is invoked on automata must be the same as normally
        // i.e. a1->Step(), a2->Step(), ... and NOT a1->Step(), a1->Step(), ..., a1->Step(), a2->Step(),...
        void Drain() {
            bool all_finished = false;
            while(!all_finished){
                all_finished = true;
                for (std::size_t i = 0; i < size_; i++) {
                    all_finished &= !curr_->Step();
                    Next();
                }
            }
        }

    private:
        std::size_t size_;
        std::vector<Automaton> automata_;
        typename std::vector<Automaton>::iterator curr_;
};


template<typename Callback>
class Pipeline {
    public:
        Pipeline(std::size_t queue_size, typename ngram::NGramAutomaton<ngram::BackoffValue, Callback>::Construct construct) : queue_(queue_size, construct) {}
        void BeginScore(const lm::ngram::State& context_state, const WordIndex word, Callback& callback) {
            typename AutomatonT::Task task = {NULL, word, &context_state, callback};
            pred_ = queue_.Add(task);
        }

        void AppendWord(const WordIndex word, Callback& callback){
            assert(pred_);
            typename AutomatonT::Task task = {pred_, word, NULL, callback};
            pred_ = queue_.Add(task);
        }

        void Drain() {
            queue_.Drain();
        }

    private:
        typedef typename ngram::NGramAutomaton<ngram::BackoffValue, Callback> AutomatonT;
        Queue<ngram::NGramAutomaton<ngram::BackoffValue, Callback> > queue_;
        ngram::NGramAutomaton<ngram::BackoffValue, Callback>* pred_;

};

} // namespace lm
#endif //LM_AUTOMATON_H
