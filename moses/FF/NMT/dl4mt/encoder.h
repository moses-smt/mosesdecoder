#pragma once

#include "mblas/matrix.h"
#include "dl4mt/model.h"
 
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
          State_.Resize(batchSize, 1024, 0.0);
        }
        
        void GetNextState(mblas::Matrix& State,
                          const mblas::Matrix& Embd,
                          const mblas::Matrix& PrevState) {
          using namespace mblas;
    
          const size_t cols = 1024;
    
          // @TODO: Launch streams to performs GEMMs in parallel
          // @TODO: Join matrices and perform single GEMM --------
          Prod(RU_, Embd, w_.W_);
          Prod(H_,  Embd, w_.Wx_);
          // -----------------------------------------------------
          
          // @TODO: Join matrices and perform single GEMM --------
          Prod(Temp1_, PrevState, w_.U_);
          Prod(Temp2_, PrevState, w_.Ux_);        
          // -----------------------------------------------------
          
          // @TODO: Organize into one kernel ---------------------
          Element(_1 + _2, RU_, w_.B_); // Broadcasting row-wise
          Element(Logit(_1 + _2), RU_, Temp1_);
          Slice(R_, RU_, 0, cols);
          Slice(U_, RU_, 1, cols);
          Element(_1 + _2, H_, w_.Bx_); // Broadcasting row-wise
          Element(Tanh(_1 + _2 * _3), H_, R_, Temp2_);
          Element((1.0 - _1) * _2 + _1 * _3, U_, H_, PrevState);
          // -----------------------------------------------------
          
          Swap(State, U_);
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
              mblas::PasteRow(Context, State_, n - i - 1, 1024);
            else
              mblas::PasteRow(Context, State_, i, 0);
            ++i;
          }
        }
        
      private:
        // Model matrices
        const Weights& w_;
        
        // reused to avoid allocation
        mblas::Matrix RU_;
        mblas::Matrix R_;
        mblas::Matrix U_;
        mblas::Matrix H_;
        mblas::Matrix Temp1_;
        mblas::Matrix Temp2_;
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
      
      Context.Resize(words.size(), 2048);
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
