#pragma once

#include "mblas/matrix.h"
#include "model.h"
 
class Decoder {
  private:
    template <class Weights>
    class Embeddings {
      public:
        Embeddings(const Weights& model)
        : w_(model)
        {}
            
        void Lookup(mblas::Matrix& Rows, const std::vector<size_t>& ids) {
          using namespace mblas;
          Assemble(Rows, w_.E_, ids);
          Element(_1 + _2, Rows, w_.EB_);
        }
      
      private:
        const Weights& w_;
    };
    
    template <class Weights>
    class RNN {
      public:
        RNN(const Weights& model)
        : w_(model) {}          
        
        void InitializeState(mblas::Matrix& State,
                             const mblas::Matrix& SourceContext,
                             const size_t batchSize = 1) {
          using namespace mblas;
          CopyRow(Temp1_, SourceContext, 0, 1000);
          Temp2_.Clear();
          Temp2_.Resize(batchSize, 1000, 0.0);
          Element(_1 + _2, Temp2_, Temp1_);
          Prod(State, Temp2_, w_.Ws_);
          Broadcast(Tanh(_1 + _2), State, w_.WsB_); // Broadcasting?
        }
        
        mblas::Matrix& GetNextState(mblas::Matrix& State,
                                    const mblas::Matrix& Embd,
                                    const mblas::Matrix& PrevState,
                                    const mblas::Matrix& Context) {
          using namespace mblas;
    
          Prod(Z_, Embd, w_.Wz_);
          Prod(Temp1_, PrevState, w_.Uz_);
          Prod(Temp2_, Context, w_.Cz_);
          Element(Logit(_1 + _2 + _3),
                  Z_, Temp1_, Temp2_);
          
          Prod(R_, Embd, w_.Wr_);
          Prod(Temp1_, PrevState, w_.Ur_);
          Prod(Temp2_, Context, w_.Cr_);
          Element(Logit(_1 + _2 + _3),
                  R_, Temp1_, Temp2_);
          
          Prod(S_, Embd, w_.W_);
          Broadcast(_1 + _2, S_, w_.B_); // Broadcasting row-wise
          Prod(Temp1_, Element(_1 * _2, R_, PrevState), w_.U_);
          Prod(Temp2_, Context, w_.C_);
          Element(Tanh(_1 + _2 + _3), S_, Temp1_, Temp2_);
          
          Element((1.0 - _1) * _2 + _1 * _3,
                  Z_, PrevState, S_);
          
          State.Resize(Z_.Rows(), Z_.Cols());
          Swap(State, Z_);

          return State;
        }
        
      private:
        // Model matrices
        const Weights& w_;
        
        // reused to avoid allocation
        mblas::Matrix Z_;
        mblas::Matrix R_;
        mblas::Matrix S_;
        
        mblas::Matrix Temp1_;
        mblas::Matrix Temp2_;
    };
    
    template <class Weights>
    class Alignment {
      public:
        Alignment(const Weights& model)
        : w_(model)
        {}
          
        void GetContext(mblas::Matrix& Context,
                        const mblas::Matrix& SourceContext,
                        const mblas::Matrix& PrevState) {
          using namespace mblas;  
          
          Prod(Temp1_, SourceContext, w_.Ua_);
          Prod(Temp2_, PrevState, w_.Wa_);
          
          Broadcast(Tanh(_1 + _2), Temp1_, Temp2_);
          
          Prod(A_, w_.Va_, Temp1_, false, true);
          size_t rows1 = SourceContext.Rows();
          size_t rows2 = PrevState.Rows();     
          A_.Reshape(rows2, rows1); // due to broadcasting above
          
          mblas::Softmax(A_);
          Prod(Context, A_, SourceContext);
        }
      
      private:
        const Weights& w_;
        
        mblas::Matrix Temp1_;
        mblas::Matrix Temp2_;
        mblas::Matrix A_;
        
        mblas::Matrix Ones_;
        mblas::Matrix Sums_;
    };
    
