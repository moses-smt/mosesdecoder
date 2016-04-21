#pragma once

#include "mblas/matrix.h"
#include "dl4mt/model.h"
#include "dl4mt/gru.h"
 
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
        }
        
        size_t GetDim() {
          return w_.E_.Cols();    
        }
        
      private:
        const Weights& w_;
    };
    
    template <class Weights1, class Weights2>
    class RNNHidden {
      public:
        RNNHidden(const Weights1& initModel, const Weights2& gruModel)
        : w_(initModel), gru_(gruModel) {}          
        
        void InitializeState(mblas::Matrix& State,
                             const mblas::Matrix& SourceContext,
                             const size_t batchSize = 1) {
          using namespace mblas;
          
          Mean(Temp1_, SourceContext);
          Temp2_.Clear();
          Temp2_.Resize(batchSize, SourceContext.Cols(), 0.0);
          BroadcastVec(_1 + _2, Temp2_, Temp1_);
          Prod(State, Temp2_, w_.Wi_);
          BroadcastVec(Tanh(_1 + _2), State, w_.Bi_);
        }
        
        void GetNextState(mblas::Matrix& NextState,
                          const mblas::Matrix& State,
                          const mblas::Matrix& Context) {
          gru_.GetNextState(NextState, State, Context);
        }
        
      private:
        const Weights1& w_;
        const GRU<Weights2> gru_;
        
        mblas::Matrix Temp1_;
        mblas::Matrix Temp2_;
    };
    
    template <class Weights>
    class RNNFinal {
      public:
        RNNFinal(const Weights& model)
        : gru_(model) {}          
        
        void GetNextState(mblas::Matrix& NextState,
                          const mblas::Matrix& State,
                          const mblas::Matrix& Context) {
          gru_.GetNextState(NextState, State, Context);
        }
        
      private:
        const GRU<Weights> gru_;
    };
        
    template <class Weights>
    class Alignment {
      public:
        Alignment(const Weights& model)
        : w_(model)
        {
        }
          
        void GetAlignedSourceContext(mblas::Matrix& AlignedSourceContext,
                                     const mblas::Matrix& HiddenState,
                                     const mblas::Matrix& SourceContext) {
          using namespace mblas;  
          
          Prod(Temp1_, SourceContext, w_.U_);
          Prod(Temp2_, HiddenState, w_.W_);
          BroadcastVec(_1 + _2, Temp2_, w_.B_);
          
          cudaDeviceSynchronize();
          
          Broadcast(Tanh(_1 + _2), Temp1_, Temp2_);
          
          Prod(A_, w_.V_, Temp1_, false, true);
          
          size_t rows1 = SourceContext.Rows();
          size_t rows2 = HiddenState.Rows();     
          A_.Reshape(rows2, rows1); // due to broadcasting above
          Element(_1 + w_.C_(0,0), A_);
          
          mblas::Softmax(A_);
          Prod(AlignedSourceContext, A_, SourceContext);
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
        {
        }
          
        void GetProbs(mblas::Matrix& Probs,
                  const mblas::Matrix& State,
                  const mblas::Matrix& Embedding,
                  const mblas::Matrix& AlignedSourceContext) {
          using namespace mblas;
          
          Prod(T1_, State, w_.W1_);
          Prod(T2_, Embedding, w_.W2_);
          Prod(T3_, AlignedSourceContext, w_.W3_);
          
          BroadcastVec(_1 + _2, T1_, w_.B1_);
          BroadcastVec(_1 + _2, T2_, w_.B2_);
          BroadcastVec(_1 + _2, T3_, w_.B3_);
      
          cudaDeviceSynchronize();
      
          Element(Tanh(_1 + _2 + _3), T1_, T2_, T3_);
          
          Prod(Probs, T1_, w_.W4_);
          BroadcastVec(_1 + _2, Probs, w_.B4_);
          mblas::Softmax(Probs);
        }
    
        void Filter(const std::vector<size_t>& ids) {
        }
       
      private:        
        const Weights& w_;
        
        bool filtered_;
        mblas::Matrix FilteredWo_;
        mblas::Matrix FilteredWoB_;
        
        mblas::Matrix T1_;
        mblas::Matrix T2_;
        mblas::Matrix T3_;
    };
    
  public:
    Decoder(const Weights& model)
    : embeddings_(model.decEmbeddings_),
      rnn1_(model.decInit_, model.decGru1_),
      rnn2_(model.decGru2_),
      alignment_(model.decAlignment_),
      softmax_(model.decSoftmax_)
    {}
    
    void MakeStep(mblas::Matrix& NextState,
                  mblas::Matrix& Probs,
                  const mblas::Matrix& State,
                  const mblas::Matrix& Embeddings,
                  const mblas::Matrix& SourceContext) {
      GetHiddenState(HiddenState_, State, Embeddings);
      GetAlignedSourceContext(AlignedSourceContext_, HiddenState_, SourceContext);
      GetNextState(NextState, HiddenState_, AlignedSourceContext_);
      GetProbs(Probs, NextState, Embeddings, AlignedSourceContext_);
    }
    
    void EmptyState(mblas::Matrix& State,
                    const mblas::Matrix& SourceContext,
                    size_t batchSize = 1) {
      rnn1_.InitializeState(State, SourceContext, batchSize);
    }
    
    void EmptyEmbedding(mblas::Matrix& Embedding,
                        size_t batchSize = 1) {
      Embedding.Clear();
      Embedding.Resize(batchSize, embeddings_.GetDim(), 0);
    }
    
    void Lookup(mblas::Matrix& Embedding,
                const std::vector<size_t>& w) {
      embeddings_.Lookup(Embedding, w);
    }
    
    void Filter(const std::vector<size_t>& ids) {
    
    }
      
    
    //private:
    
    void GetHiddenState(mblas::Matrix& HiddenState,
                        const mblas::Matrix& PrevState,
                        const mblas::Matrix& Embedding) {
      rnn1_.GetNextState(HiddenState, PrevState, Embedding);
    }
    
    void GetAlignedSourceContext(mblas::Matrix& AlignedSourceContext,
                                 const mblas::Matrix& HiddenState,
                                 const mblas::Matrix& SourceContext) {
      alignment_.GetAlignedSourceContext(AlignedSourceContext, HiddenState, SourceContext);
    }
    
    void GetNextState(mblas::Matrix& State,
                      const mblas::Matrix& HiddenState,
                      const mblas::Matrix& AlignedSourceContext) {
      rnn2_.GetNextState(State, HiddenState, AlignedSourceContext);
    }
    
    
    void GetProbs(mblas::Matrix& Probs,
                  const mblas::Matrix& State,
                  const mblas::Matrix& Embedding,
                  const mblas::Matrix& AlignedSourceContext) {
      softmax_.GetProbs(Probs, State, Embedding, AlignedSourceContext);
    }
    
    
  private:
    mblas::Matrix HiddenState_;
    mblas::Matrix AlignedSourceContext_;  
    
    Embeddings<Weights::DecEmbeddings> embeddings_;
    RNNHidden<Weights::DecInit, Weights::DecGRU1> rnn1_;
    RNNFinal<Weights::DecGRU2> rnn2_;
    Alignment<Weights::DecAlignment> alignment_;
    Softmax<Weights::DecSoftmax> softmax_;
};
