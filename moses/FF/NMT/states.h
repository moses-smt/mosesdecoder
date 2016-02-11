#pragma once

#include <cmath>
#include <boost/shared_ptr.hpp>
#include <unordered_map>
#include <queue>
#include "murmur3.h"
#include "mblas/matrix.h"

class States;

class StateInfo {
  public:
    StateInfo(size_t rowNo, States* states)
    : rowNo_(rowNo), states_(states) { }

  ~StateInfo();

  size_t GetRowNo() {
    return rowNo_;
  }
    
  private:
    size_t rowNo_;
    States* states_;
};

typedef boost::shared_ptr<StateInfo> StateInfoPtr;

class States {
  public:
    void ConstructStates(mblas::Matrix& Out, const std::vector<StateInfoPtr>& infos) {
      std::vector<size_t> indeces;
      for(auto i : infos)
        indeces.push_back(i->GetRowNo());
      mblas::CopyRows(Out, States_, indeces);
    }

    void SaveStates(std::vector<StateInfoPtr>& infos, const mblas::Matrix& In) {
      std::vector<uint32_t> hashes;
      HashStates(hashes, In);
      
      std::unordered_map<uint32_t, StateInfoPtr> hashedRows;
      
      for(size_t i = 0; i < In.Rows(); ++i) {
        auto it = hashedRows.find(hashes[i]);
        if(it != hashedRows.end()) {
          infos.push_back(it->second);
        }
        else {
          size_t rowNo = 0;
          if(freeRows_.empty()) {
            rowNo = States_.Rows();
            mblas::AppendRow(States_, In, i);
          }
          else {
            rowNo = freeRows_.top();
            freeRows_.pop();
            mblas::CopyRow2(States_, In, rowNo, i);
          }
          infos.push_back(StateInfoPtr(new StateInfo(rowNo, this)));
          hashedRows.emplace(hashes[i], infos.back());
        }
      }
      
      //std::cerr << "States size: " << States_.Rows() << std::endl;
    }
     
    void Clear() {
      std::priority_queue<size_t> empty;
      freeRows_.swap(empty);
      
      mblas::Matrix emptyMatrix;
      mblas::Swap(States_, emptyMatrix);
    }
    
  private:
    
    friend class StateInfo;
    
    void Free(size_t rowNo) {
      freeRows_.push(rowNo);
    }
    
    // @TODO: parallelize this!
    void HashStates(std::vector<uint32_t>& hashes, const mblas::Matrix& In) {
      size_t rowLength = In.Cols();
      for(size_t i = 0; i < In.Rows(); ++i) {
        const float* begin = In.data() + i * rowLength;
        uint32_t* hashPtr;
        cudaMalloc((void **) &hashPtr, sizeof(uint32_t));
        
        hash_data<<<1,1>>>(begin, rowLength, hashPtr);
        
        thrust::device_ptr<uint32_t> devHashPtr(hashPtr);
        uint32_t hash = *devHashPtr;
        
        cudaFree(hashPtr);
        
        hashes.push_back(hash);
      }
    }
    
    mblas::Matrix States_;
    std::priority_queue<size_t> freeRows_;
};

//----------------------------------------------------------------------------//

StateInfo::~StateInfo() {
  //std::cerr << "Freeing " << rowNo_ << std::endl;
  states_->Free(rowNo_);
}
