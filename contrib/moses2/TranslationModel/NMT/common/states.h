#pragma once

#include <cmath>
#include <boost/shared_ptr.hpp>
#include <queue>
#include <iostream>
#include <sstream>
#include "../mblas/matrix.h"

class States;

class StateInfo {
  public:
    StateInfo(size_t rowNo, States* states)
    : rowNo_(rowNo), states_(states) { }

  ~StateInfo();

  size_t GetRowNo() {
    return rowNo_;
  }
  
  friend std::ostream& operator<<(std::ostream& o, const StateInfo& s);
  
  private:
    size_t rowNo_;
    States* states_;
};

typedef boost::shared_ptr<StateInfo> StateInfoPtr;

class States {
  public:
    void ConstructStates(mblas::Matrix& Out, const std::vector<StateInfoPtr>& infos) {
      mblas::RowPairs rowPairs;
      size_t j = 0;
      for(auto i : infos)
        rowPairs.emplace_back(j++, i->GetRowNo());
      Out.Resize(rowPairs.size(), States_.Cols());
      mblas::CopyRows(Out, States_, rowPairs);
    }

    void SaveStates(std::vector<StateInfoPtr>& infos, const mblas::Matrix& In) {
      mblas::RowPairs rowPairs;
      size_t append = States_.Rows();
      for(size_t i = 0; i < In.Rows(); ++i) {
        if(freeRows_.empty()) {
          rowPairs.emplace_back(append, i);
          infos.push_back(StateInfoPtr(new StateInfo(append, this)));
          append++;
        }
        else {
          size_t rowNo = freeRows_.top();
          freeRows_.pop();
          rowPairs.emplace_back(rowNo, i);
          infos.push_back(StateInfoPtr(new StateInfo(rowNo, this)));
        } 
      }
      if(append > States_.Rows())
        States_.Resize(append, In.Cols());
      mblas::CopyRows(States_, In, rowPairs);
      //std::cerr << "States: " << In.Rows() << " " << freeRows_.size() << " -> " << States_.Rows() << std::endl;
    }
      
    std::string ToString(size_t rowNo) {
      std::stringstream ss;
      ss << rowNo << " : ";
      for(size_t i = 0; i < 5; ++i) {
        ss << States_(rowNo, i) << " ";
      }
      return ss.str();
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
    
    mblas::Matrix States_;
    std::priority_queue<size_t> freeRows_;
};

//----------------------------------------------------------------------------//

StateInfo::~StateInfo() {
  states_->Free(rowNo_);
}

std::ostream& operator<<(std::ostream& o, const StateInfo& s) {
  return o << s.states_->ToString(s.rowNo_);
}
  
