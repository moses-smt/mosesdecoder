#pragma once

#include "mblas/matrix.h"
#include "common/model.h"
 
class Encoder {
  private:
    template <class Weights>
    class Embeddings {
      public:
        Embeddings(const Weights& model)
        : w_(model)
        {}
          
        void Lookup(mblas::Matrix& Row, size_t i) {
          using namespace mblas;
          CopyRow(Row, w_.E_, i);
          Element(_1 + _2,
                  Row, w_.EB_);
        }
      
      private:
        const Weights& w_;
    };
    
    template <class Weights>
    class RNN {
      public:
        RNN(const Weights& model)
        : w_(model) {}
        
        void InitializeState(size_t batchSize = 1) {
          State_.Clear();
          State_.Resize(batchSize, 1000, 0.0);
        }
        
        void GetNextState(mblas::Matrix& State,
                          const mblas::Matrix& Embd,
                          const mblas::Matrix& PrevState) {
          using namespace mblas;
    
          Prod(Za_, Embd, w_.Wz_);
          Prod(Temp_, PrevState, w_.Uz_);
          Element(Logit(_1 + _2), Za_, Temp_);
            
          Prod(Ra_, Embd, w_.Wr_);
          Prod(Temp_, PrevState, w_.Ur_);
          Element(Logit(_1 + _2), Ra_, Temp_);    
        
          Prod(Ha_, Embd, w_.W_);
          Prod(Temp_, Element(_1 * _2, Ra_, PrevState), w_.U_);
          Element(_1 + _2, Ha_, w_.B_); // Broadcasting row-wise
          Element(Tanh(_1 + _2), Ha_, Temp_);
          
          Element((1.0 - _1) * _2 + _1 * _3, Za_, PrevState, Ha_);
          
          Swap(State, Za_);
        }
        
        template <class It>
        void GetContext(It it, It end, 
                        mblas::Matrix& Context, bool invert) {
          InitializeState();
          
          size_t n = std::distance(it, end);
          size_t i = 0;
          while(it != end) {
            GetNextState(State_, *it++, State_);
            if(invert)
              mblas::PasteRow(Context, State_, n - i - 1, 1000);
            else
              mblas::PasteRow(Context, State_, i, 0);
            ++i;
          }
        }
        
      private:
        // Model matrices
        const Weights& w_;
        
        // reused to avoid allocation
        mblas::Matrix Za_;
        mblas::Matrix Ra_;
        mblas::Matrix Ha_;
        mblas::Matrix Temp_;
        mblas::Matrix State_;
    };
    
  public:
    Encoder(const Weights& model)
    : embeddings_(model.encEmbeddings_),
      forwardRnn_(model.encForwardRnn_),
      backwardRnn_(model.encBackwardRnn_)
    {}
    
    void GetContext(const std::vector<size_t>& words,
                    mblas::Matrix& Context) {
      std::vector<mblas::Matrix> embeddedWords;
      
      Context.Resize(words.size(), 2000);
      for(auto& w : words) {
        embeddedWords.emplace_back();
        embeddings_.Lookup(embeddedWords.back(), w);
      }
      
      forwardRnn_.GetContext(embeddedWords.begin(),
                             embeddedWords.end(),
                             Context, false);
      backwardRnn_.GetContext(embeddedWords.rbegin(),
                              embeddedWords.rend(),
                              Context, true);
    }
    
  private:
    Embeddings<Weights::EncEmbeddings> embeddings_;
    RNN<Weights::EncForwardRnn> forwardRnn_;
    RNN<Weights::EncBackwardRnn> backwardRnn_;
};
