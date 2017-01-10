#pragma once

#include "../cnpy/cnpy.h"
#include "../mblas/matrix.h"

class NpzConverter {
  private:
    class NpyMatrixWrapper {
      public:
        NpyMatrixWrapper(const cnpy::NpyArray& npy)
        : npy_(npy) {}
        
        size_t size() const {
          return size1() * size2();
        }
        
        float* data() const {
          return (float*)npy_.data;
        }
        
        float operator()(size_t i, size_t j) const {
          return ((float*)npy_.data)[i * size2() + j];
        }
        
        size_t size1() const {
          return npy_.shape[0];
        }
        
        size_t size2() const {
          if(npy_.shape.size() == 1)
            return 1;
          else
            return npy_.shape[1];        
        }
        
      private:
        const cnpy::NpyArray& npy_;
    };
  
  public:
    NpzConverter(const std::string& file)
      : model_(cnpy::npz_load(file)),
        destructed_(false) {
      }
    
    ~NpzConverter() {
      if(!destructed_)
        model_.destruct();
    }
    
    void Destruct() {
      model_.destruct();
      destructed_ = true;
    }
    
    mblas::Matrix operator[](const std::string& key) const {
      mblas::Matrix matrix;
      auto it = model_.find(key);
      if(it != model_.end()) {
        NpyMatrixWrapper np(it->second);
        matrix.Resize(np.size1(), np.size2());
        lib::copy(np.data(), np.data() + np.size(), matrix.begin());
      }
      else {
        std::cerr << "Missing " << key << std::endl; 
      }
      return std::move(matrix);
    }
  
    mblas::Matrix operator()(const std::string& key,
                                   bool transpose) const {
      mblas::Matrix matrix;
      auto it = model_.find(key);
      if(it != model_.end()) {
        NpyMatrixWrapper np(it->second);
        matrix.Resize(np.size1(), np.size2());
        lib::copy(np.data(), np.data() + np.size(), matrix.begin());
      }
      mblas::Transpose(matrix);
      return std::move(matrix);
    }
  
  private:
    cnpy::npz_t model_;
    bool destructed_;
};
