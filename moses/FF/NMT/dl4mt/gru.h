#pragma once

#include "mblas/matrix.h"

template <class Weights>
class SlowGRU {
  public:
    SlowGRU(const Weights& model)
    : w_(model) {}
          
    void GetNextState(mblas::Matrix& NextState,
                      const mblas::Matrix& State,
                      const mblas::Matrix& Context) const {
      using namespace mblas;
      
      const size_t cols = State.Cols();
      
      // @TODO: Optimization
      // @TODO: Launch streams to perform GEMMs in parallel
      // @TODO: Join matrices and perform single GEMM --------
      Prod(RU_, Context, w_.W_);
      Prod(H_,  Context, w_.Wx_);
      // -----------------------------------------------------
      
      // @TODO: Join matrices and perform single GEMM --------
      Prod(Temp1_, State, w_.U_);
      Prod(Temp2_, State, w_.Ux_);        
      // -----------------------------------------------------
      
      // @TODO: Organize into one kernel ---------------------
      BroadcastVec(_1 + _2, RU_, w_.B_); // Broadcasting row-wise
      Element(Logit(_1 + _2), RU_, Temp1_);
      Slice(R_, RU_, 0, cols);
      Slice(U_, RU_, 1, cols);
      
      BroadcastVec(_1 + _2, H_,    w_.Bx1_); // Broadcasting row-wise
      BroadcastVec(_1 + _2, Temp2_, w_.Bx2_); // Broadcasting row-wise
      
      Element(Tanh(_1 + _2 * _3), H_, R_, Temp2_);
      Element((1.0 - _1) * _2 + _1 * _3, U_, H_, State);
      // -----------------------------------------------------
      
      Swap(NextState, U_);
    }
    
  private:
    // Model matrices
    const Weights& w_;
    
    // reused to avoid allocation
    mutable mblas::Matrix RU_;
    mutable mblas::Matrix R_;
    mutable mblas::Matrix U_;
    mutable mblas::Matrix H_;
    mutable mblas::Matrix Temp1_;
    mutable mblas::Matrix Temp2_;
};

__global__ void gElementwiseOps(float* out,
                                const float* state,
                                const float* ru,
                                const float* h,
                                const float* t1,
                                const float* t2,
                                const float* b,
                                const float* bx1,
                                const float* bx2,
                                size_t rows, size_t cols) {

  for(int bid = 0; bid < rows; bid += gridDim.x) {
    int j = bid + blockIdx.x;
    if(j < rows) {
      float* rowOut = out + j * cols;
      const float* rowRu = ru + j * cols * 2;
      const float* rowT1 = t1 + j * cols * 2;
      
      const float* rowH = h + j * cols;
      const float* rowT2 = t2 + j * cols;
      const float* rowState = state + j * cols;
      
      for(int tid = 0; tid < cols; tid += blockDim.x) {
        int i = tid + threadIdx.x;
        if(i < cols) {
          float ev1 = expf(-(rowRu[i] + b[i] + rowT1[i]));
          float r = 1.0 / (1.0 + ev1);
          
          int k = i + cols;
          float ev2 = expf(-(rowRu[k] + b[k] + rowT1[k]));
          float u = 1.0 / (1.0 + ev2);              

          float hv = rowH[i] + bx1[i];
          float t2v = rowT2[i] + bx2[i];
          hv = tanhf(hv + r * t2v);
          rowOut[i] = (1.0 - u) * hv + u * rowState[i];
        }
      }
    }
  }
}


template <class Weights>
class FastGRU {
  public:
    FastGRU(const Weights& model)
    : w_(model) {
    }
          
    void GetNextState(mblas::Matrix& NextState,
                      const mblas::Matrix& State,
                      const mblas::Matrix& Context) const {
      using namespace mblas;
      
      const size_t cols = State.Cols();
      
      // @TODO: Optimization
      // @TODO: Launch streams to perform GEMMs in parallel
      // @TODO: Join matrices and perform single GEMM --------
      Prod(RU_, Context, w_.W_);
      Prod(H_,  Context, w_.Wx_);
      // -----------------------------------------------------
      
      // @TODO: Join matrices and perform single GEMM --------
      Prod(Temp1_, State, w_.U_);
      Prod(Temp2_, State, w_.Ux_);        
      // -----------------------------------------------------
      
      ElementwiseOps(NextState, State, RU_, H_, Temp1_, Temp2_);
    }
        
    void ElementwiseOps(mblas::Matrix& NextState,
                        const mblas::Matrix& State,
                        const mblas::Matrix& RU,
                        const mblas::Matrix& H,
                        const mblas::Matrix& Temp1,
                        const mblas::Matrix& Temp2) const {
      const size_t rows = State.Rows();
      const size_t cols = State.Cols();
      NextState.Resize(rows, cols);
      
      int blocks  = std::min(MAX_BLOCKS, (int)rows);
      int threads = std::min(MAX_THREADS, (int)cols);
      gElementwiseOps<<<blocks, threads>>>(NextState.data(), State.data(),
                                          RU.data(), H.data(),
                                          Temp1.data(), Temp2.data(),
                                          w_.B_.data(), w_.Bx1_.data(), w_.Bx2_.data(),
                                          rows, cols);
      cudaStreamSynchronize(0);
    }
    
  private:
    // Model matrices
    const Weights& w_;

    // reused to avoid allocation
    mutable mblas::Matrix RU_;
    mutable mblas::Matrix H_;
    mutable mblas::Matrix Temp1_;
    mutable mblas::Matrix Temp2_;
};

template<class T>
using GRU = FastGRU<T>;