    template <class Weights>
    class Softmax {
      public:
        Softmax(const Weights& model)
        : w_(model), filtered_(false)
        {}
          
        void GetProbs(mblas::Matrix& Probs,
                      const mblas::Matrix& PrevState,
                      const mblas::Matrix& PrevEmbd,
                      const mblas::Matrix& Context) {
          
          using namespace mblas;
          
          Prod(T_, PrevState, w_.Uo_);
          Prod(Temp1_, PrevEmbd, w_.Vo_);
          Prod(Temp2_, Context, w_.Co_);
          Element(_1 + _2 + _3, T_, Temp1_, Temp2_);
          Broadcast(_1 + _2, T_, w_.UoB_); // Broadcasting row-wise
          PairwiseReduce(Max(_1, _2), T_);
            
          if(filtered_) { // use only filtered vocabulary for SoftMax
            Prod(Probs, T_, FilteredWo_);
            Broadcast(_1 + _2, Probs, FilteredWoB_); // Broadcasting row-wise
          }
          else {
            Prod(Probs, T_, w_.Wo_);
            Broadcast(_1 + _2, Probs, w_.WoB_); // Broadcasting row-wise
          }
          mblas::Softmax(Probs);
        }
        
        void Filter(const std::vector<size_t>& ids) {
          using namespace mblas;
          
          Matrix TempWo;
          Transpose(TempWo, w_.Wo_);
          Assemble(FilteredWo_, TempWo, ids);
          Transpose(FilteredWo_);
          
          Matrix TempWoB;
          Transpose(TempWoB, w_.WoB_);
          Assemble(FilteredWoB_, TempWoB, ids);
          Transpose(FilteredWoB_);
          
          filtered_ = true;
        }
       
      private:        
        const Weights& w_;
        
        bool filtered_;
        mblas::Matrix FilteredWo_;
        mblas::Matrix FilteredWoB_;
        
        mblas::Matrix T_;
        mblas::Matrix Temp1_;
        mblas::Matrix Temp2_;
        
        mblas::Matrix Ones_;
        mblas::Matrix Sums_;
    };
    
  public:
    Decoder(const Weights& model)
    : embeddings_(model.decEmbeddings_),
      rnn_(model.decRnn_), alignment_(model.decAlignment_),
      softmax_(model.decSoftmax_)
    {}
    
    void Filter(const std::vector<size_t>& ids) {
      softmax_.Filter(ids);
    }
    
    void GetProbs(mblas::Matrix& Probs,
                  mblas::Matrix& AlignedSourceContext,
                  const mblas::Matrix& PrevState,
                  const mblas::Matrix& PrevEmbedding,
                  const mblas::Matrix& SourceContext) {
      alignment_.GetContext(AlignedSourceContext, SourceContext, PrevState);
      softmax_.GetProbs(Probs, PrevState, PrevEmbedding, AlignedSourceContext);
    }
    
    void EmptyState(mblas::Matrix& State, const mblas::Matrix& SourceContext,
                    size_t batchSize = 1) {
      State.Resize(batchSize, 1000);
      rnn_.InitializeState(State, SourceContext, batchSize);
    }
    
    void EmptyEmbedding(mblas::Matrix& Embedding, size_t batchSize = 1) {
      Embedding.Clear();
      Embedding.Resize(batchSize, 620, 0);
    }
    
    void Lookup(mblas::Matrix& Embedding, const std::vector<size_t>& w) {
      embeddings_.Lookup(Embedding, w);
    }
    
    void GetNextState(mblas::Matrix& State,
                      const mblas::Matrix& Embedding,
                      const mblas::Matrix& PrevState,
                      const mblas::Matrix& AlignedSourceContext) {
      rnn_.GetNextState(State, Embedding, PrevState, AlignedSourceContext);  
    }
    
  private:
    Embeddings<Weights::DecEmbeddings> embeddings_;
    RNN<Weights::DecRnn> rnn_;
    Alignment<Weights::DecAlignment> alignment_;
    Softmax<Weights::DecSoftmax> softmax_;
};
