#pragma once

#include <cmath>

#include "base_matrix.h"

#define MAX_THREADS 512
#define MAX_BLOCKS 65535

#include <cublas_v2.h>   
#include <thrust/device_vector.h>
#include <thrust/functional.h>

#include "thrust_functions.h"

namespace lib = thrust;
namespace iterlib = thrust;

namespace mblas {

using namespace thrust::placeholders;

template <class VecType>
class TMatrix : public BaseMatrix {
  public:
    typedef typename VecType::value_type value_type;
    typedef typename VecType::iterator iterator;
    typedef typename VecType::const_iterator const_iterator;
    
    TMatrix()
    : rows_(0), cols_(0)
    {}
    
    TMatrix(size_t rows, size_t cols)
    : rows_(rows), cols_(cols), data_(rows_ * cols_)
    {}
    
    TMatrix(size_t rows, size_t cols, value_type val)
    : rows_(rows), cols_(cols), data_(rows_ * cols_, val)
    {}
    
    TMatrix(TMatrix&& m)
    : rows_(m.rows_), cols_(m.cols_), data_(std::move(m.data_)) {}
        
    TMatrix(const TMatrix& m) = delete;
    
    value_type operator()(size_t i, size_t j) const {
      return data_[i * cols_ + j];
    }

    size_t Rows() const {
      return rows_;
    }
    
    size_t Cols() const {
      return cols_;
    }
    
    void Resize(size_t rows, size_t cols) {
      rows_ = rows;
      cols_ = cols;
      data_.resize(rows_ * cols_);
    }
    
    void Resize(size_t rows, size_t cols, value_type val) {
      rows_ = rows;
      cols_ = cols;
      data_.resize(rows_ * cols_, val);
    }
    
    void Reserve(size_t rows, size_t cols) {
      data_.reserve(rows * cols);
    }
    
    void Reshape(size_t rows, size_t cols) {
      rows_ = rows;
      cols_ = cols;
    }
    
    void Purge() {
      Clear();
      VecType temp;
      data_.swap(temp);
    }

    void Clear() {
      data_.clear();
      rows_ = 0;
      cols_ = 0;
    }
    
    VecType& GetVec() {
      return data_;
    }
    
    const VecType& GetVec() const {
      return data_;
    }
    
    value_type* data() {
      return thrust::raw_pointer_cast(data_.data());
    }
    
    const value_type* data() const {
      return thrust::raw_pointer_cast(data_.data());
    }
    
    iterator begin() {
      return data_.begin();
    }
    
    iterator end() {
      return data_.end();
    }
    
    const_iterator begin() const{
      return data_.begin();
    }
    
    const_iterator end() const {
      return data_.end();
    }
    
    size_t size() const {
      return data_.size();    
    }
    
  private:
    size_t rows_;
    size_t cols_;
    VecType data_;
};

typedef thrust::device_vector<float> FVec;
typedef thrust::device_vector<unsigned int> IVec;

class CublasHandler {
  public:

    ~CublasHandler() {
      if(handle_ != nullptr) {
        cublasDestroy(*handle_);
        delete handle_;
        handle_ = nullptr;
      }
    }
    
    static cublasHandle_t GetHandle() {
      if(instance_.handle_ == nullptr) {
        instance_.CreateHandle();
      }
      return *instance_.handle_;
    }
    
    static void StaticHandle() {
      instance_.CreateHandle();
    }
    
  private:    

    void CreateHandle() {
      if(handle_ != nullptr) {
        cublasDestroy(*handle_);
        delete handle_;
      }
      handle_ = new cublasHandle_t;
      cublasCreate(handle_);
    }

