#pragma once
#include <valarray>
#include <vector>
#include <iostream>

namespace Josiah {
  
class SufficientStats {
  public:
  virtual ~SufficientStats() {}
};
  
class BleuSufficientStats : public SufficientStats {  
  public: 
    std::valarray<int> hyp;
    std::valarray<int> correct;
    float hyp_len;
    float ref_len;
    
    BleuSufficientStats(int n = 4): hyp_len(0), ref_len(0) {
      hyp.resize(n);
      correct.resize(n);
    }
  
    BleuSufficientStats(std::vector<float> & data) {
     
      assert (((data.size() -2) % 2) == 0);
      size_t n_ = (data.size() - 2) / 2;
      size_t i = 0;
      
      hyp.resize(n_); correct.resize(n_);
      
      for (; i < n_ ; ++i) {
        hyp[i] = (int) data[i];
      }
      
      for (; i < 2*n_ ; ++i) {
        correct[i-n_] = (int) data[i];
      }
      
      hyp_len = data[2*n_];
      ref_len = data[2*n_ + 1];
    }
    
    BleuSufficientStats& operator+= ( BleuSufficientStats &other ) {
      hyp_len += other.hyp_len;
      ref_len += other.ref_len;
      hyp += other.hyp;
      correct += other.correct; 
      return *this;
    }
  
    std::vector<float> data() {
      std::vector <float> data;
      for (size_t i = 0; i < hyp.size(); ++i) {
        data.push_back(hyp[i]);
      }
      for (size_t i = 0; i < correct.size(); ++i) {
        data.push_back(correct[i]);
      }
      data.push_back(hyp_len);
      data.push_back(ref_len);
      return data;  
    }
  
    virtual ~BleuSufficientStats() {}
  
    friend std::ostream& operator<<(std::ostream& out, const BleuSufficientStats& stats) {
      out << "Hyp :" ; 
      for (size_t i = 0; i < stats.hyp.size(); ++i)
        out << stats.hyp[i] << " " << std::endl;
      out << "Correct :" ;
      for (size_t i = 0; i < stats.correct.size(); ++i)
        out << stats.correct[i] << " " << std::endl;
      
      out << "Hyp len " << stats.hyp_len << ", Ref len" << stats.ref_len << std::endl;
      return out;
    }
};  

}
