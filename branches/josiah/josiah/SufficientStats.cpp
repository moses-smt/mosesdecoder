#include "SufficientStats.h"

namespace Josiah{
  
BleuSufficientStats::BleuSufficientStats(int n): hyp_len(0), ref_len(0), src_len(0) {
  hyp.resize(n);
  correct.resize(n);
}


void  BleuSufficientStats::initialise(std::vector<float> & data) {
  assert (((data.size() -3) % 2) == 0);
  size_t n_ = (data.size() - 3) / 2;
  size_t i = 0;
  
  hyp.resize(n_); correct.resize(n_);
  
  for (; i < n_ ; ++i) {
    hyp[i] = data[i];
  }
  
  for (; i < 2*n_ ; ++i) {
    correct[i-n_] =  data[i];
  }
  
  hyp_len = data[2*n_];
  ref_len = data[2*n_ + 1];
  src_len = data[2*n_ + 2];
} 
  
BleuSufficientStats& BleuSufficientStats::operator+= ( const BleuSufficientStats &other ) {
  hyp_len += other.hyp_len;
  ref_len += other.ref_len;
  src_len += other.src_len;
  hyp += other.hyp;
  correct += other.correct; 
  return *this;
}

BleuSufficientStats BleuSufficientStats::operator/ ( float num ) {
  BleuSufficientStats stats;
  stats.hyp_len = hyp_len/num;
  stats.ref_len = ref_len/num;
  stats.src_len = src_len/num;
  for (size_t i = 0; i < hyp.size() ; ++i) {
    stats.hyp[i] = hyp[i] / num;
  }
  for (size_t i = 0; i < correct.size() ; ++i) {
    stats.correct[i] = correct[i] / num;
  }
  return stats;
}

BleuSufficientStats& BleuSufficientStats::operator*= ( float num ) {
    hyp_len *= num;
    ref_len *= num;
    src_len *= num;
    for (size_t i = 0; i < hyp.size() ; ++i) {
      hyp[i] *= num;
    }
    for (size_t i = 0; i < correct.size() ; ++i) {
      correct[i] *= num;
    }
    return *this;
}
  
std::vector<float> BleuSufficientStats::data() {
  std::vector <float> data;
  for (size_t i = 0; i < hyp.size(); ++i) {
    data.push_back(hyp[i]);
  }
  for (size_t i = 0; i < correct.size(); ++i) {
    data.push_back(correct[i]);
  }
  data.push_back(hyp_len);
  data.push_back(ref_len);
  data.push_back(src_len);
  return data;  
}

    
void BleuSufficientStats::Zero() {
  hyp_len =0 ; ref_len = 0, src_len = 0;
  
  for (size_t i = 0; i < hyp.size(); ++i) {
    hyp[i] = 0.0;
  }
  for (size_t i = 0; i < correct.size(); ++i) {
      correct[i] = 0.0;
  }
}
  
std::ostream& operator<<(std::ostream& out, const BleuSufficientStats& stats) {
  out << "Hyp :" ; 
  for (size_t i = 0; i < stats.hyp.size(); ++i)
    out << stats.hyp[i] << " " << std::endl;
  out << "Correct :" ;
  for (size_t i = 0; i < stats.correct.size(); ++i)
    out << stats.correct[i] << " " << std::endl;
  
  out << "Hyp len " << stats.hyp_len << ", Ref len" << stats.ref_len << ", Src len" << stats.src_len << std::endl;
  return out;
}

BleuDefaultSmoothingSufficientStats::BleuDefaultSmoothingSufficientStats(int n, float smoothing_factor ) {
    hyp_len = 0.0;
    ref_len = 0.0;
    src_len = 0.0;
    hyp.resize(n, smoothing_factor);
    correct.resize(n, smoothing_factor);
    
}
  
  
}