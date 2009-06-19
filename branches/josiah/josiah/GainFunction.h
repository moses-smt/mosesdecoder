#pragma once

#include <string>
#include <vector>
#include "SufficientStats.h"

namespace Moses { class Factor; }

namespace Josiah {

// represents a *sentence-level* gain function
class GainFunction {
 public:
  virtual ~GainFunction();
  virtual float ComputeGain(const std::vector<const Moses::Factor*>& hyp) const = 0;
  virtual float ComputeGain(const GainFunction& hyp) const = 0;
  virtual float GetAverageReferenceLength() const = 0;
  virtual int GetType() const {return 0;}
  virtual void GetSufficientStats(const std::vector<const Moses::Factor*>& hyp, SufficientStats*) const = 0;
  static void ConvertStringToFactorArray(const std::string& str, std::vector<const Moses::Factor*>* out);
};

class GainFunctionVector {
 public:
  const GainFunction& operator[](int n) const {
    return *v[n];
  }
  void push_back(const GainFunction* f) {
    v.push_back(f);
  }
  size_t size() const { return v.size(); }
  ~GainFunctionVector() {
    for (unsigned i=0;i<v.size();++i)
      delete v[i];
  }
 private:
  std::vector<const GainFunction*> v;
};

}

