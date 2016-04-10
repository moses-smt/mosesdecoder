#pragma once

#include "mblas/matrix.h"
#include "dl4mt/model.h"
 
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
    
    template <class Weights>
    class RNN {
      public:
        RNN(const Weights& model)
        : w_(model) {}          
        
        void InitializeState(mblas::Matrix& State,
                             const mblas::Matrix& SourceContext,
                             const size_t batchSize = 1) {
          using namespace mblas;
          
          Mean(Temp1_, SourceContext);
          Temp2_.Clear();
          Temp2_.Resize(batchSize, SourceContext.Cols(), 0.0);
          Broadcast(_1 + _2, Temp2_, Temp1_);
          Prod(State, Temp2_, w_.Wi_);
          Broadcast(Tanh(_1 + _2), State, w_.Bi_);
        }
        
        void GetHiddenState(mblas::Matrix& State,
                            const mblas::Matrix& PrevState,
                            const mblas::Matrix& Embd) {
          using namespace mblas;
          
          const size_t cols = PrevState.Cols();
          
          // @TODO: Optimization
          // @TODO: Launch streams to perform GEMMs in parallel
          // @TODO: Join matrices and perform single GEMM --------
          Prod(RU_, Embd, w_.W_);
          Prod(H_,  Embd, w_.Wx_);
          // -----------------------------------------------------
          
          // @TODO: Join matrices and perform single GEMM --------
          Prod(Temp1_, PrevState, w_.U_);
          Prod(Temp2_, PrevState, w_.Ux_);        
          // -----------------------------------------------------
          
          // @TODO: Organize into one kernel ---------------------
          Broadcast(_1 + _2, RU_, w_.B_); // Broadcasting row-wise
          Element(Logit(_1 + _2), RU_, Temp1_);
          Slice(R_, RU_, 0, cols);
          Slice(U_, RU_, 1, cols);
          Broadcast(_1 + _2, H_, w_.Bx_); // Broadcasting row-wise
          Element(Tanh(_1 + _2 * _3), H_, R_, Temp2_);
          Element((1.0 - _1) * _2 + _1 * _3, U_, H_, PrevState);
          // -----------------------------------------------------
          
          Swap(State, U_);
        }
        
        void GetNextState(mblas::Matrix& State,
                          const mblas::Matrix& HiddenState,
                          const mblas::Matrix& AlignedSourceContext) {
          using namespace mblas;
          size_t cols = 1024;
    
          // @TODO: Optimization
          // @TODO: Launch streams to perform GEMMs in parallel
          // @TODO: Join matrices and perform single GEMM --------
          Prod(RU_, AlignedSourceContext, w_.Wp_);
          Prod(H_,  AlignedSourceContext, w_.Wpx_);
          // -----------------------------------------------------
          
          // @TODO: Join matrices and perform single GEMM --------
          Prod(Temp1_, HiddenState, w_.Up_);
          Prod(Temp2_, HiddenState, w_.Upx_);        
          // -----------------------------------------------------
          
          // @TODO: Organize into one kernel ---------------------
          Broadcast(_1 + _2, RU_, w_.Bp_); // Broadcasting row-wise
          Element(Logit(_1 + _2), RU_, Temp1_);
          Slice(R_, RU_, 0, cols);
          Slice(U_, RU_, 1, cols);
          Broadcast(_1 + _2, Temp2_, w_.Bpx_); // Broadcasting row-wise,
                                               // bias in different place!
          Element(Tanh(_1 + _2 * _3), H_, R_, Temp2_);
          Element((1.0 - _1) * _2 + _1 * _3, U_, H_, HiddenState);
          // -----------------------------------------------------
          
          Swap(State, U_);
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
    };
    
    template <class Weights>
    class Alignment {
      public:
        Alignment(const Weights& model)
        : w_(model)
        {}
          
        void GetAlignedSourceContext(mblas::Matrix& AlignedSourceContext,
                                     const mblas::Matrix& HiddenState,
                                     const mblas::Matrix& SourceContext) {
          using namespace mblas;  
          
          Prod(Temp1_, SourceContext, w_.U_);
          Prod(Temp2_, HiddenState, w_.W_);
          Broadcast(_1 + _2, Temp2_, w_.B_);
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
        {}
          
        void GetProbs(mblas::Matrix& Probs,
                  const mblas::Matrix& State,
                  const mblas::Matrix& Embedding,
                  const mblas::Matrix& AlignedSourceContext) {
          using namespace mblas;
          
          Broadcast(_1 + _2, Prod(T1_, State, w_.W1_), w_.B1_);
          Broadcast(_1 + _2, Prod(T2_, Embedding, w_.W2_), w_.B2_);
          Broadcast(_1 + _2, Prod(T3_, AlignedSourceContext, w_.W3_), w_.B3_);
          Element(Tanh(_1 + _2 + _3), T1_, T2_, T3_);
          Broadcast(_1 + _2, Prod(Probs, T1_, w_.W4_), w_.B4_);
          mblas::Softmax(Probs);
        }
    
        
        void Filter(const std::vector<size_t>& ids) {
          using namespace mblas;
          
          //Matrix TempWo;
          //Transpose(TempWo, w_.Wo_);
          //Assemble(FilteredWo_, TempWo, ids);
          //Transpose(FilteredWo_);
          //
          //Matrix TempWoB;
          //Transpose(TempWoB, w_.WoB_);
          //Assemble(FilteredWoB_, TempWoB, ids);
          //Transpose(FilteredWoB_);
          
          filtered_ = true;
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
      rnn_(model.decRnn_), alignment_(model.decAlignment_),
      softmax_(model.decSoftmax_)
    {}
    
    void EmptyState(mblas::Matrix& State,
                    const mblas::Matrix& SourceContext,
                    size_t batchSize = 1) {
      rnn_.InitializeState(State, SourceContext, batchSize);
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
    
    void GetHiddenState(mblas::Matrix& HiddenState,
                        const mblas::Matrix& PrevState,
                        const mblas::Matrix& Embedding) {
      rnn_.GetHiddenState(HiddenState, PrevState, Embedding);
    }
    
    void GetAlignedSourceContext(mblas::Matrix& AlignedSourceContext,
                                 const mblas::Matrix& HiddenState,
                                 const mblas::Matrix& SourceContext) {
      alignment_.GetAlignedSourceContext(AlignedSourceContext, HiddenState, SourceContext);
    }
    
    void GetNextState(mblas::Matrix& State,
                      const mblas::Matrix& HiddenState,
                      const mblas::Matrix& AlignedSourceContext) {
      rnn_.GetNextState(State, HiddenState, AlignedSourceContext);
    }
    
    
    void GetProbs(mblas::Matrix& Probs,
                  const mblas::Matrix& State,
                  const mblas::Matrix& Embedding,
                  const mblas::Matrix& AlignedSourceContext) {
      softmax_.GetProbs(Probs, State, Embedding, AlignedSourceContext);
    }
    
    
  private:
    Embeddings<Weights::DecEmbeddings> embeddings_;
    RNN<Weights::DecRnn> rnn_;
    Alignment<Weights::DecAlignment> alignment_;
    Softmax<Weights::DecSoftmax> softmax_;
};
