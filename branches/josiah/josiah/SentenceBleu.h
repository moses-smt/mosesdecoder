#pragma once

#include <map>
#include <vector>
#include <valarray>
#include "GibblerExpectedLossTraining.h"
#include "GainFunction.h"

namespace Josiah {

class SentenceBLEU : public GainFunction {
 public:
  SentenceBLEU(int n, const std::vector<std::string>& refs);
  SentenceBLEU(int n, const vector<const Factor*> & ref);
    
  int GetType() const { return 1;}
  float ComputeGain(const std::vector<const Factor*>& hyp) const;
  float ComputeGain(const GainFunction& hyp) const;
  
  int GetLength() const {
    assert (lengths_.size());
    return lengths_[0];
  }
  
 private:
  struct NGramCompare {
    int operator() (const vector<const Factor*>& a, const vector<const Factor*>& b) {
      const size_t as = a.size();
      const size_t bs = b.size();
      const size_t s = (as < bs ? as : bs);
      for (size_t i = 0; i < s; ++i) {
         const int d = a[i] - b[i];
         if (d < 0) return true;
	 if (d > 0) return false;
      }
      return as < bs;
    }
  };
  typedef std::map<std::vector<const Factor*>, std::pair<int,int>, NGramCompare> NGramCountMap;

  void CountRef(const vector<const Factor*>& ref, NGramCountMap&) const;

  inline int GetClosestLength(int hl) const {
    if (lengths_.size() == 1) return lengths_[0];
    int bestd = 2000000;
    int bl = -1;
    for (vector<int>::const_iterator ci = lengths_.begin(); ci != lengths_.end(); ++ci) {
      int cl = *ci;
      if (abs(cl - hl) < bestd) {
        bestd = abs(cl - hl);
        bl = cl;
      }
    }
    return bl;
  }

  float CalcScore(const NGramCountMap & refNgrams, const NGramCountMap & hypNgrams, int hypLen) const ;
  float CalcBleu(const valarray<int> & hyp, const valarray<int> & correct, float ref_len, float hyp_len) const;
  
  const NGramCountMap& GetNgrams() const {
    return ngrams_;
  } 
  
  std::vector<int> lengths_;
  mutable NGramCountMap ngrams_;
  int n_;
};

};

