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
#include "util/exception.hh"

#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <array>
#include <assert.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iterator>

namespace lm {

enum class Status {Done, Working};

namespace {

class SimpleAutomaton {
    public:
        using Task = std::pair<std::string, unsigned int>;
        using Construct = int;

        SimpleAutomaton(int x = 0) : repeat_(0) {}

        Status Step() {
            if (repeat_ <= 0) return Status::Done;
            std::cout << word_ << std::endl;
            --repeat_;
            return Status::Working;
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
            NGramAutomaton<Value>* const pred;
            const WordIndex new_word;
            const State* const context_state; // unused if pred != nullptr
            const std::function<void(FullScoreReturn)> callback;
        };

        struct NGramConstruct {
            detail::HashedSearch<Value>& search;
            unsigned char max_order;
        };


        using Task = NGramTask;
        using Construct = NGramConstruct;
        using Status = lm::Status;

        explicit NGramAutomaton(Construct construct) :
            ngram_order_(0),
            status_(Status::Done),
            search_(construct.search),
            max_order_(construct.max_order),
            node_(0),
            pred_finished_(false), 
            succ_finished_(false),
            pred_(nullptr),
            succ_(nullptr),
            in_state_(),
            succ_data_(),
            new_word_(),
            ret_(){}

        Status Step() {
            if (status_ == Status::Done || ngram_order_ > max_order_){
                return Status::Done;
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
            return status_;

        }

        void SetTask(const Task& task) {
            std::cout << this << " start" << std::endl;

            new_word_ = task.new_word;
            pred_ = task.pred;
            callback_ = task.callback;

            InitialPredecessorCheck(task);

            ngram_order_ = 0;
            node_ = 0;
            out_state_.length = std::min(in_state_.length + 1, max_order_ - 1);
            ret_ = FullScoreReturn();
            succ_ = nullptr;
            succ_finished_ = false;
            status_ = Status::Working;
        }

    private:
        struct SuccessorData {
            FullScoreReturn ret;
            std::function<void(FullScoreReturn&)> callback;
        };

        void InitialPredecessorCheck(const Task& task) {
            assert(task.pred == nullptr ^ task.context_state == nullptr); //either predecessor is set or context_state
            pred_ = task.pred;
            pred_finished_ = false;
            if (pred_) {
                if (pred_->Finished() || this == pred_) {
                    // the second condition (this == pred_) does not need to be there
                    // if we assume this->status_ == Status::Done when this function is called
                    pred_finished_ = true;
                    CopyStateFromPredecessor();
                }
                else {
                    NotifyPredecessor();
                }
            }
            else {
                pred_finished_ = true;
                std::copy(task.context_state->words, task.context_state->words + task.context_state->length, in_state_.words);
                std::copy(task.context_state->backoff, task.context_state->backoff + task.context_state->length, in_state_.backoff);
                in_state_.length = task.context_state->length;
            }
        }

        void CopyStateFromPredecessor() {
            CopyContextWordsFromPredecessor();
            std::copy(pred_->out_state_.backoff, pred_->out_state_.backoff + pred_->out_state_.length, in_state_.backoff);
            in_state_.length = pred_->out_state_.length;
        }

        void NotifyPredecessor(){
            pred_->SetSuccessor(this);
            //predecessor copies context words and state length to his successor(=this)
            CopyContextWordsFromPredecessor();
            in_state_.length = pred_->out_state_.length;
        }
        
        void CopyContextWordsFromPredecessor(){
            in_state_.words[0] = pred_->new_word_;
            //std::cout<<"pred new_word_: " << pred_->new_word_;
            auto length = std::min(pred_->out_state_.length, static_cast<unsigned char>(max_order_ - 2));
            std::copy(pred_->in_state_.words, pred_->in_state_.words + length, in_state_.words + 1);
            std::cout << "Copying words";
            for (auto b : in_state_.words) std::cout << b << " ";
            std::cout << std::endl;
        }

        void CheckSuccessorFinished(){
            if (succ_finished_) {
                assert(succ_!=nullptr);
                // apply backoffs to fullscorereturn and call callback
                for(auto i = succ_data_.ret.ngram_length - 1; i < out_state_.length; i++){
                    succ_data_.ret.prob += out_state_.backoff[i];
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
                // apply backoffs from predecessor and call callback
                for(auto i = ret_.ngram_length - 1; i < in_state_.length; i++){
                    ret_.prob += in_state_.backoff[i];
                }
                std::cout << this << " callback" <<std::endl;
                callback_(ret_);
            }
            else {
                // Give callback and FullScoreReturn to predecessor
                std::cout << this << " callback left for " << pred_ << std::endl;
                NotifyPredecessorOfCompletion();
            }
        }

        void NotifyPredecessorOfCompletion() {
            pred_->succ_finished_ = true;
            pred_->succ_data_.callback = callback_;
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

        void SetSuccessor(NGramAutomaton<Value>* succ){
            succ_ = succ;
        }

        Status GetStatus() {
            return status_;
        }


        void GetUnigramPrefetchNext(){
            typename detail::HashedSearch<Value>::UnigramPointer uni(search_.LookupUnigram(new_word_, node_, ret_.independent_left, ret_.extend_left));
            std::cout << new_word_ << " performed " << 1 << " lookup" << std::endl;
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
                if (max_order_ == 2) {
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
            std::cout << new_word_ << " performed " << ngram_order_ << " lookup" << std::endl;
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
            std::cout << ngram_order_ << " " << (int) in_state_.length << std::endl;

            if (ngram_order_ - 1 == in_state_.length || ret_.independent_left) {

                Finish();
                return;
            }

            if (ngram_order_ + 1 == max_order_){
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
            std::cout << new_word_ << " performed " << ngram_order_ << " lookup" << std::endl;
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
            std::cout << this << " finish" << std::endl;
            std::cout << "new word: " << new_word_ << std::endl;
            std::cout << "in_length: " << (int)in_state_.length << std::endl;
            std::cout << "ind left: " << ret_.independent_left << std::endl;
            std::cout << "out length: " << (int)out_state_.length << std::endl;
            std::cout << "in length: " << (int)in_state_.length << std::endl;



            WriteOutLength(std::min(ret_.ngram_length, static_cast<unsigned char>(max_order_ - 1)));
            CheckPredecessorFinished();
            CheckSuccessorFinished();
            status_ = Status::Done;
        }


        std::function<void(const FullScoreReturn&)> callback_;// = [](FullScoreReturn& r){std::cout << "In callback, prob: " << r.prob << " ngram_length: " << (int)r.ngram_length << std::endl;};
        std::size_t ngram_order_;
        Status status_;
        detail::HashedSearch<Value> &search_;
        unsigned char max_order_;
        typename detail::HashedSearch<Value>::Node node_;
        bool pred_finished_;
        bool succ_finished_;
        NGramAutomaton<Value>* pred_;
        NGramAutomaton<Value>* succ_;
        SuccessorData succ_data_; 
        WordIndex new_word_;
        FullScoreReturn ret_;
        State in_state_;
        State out_state_;
};

} // namespace ngram

template <class Automaton> class Queue {
    using Task = typename Automaton::Task;
    using Construct = typename Automaton::Construct;
    public:
        explicit Queue(std::size_t size, Construct construct) : size_(size), curr_(0), automata_(size, Automaton(construct)) {}

        Automaton* Add(const Task task) {
            while (automata_[curr_].Step() != Status::Done) {
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
                while (automata_[curr_].Step() != Status::Done) {std::cout <<"step" <<std::endl;}
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
        void FullScore(const lm::ngram::State& context_state, const WordIndex word, const std::function<void(const FullScoreReturn&)> callback) {
            std::cout << "appending " << word << std::endl;
            pred_ = queue_.Add({nullptr, word, &context_state, callback});
        }

//      void ScoreWords(const lm::ngram::State& context_state, const WordIndex* const words_begin, const WordIndex* const words_end) {
//          if (words_begin == words_end) return;
//          auto word = words_begin;
//          pred_ = queue_.Add({nullptr, *word, &context_state});
//          ++word;
//          while(word != words_end){ 
//              AddWord(*word);
//              ++word;
//          }
//      }

        void AppendWord(const WordIndex word, const std::function<void(const FullScoreReturn&)>& callback){
            std::cout << "appending " << word << std::endl;
            assert(pred_);
            pred_ = queue_.Add({pred_, word, nullptr, callback});
        }

        void Drain() {
            queue_.Drain();
        }


    private:
        using Task = ngram::NGramAutomaton<ngram::BackoffValue>::Task;

        Queue<ngram::NGramAutomaton<ngram::BackoffValue>> queue_;
        ngram::NGramAutomaton<ngram::BackoffValue>* pred_;

};

} // namespace lm

void CheckEqual(const lm::FullScoreReturn& lhs, const lm::FullScoreReturn& rhs) {
    assert(lhs.prob == rhs.prob);
    assert(lhs.independent_left == rhs.independent_left);
    assert(lhs.ngram_length == rhs.ngram_length);
}

int main(int argc, char* argv[]){
    if (argc < 4) {
        std::cerr << "Usage: <pipeline size> <arpa file> <test file>" << std::endl;
        return 1;
    }
    int pipeline_size = std::stoi(std::string(argv[1]));
    std::string arpa(argv[2]);
    std::string test(argv[3]);

    lm::ngram::Config config;
    config.arpa_complain = lm::ngram::Config::NONE;
    config.messages = nullptr;
    config.positive_log_probability = lm::SILENT;
    config.probing_multiplier = 2.0;
    lm::ngram::ProbingModel model(arpa.data(), config);
    std::cout << "ORDER: " << (int)model.Order() << std::endl;
    std::cout << "PIPELINE SIZE: " << pipeline_size << std::endl;
    lm::Pipeline pipeline(pipeline_size, {model.GetSearch(), model.Order()});
    
    util::FilePiece in(test.data());
    StringPiece word;
    auto model_score = 0.0;
    auto pipe_score = 0.0;

    while (true) {
        lm::ngram::State in_state, out_state;
        if (in.ReadWordSameLine(word)) {
            lm::WordIndex vocab = model.GetVocabulary().Index(word);
            auto ret = model.FullScore(model.BeginSentenceState(), vocab, out_state); 
            model_score += ret.prob;
            auto callback = [=, &pipe_score](const lm::FullScoreReturn& r){CheckEqual(ret, r); pipe_score += r.prob;};
            pipeline.FullScore(model.BeginSentenceState(), vocab, callback);
            in_state = out_state;
        }

        while(in.ReadWordSameLine(word)) {
            lm::WordIndex vocab = model.GetVocabulary().Index(word);
            auto ret = model.FullScore(in_state, vocab, out_state);
            model_score += ret.prob;
            auto callback = [=, &pipe_score](const lm::FullScoreReturn& r){CheckEqual(ret, r); pipe_score += r.prob;};
            pipeline.AppendWord(vocab, callback);
            in_state = out_state;
        }

        try {
            UTIL_THROW_IF('\n' != in.get(), util::Exception, "FilePiece is confused.");
        } catch (const util::EndOfFileException &e) { break; }

        
        auto ret = model.FullScore(in_state, model.GetVocabulary().EndSentence(), out_state);
        model_score += ret.prob;
        auto callback = [=, &pipe_score](const lm::FullScoreReturn& r){CheckEqual(ret, r); pipe_score += r.prob;};
        pipeline.AppendWord(model.GetVocabulary().EndSentence(), callback);
    }
    pipeline.Drain();
    std::cout << model_score << " " << pipe_score << std::endl;
}
#endif //LM_AUTOMATON_H