    static thread_local cublasHandle_t* handle_;
    static CublasHandler instance_;
};

CublasHandler CublasHandler::instance_;
thread_local cublasHandle_t* CublasHandler::handle_ = nullptr;

typedef TMatrix<FVec> Matrix;
typedef TMatrix<IVec> IMatrix;

template <class M>
void debug1(const M& m, size_t pos = 0, size_t l = 5) {
  std::cerr << m.Rows() << " " << m.Cols() << std::endl;
  for(size_t i = 0; i < m.Rows(); ++i) {
    for(size_t j = pos; j < m.Cols() && j < pos + l; ++j) {
      std::cerr << m.GetVec()[i * m.Cols() + j] << " ";
    }
    std::cerr << std::endl;
    if(i == 4)
      break;
  }
}

Matrix& Swap(Matrix& Out, Matrix& In) {
  size_t iRows = In.Rows();
  size_t iCols = In.Cols();
  size_t oRows = Out.Rows();
  size_t oCols = Out.Cols();
  
  Out.Reshape(iRows, iCols);
  In.Reshape(oRows, oCols);
  
  In.GetVec().swap(Out.GetVec());
  return Out;
}

Matrix& Mean(Matrix& Out, const Matrix& In) {
  size_t m = In.Rows();
  size_t n = In.Cols();
  
  Out.Resize(1, n, 0.f);
  Matrix Ones(1, m, 1.f);
  
  float alpha = 1.0 / m;
  float beta  = 0.0;
  cublasSgemv(CublasHandler::GetHandle(), CUBLAS_OP_N, n, m, &alpha, In.data(), n,
              Ones.data(), 1, &beta, Out.data(), 1);
  return Out;
}

Matrix& Transpose(Matrix& Out, const Matrix& In) {
  size_t m = In.Rows();
  size_t n = In.Cols();
  
  Out.Resize(n, m);
  
  float alpha = 1.0;
  float beta  = 0.0;
  
  cublasSgeam(CublasHandler::GetHandle(), CUBLAS_OP_T, CUBLAS_OP_T, m, n, &alpha, In.data(), n,
              &beta, In.data(), n, Out.data(), m); 
  
  return Out;
}

Matrix& Transpose(Matrix& Out) {
  Matrix Temp;
  Transpose(Temp, Out);
  Swap(Out, Temp);
  return Out;
}

Matrix& Copy(Matrix& Out, const Matrix& In) {
  Out.Resize(In.Rows(), In.Cols());
  lib::copy(In.begin(), In.end(), Out.begin());
  return Out;
}

Matrix& AppendRow(Matrix& Out, const Matrix& In, size_t i) {
  size_t oldSize = Out.GetVec().size();
  size_t addSize = In.Cols();
  Out.Resize(Out.Rows() + 1, In.Cols());
  Out.GetVec().resize(oldSize + addSize);
  size_t start = In.Cols() * i;
  size_t end   = In.Cols() * (i + 1);
  lib::copy(In.begin() + start, In.begin() + end, Out.begin() + oldSize);
  return Out;
}

Matrix& AppendRows(Matrix& Out, const Matrix& In) {
  size_t oldSize = Out.GetVec().size();
  size_t addSize = In.GetVec().size();
  Out.Resize(Out.Rows() + In.Rows(), In.Cols());
  Out.GetVec().resize(oldSize + addSize);
  lib::copy(In.begin(), In.end(), Out.begin() + oldSize);
  return Out;
}

Matrix& PrependRows(Matrix& Out, const Matrix& In) {
  Out.Resize(Out.Rows() + In.Rows(), In.Cols());
  Out.GetVec().insert(Out.begin(), In.begin(), In.end());
  return Out;
}

Matrix& PasteRow(Matrix& Out,
                 const Matrix& In,
                 const size_t r = 0, const size_t c = 0) {
  size_t start = r * Out.Cols() + c;
  lib::copy(In.begin(), In.end(), Out.begin() + start);
  return Out;
}

Matrix& CopyRow(Matrix& Out,
                const Matrix& In,
                const size_t r = 0, const size_t c = 0) {
  size_t length = In.Cols() - c;
  Out.Resize(1, length);
  size_t start = r * In.Cols() + c;
  size_t end   = start + length;
  lib::copy(In.begin() + start, In.begin() + end, Out.begin());
  return Out;
}

typedef std::pair<size_t, size_t> RowPair;
typedef std::vector<RowPair> RowPairs;
typedef thrust::device_vector<RowPair> DeviceRowPairs;

__global__ void gCopyRows(float* out, const float* in, size_t cols,
                          const RowPair* devPairs, size_t numPairs) {
  for(int bid = 0; bid < numPairs; bid += gridDim.x) {
    int j = bid + blockIdx.x;
    if(j < numPairs) {
      size_t dstId = devPairs[j].first;
      size_t srcId = devPairs[j].second;
      
      float* rowOut = out + dstId * cols;
      const float* rowIn = in + srcId * cols;
      
      for(int tid = 0; tid < cols; tid += blockDim.x) {
        int i = tid + threadIdx.x;
        if(i < cols)
          rowOut[i] = rowIn[i];
      }
    }
  }
}

Matrix& CopyRows(Matrix& Out,
                 const Matrix& In,
                 const RowPair* devPairs,
                 size_t numPairs) {
  float* d_out = Out.data();
  const float* d_in = In.data();
  
  int threads = std::min(MAX_THREADS, (int)In.Cols());
  int blocks = std::min(MAX_BLOCKS, (int)numPairs);;
  gCopyRows<<<blocks, threads>>>(d_out, d_in, In.Cols(), devPairs, numPairs);
  cudaStreamSynchronize(0);
  return Out;
}

Matrix& CopyRows(Matrix& Out,
                 const Matrix& In,
                 const RowPairs& pairs) {
  thrust::device_vector<RowPair> devPairs = pairs;
  CopyRows(Out, In, thrust::raw_pointer_cast(devPairs.data()), devPairs.size());
  return Out;
}

Matrix& Assemble(Matrix& Out,
                 const Matrix& In,
                 const std::vector<size_t>& indeces) {
  RowPairs rowPairs;
  for(size_t i = 0; i < indeces.size(); i++)
    rowPairs.emplace_back(i, indeces[i]);
  Out.Resize(rowPairs.size(), In.Cols());
  CopyRows(Out, In, rowPairs);
  return Out;
}

__global__ void gSlice(float* out, const float* in,
                       size_t n, size_t dim,
                       size_t rows, size_t cols) {
  for(int bid = 0; bid < rows; bid += gridDim.x) {
    int j = bid + blockIdx.x;
    if(j < rows) {
      float* rowOut = out + j * dim;
      const float* rowIn = in + j * cols + n * dim;
      
      for(int tid = 0; tid < dim; tid += blockDim.x) {
        int i = tid + threadIdx.x;
        if(i < dim)
          rowOut[i] = rowIn[i];
      }
    }
  }
}

Matrix& Slice(Matrix& Out,
              const Matrix& In,
              size_t n, size_t dim) {
  
  Out.Resize(In.Rows(), dim);
  
  float* d_out = Out.data();
  const float* d_in = In.data();
  
  int threads = std::min(MAX_THREADS, (int)dim);
  int blocks = std::min(MAX_BLOCKS, (int)In.Rows());
  gSlice<<<blocks, threads>>>(d_out, d_in, n, dim, In.Rows(), In.Cols());
  cudaStreamSynchronize(0);
  return Out;
}

Matrix& Prod(cublasHandle_t handle, Matrix& C, const Matrix& A, const Matrix& B,
             bool transA = false, bool transB = false) {
  Matrix::value_type alpha = 1.0;
  Matrix::value_type beta = 0.0;

  size_t m = A.Rows();
  size_t k = A.Cols();
  if(transA)
    std::swap(m, k);
  
  size_t l = B.Rows();
  size_t n = B.Cols();
  if(transB)
    std::swap(l, n);
  
  size_t lda = A.Cols();                                                                              
  size_t ldb = B.Cols();                                                                              
  size_t ldc = B.Cols();

  if(transB)
    ldc = B.Rows();  
  
  C.Resize(m, n);
  
  cublasOperation_t opA = transA ? CUBLAS_OP_T : CUBLAS_OP_N;
  cublasOperation_t opB = transB ? CUBLAS_OP_T : CUBLAS_OP_N;

  cublasSgemm(handle, opB, opA,
              n, m, k, &alpha, B.data(), ldb, A.data(), lda, &beta, C.data(), ldc);
  return C;
}

Matrix& Prod(Matrix& C, const Matrix& A, const Matrix& B,
             bool transA = false, bool transB = false) {
 
 return Prod(CublasHandler::GetHandle(), C, A, B, transA, transB);
}

__global__ void gSoftMax(float* softMaxP, size_t rows, size_t cols) {
  for(int bid = 0; bid < rows; bid += gridDim.x) {
    int j = bid + blockIdx.x;
    if(j < rows) { 
      extern __shared__ float _share[];
      float* _sum = _share + blockDim.x;
      float* sp = softMaxP + j * cols;
      _sum[threadIdx.x] = 0.0;
      for(int tid = 0; tid < cols; tid += blockDim.x) {
        int id = tid + threadIdx.x;
        if(id < cols) {
          sp[id] = __expf(sp[id]);
          _sum[threadIdx.x] += sp[id];
        }
      }
      __syncthreads();
      int len = blockDim.x;
      while(len != 1) {
        __syncthreads();
        int skip = (len + 1) >> 1;
        if(threadIdx.x < (len >> 1))
          _sum[threadIdx.x] += _sum[threadIdx.x + skip];
        len = (len + 1) >> 1;
      }
      __syncthreads();
      for(int tid = 0; tid < cols; tid += blockDim.x){
        int id = tid + threadIdx.x;
        if(id < cols)
          sp[id] /= _sum[0];
      }
    }
  }
}

Matrix& Softmax(Matrix& Out) {
  int blocks = std::min(MAX_BLOCKS, (int)Out.Rows());
  int threads = std::min(MAX_THREADS, (int)Out.Cols());
  int shared = sizeof(float) * threads * 2;
  gSoftMax<<<blocks, threads, shared>>>(Out.data(), Out.Rows(), Out.Cols());
  cudaStreamSynchronize(0);
  return Out;
}

template <class Functor>
__global__ void gBroadcast(Functor functor,
                           float* out, const float* in1, const float* in2,
                           size_t rows, size_t rows1, size_t cols) {
  for(int bid = 0; bid < rows; bid += gridDim.x) {
    int j = bid + blockIdx.x;
    if(j < rows) { 
      float* rowOut = out + j * cols;
    
      const float* rowIn1 = in1 + (j % rows1) * cols;
      const float* rowIn2 = in2 + (j / rows1) * cols;
      
      for(int tid = 0; tid < cols; tid += blockDim.x) {
        int i = tid + threadIdx.x;
        if(i < cols)
          rowOut[i] = functor(rowIn1[i], rowIn2[i]);
      }
    }
  }
}

template <class Functor>
Matrix& Broadcast(Functor functor, Matrix& Out, const Matrix& In, cudaStream_t stream = 0) {
  size_t rows1 = Out.Rows();
  size_t rows2 = In.Rows();
  
  size_t rows = rows1 * rows2;
  size_t cols  = Out.Cols();
  
  Matrix Temp(rows, cols, 1.0);
  
  float* d_out = Temp.data();
  const float* d_in1 = Out.data();
  const float* d_in2 = In.data();
  
  int blocks  = std::min(MAX_BLOCKS, (int)rows);
  int threads = std::min(MAX_THREADS, (int)cols);
  gBroadcast<<<blocks, threads, 0, stream>>>(functor, d_out, d_in1, d_in2,
                                             rows, rows1, cols);
  cudaStreamSynchronize(stream);
  Swap(Out, Temp);
  return Out;
}

template <class Functor>
__global__ void gElement(Functor functor, float* out,
                         size_t rows, size_t cols) {
  for(int bid = 0; bid < rows; bid += gridDim.x) {
    int j = bid + blockIdx.x;
    if(j < rows) {
      float* rowOut = out + j * cols;
      for(int tid = 0; tid < cols; tid += blockDim.x) {
        int i = tid + threadIdx.x;
        if(i < cols)
          rowOut[i] = functor(rowOut[i]);;
      }
    }
  }
}

template <class Functor>
__global__ void gElement(Functor functor,
                         float* out, const float* in,
                         size_t rows, size_t cols) {
  for(int bid = 0; bid < rows; bid += gridDim.x) {
    int j = bid + blockIdx.x;
    if(j < rows) {
      float* rowOut = out + j * cols;
      const float* rowIn = in + j * cols;
      
      for(int tid = 0; tid < cols; tid += blockDim.x) {
        int i = tid + threadIdx.x;
        if(i < cols)
          rowOut[i] = functor(rowOut[i], rowIn[i]);;
      }
    }
  }
}

template <class Functor>
__global__ void gElement(Functor functor,
                         float* out, const float* in1, const float* in2,
                         size_t rows, size_t cols) {
  for(int bid = 0; bid < rows; bid += gridDim.x) {
    int j = bid + blockIdx.x;
    if(j < rows) {
      float* rowOut = out + j * cols;
      const float* rowIn1 = in1 + j * cols;
      const float* rowIn2 = in2 + j * cols;
      
      for(int tid = 0; tid < cols; tid += blockDim.x) {
        int i = tid + threadIdx.x;
        if(i < cols)
          rowOut[i] = functor(rowOut[i], rowIn1[i], rowIn2[i]);
      }
    }
  }
}

template <class Functor>
Matrix& Element(Functor functor, Matrix& Out) {
  float* d_out = Out.data();  
  int blocks  = std::min(MAX_BLOCKS, (int)Out.Rows());
  int threads = std::min(MAX_THREADS, (int)Out.Cols());
  gElement<<<blocks, threads>>>(functor, d_out, Out.Rows(), Out.Cols());
  cudaStreamSynchronize(0);
  return Out;
}

template <class Functor>
Matrix& Element(Functor functor,
                Matrix& Out, const Matrix& In) {
  float* d_out = Out.data();
  const float* d_in = In.data();
  
  int blocks  = std::min(MAX_BLOCKS, (int)Out.Rows());
  int threads = std::min(MAX_THREADS, (int)Out.Cols());
  gElement<<<blocks, threads>>>(functor, d_out, d_in, Out.Rows(), Out.Cols());
  cudaStreamSynchronize(0);
  return Out;
}

template <class Functor>
Matrix& Element(Functor functor,
                Matrix& Out, const Matrix& In1, const Matrix& In2) {
  
  float* d_out = Out.data();
  const float* d_in1 = In1.data();
  const float* d_in2 = In2.data();
  
  int blocks  = std::min(MAX_BLOCKS, (int)Out.Rows());
  int threads = std::min(MAX_THREADS, (int)Out.Cols());
  gElement<<<blocks, threads>>>(functor, d_out, d_in1, d_in2,
                                Out.Rows(), Out.Cols());
  cudaStreamSynchronize(0);
  return Out;
}

template <class Functor>
__global__ void gPairwiseReduce(Functor functor,
                                float* out, const float* in,
                                size_t rows, size_t cols) {
  for(int bid = 0; bid < rows; bid += gridDim.x) {
    int j = bid + blockIdx.x;
    if(j < rows) {
      const float* rowIn = in + j * cols * 2;
      float* rowOut = out + j * cols;
      for(int tid = 0; tid < cols; tid += blockDim.x) {
        int i = tid + threadIdx.x;
        if(i < cols)
          rowOut[i] = functor(rowIn[i * 2], rowIn[i * 2 + 1]);
      }
    }
  }
}

template <class Functor>
Matrix& PairwiseReduce(Functor functor, Matrix& Out) {
  Matrix Temp(Out.Rows(), Out.Cols() / 2);
  const float* d_in = Out.data();
  float* d_out = Temp.data();
  
  int blocks  = std::min(MAX_BLOCKS, (int)Temp.Rows());
  int threads = std::min(MAX_THREADS, (int)Temp.Cols());
  gPairwiseReduce<<<blocks, threads>>>(functor, d_out, d_in,
                                       Temp.Rows(), Temp.Cols());
  cudaStreamSynchronize(0);
  Swap(Out, Temp);
  return Out;
}

}