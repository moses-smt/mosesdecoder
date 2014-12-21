#pragma once

#include <vector>

template <size_t MAX_NGRAM_ORDER = 4, bool SMOOTH = true>
class BLEU {      
  public:    
    template <class Sentence>
    float operator()(const Sentence& c, const Sentence& r) const {
      if(c.size() == 0 || r.size() == 0)
        return 0;
      
      std::vector<float> stats(MAX_NGRAM_ORDER * 3, 0);
      computeBLEUSymStats(c, r, stats);
      return computeBLEUSym(stats);
    }
    
    float computeBLEUSym(const std::vector<float>& stats) const {
      UTIL_THROW_IF(stats.size() != MAX_NGRAM_ORDER * 3,
                    util::Exception, "Error");
    
      float logbleu1 = 0.0;
      float logbleu2 = 0.0;
      
      float smoothing = 0.0;
      for (size_t i = 0; i < MAX_NGRAM_ORDER; ++i) {
        
        if(SMOOTH && i > 0)
          smoothing = 1.0;
      
        if(stats[3*i+1] + smoothing == 0)
          return 0.0;
      
        logbleu1 += log(stats[3*i] + smoothing) - log(stats[3*i+1] + smoothing);
        logbleu2 += log(stats[3*i] + smoothing) - log(stats[3*i+2] + smoothing);
      }
      logbleu1 /= MAX_NGRAM_ORDER;
      logbleu2 /= MAX_NGRAM_ORDER;
      
      // reflength divided by test length
      const float brevity1 = 1.0 - static_cast<float>(stats[2]) / stats[1];
      if (brevity1 < 0.0) {
        logbleu1 += brevity1;
      }
      const float brevity2 = 1.0 - static_cast<float>(stats[1]) / stats[2];
      if (brevity2 < 0.0) {
        logbleu2 += brevity2;
      }
    
      return exp((logbleu1 + logbleu2)/2);
    }
    
    template <class Sentence>
    void computeBLEUSymStats(const Sentence& c, const Sentence& r,
                             std::vector<float>& stats) const {
      
      for(size_t i = 0; i < MAX_NGRAM_ORDER; i++) {
        size_t correct = 0;
        
        // Check for common n-grams if there where common (n-1)-grams
        if(i == 0 || (i > 0 && stats[(i - 1) * 3] > 0)) 
          countCommon(c.ngrams()[i], r.ngrams()[i], correct);
        
        stats[i * 3]     += correct;
        stats[i * 3 + 1] += c.ngrams()[i].size();
        stats[i * 3 + 2] += r.ngrams()[i].size();
      }
    }
    
    size_t numStats() {
      return MAX_NGRAM_ORDER * 3;
    }

  private:
    
    template <class SortedNGrams>
    void countCommon(const SortedNGrams& n1,
                     const SortedNGrams& n2,
                     size_t& common) const {
      typename SortedNGrams::const_iterator it1 = n1.begin();
      typename SortedNGrams::const_iterator it2 = n2.begin();
      
      common = 0;
      while (it1 != n1.end() && it2 != n2.end()) {
        if (*it1 < *it2) {
          ++it1;
        }
        else {
          if (!(*it2 < *it1)) {
            ++common;
            ++it1;
          }
          ++it2;
        }
      }
    }    
};
