#ifndef ORLM_QUANTIZER_H
#define ORLM_QUANTIZER_H

#include <vector>
#include <cmath>
#include <algorithm>
#include "types.h"

static const float kFloatErr = 0.00001f;

#ifdef WIN32
#define log2(X) (log((double)X)/log((double)2))
#endif

//! @todo ask abby2
class LogQtizer
{
public:
  LogQtizer(float i): base_(pow(2, 1 / i)) {
	UTIL_THROW_IF2(base_ <= 1, "Can't calculate log base less than 1");
    max_code_ = 0;
    float value = 1; // code = 1 -> value = 1 for any base
    std::vector<float> code_to_value_vec;
    while (log2(value) < 30) { // assume 2^30 is largest count
      code_to_value_vec.push_back(value);
      value = pow(base_, ++max_code_);
    }
    code_to_value_vec.push_back(value);  // store max_code_ so in total [0, max_code_]
    // get valid range
    max_value_ = code_to_value_vec[max_code_];
    min_value_ = 1;
    // store codes in array for lookup
    code_to_value_ = new float[max_code_ + 1];
    code_to_log_value_ = new float[max_code_ + 1];
    for (int j = 0; j <= max_code_; ++j) {
      // map to integers
      code_to_value_[j] = floor(kFloatErr + code_to_value_vec[j]); //
      code_to_log_value_[j] = log10(code_to_value_[j]); // log_base 10 to match srilm
    }
    std::cerr << "Initialized quantization (size = " << max_code_ + 1 << ")" << std::endl;
  }
  LogQtizer(FileHandler* fin) {
	UTIL_THROW_IF2(fin == NULL, "Null file handle");
    load(fin);
  }
  int code(float value) {
    // should just be: return log_b(value)
    UTIL_THROW_IF2(value < min_value_ || value > max_value_,
    		"Value " << value << " out of bound");

    // but binary search removes errors due to floor operator above
    int code =  static_cast<int>(std::lower_bound(code_to_value_, code_to_value_+ max_code_,
                                 value) - code_to_value_);
    // make sure not overestimating
    code = code_to_value_[code] > value ? code - 1 : code;
    return code;
  }
  inline float value(int code) {
    // table look up for values
    return code_to_value_[code];
  }
  inline int maxcode() {
    return max_code_;
  }
  inline float logValue(int code) {
    // table look up for log of values
    return code_to_log_value_[code];
  }
  ~LogQtizer() {
    delete[] code_to_value_;
    delete[] code_to_log_value_;
  }
  void save(FileHandler* fout) {
    fout->write((char*)&base_, sizeof(base_));
    fout->write((char*)&max_code_, sizeof(max_code_));
    fout->write((char*)&max_value_, sizeof(max_value_));
    fout->write((char*)&min_value_, sizeof(min_value_));
    for (int j = 0; j <= max_code_; ++j)
      fout->write((char*)&code_to_value_[j], sizeof(code_to_value_[j]));
    for (int j = 0; j <= max_code_; ++j)
      fout->write((char*)&code_to_log_value_[j], sizeof(code_to_log_value_[j]));
    std::cerr << "Saved log codebook with " << max_code_ + 1 << " codes." <<std::endl;
  }
private:
  float base_;
  float* code_to_value_;
  float* code_to_log_value_;
  int max_code_;
  float max_value_;
  float min_value_;
  void load(FileHandler* fin) {
    fin->read((char*)&base_, sizeof(base_));
    fin->read((char*)&max_code_, sizeof(max_code_));
    fin->read((char*)&max_value_, sizeof(max_value_));
    fin->read((char*)&min_value_, sizeof(min_value_));
    code_to_value_ = new float[max_code_ + 1];
    for(int j = 0; j <= max_code_; ++j)
      fin->read((char*)&code_to_value_[j], sizeof(code_to_value_[j]));
    code_to_log_value_ = new float[max_code_ + 1];
    for(int j = 0; j <= max_code_; ++j)
      fin->read((char*)&code_to_log_value_[j], sizeof(code_to_log_value_[j]));
    std::cerr << "Loaded log codebook with " << max_code_ + 1 << " codes." << std::endl;
  }
};

#endif
