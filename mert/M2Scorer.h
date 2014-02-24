#ifndef MERT_M2_SCORER_H_
#define MERT_M2_SCORER_H_

#include <string>
#include <vector>
#include <functional>

#include <boost/python.hpp>

#include "Types.h"
#include "Util.h"
#include "StatisticsBasedScorer.h"

namespace MosesTuning
{

template <class Sequence, class BinaryPredicate>
size_t Levenshtein(const Sequence &s1, const Sequence &s2, BinaryPredicate pred)
{
  const size_t m(s1.size());
  const size_t n(s2.size());
 
  if(m == 0)
    return n;
  if(n == 0)
    return m;
 
  size_t *costs = new size_t[n + 1];
 
  for(size_t k = 0; k <= n; k++)
    costs[k] = k;
 
  size_t i = 0;
  for (typename Sequence::const_iterator it1 = s1.begin(); it1 != s1.end(); ++it1, ++i)
  {
    costs[0] = i + 1;
    size_t corner = i;
 
    size_t j = 0;
    for (typename Sequence::const_iterator it2 = s2.begin(); it2 != s2.end(); ++it2, ++j)
    {
      size_t upper = costs[j + 1];
      if(pred(*it1, *it2))
      {
	costs[j + 1] = corner;
      } else {
	size_t t(upper < corner ? upper : corner);
        costs[j + 1] = (costs[j] < t ? costs[j] : t) + 1;
      } 
      corner = upper;
    }
  }
  
  size_t result = costs[n];
  delete [] costs;
  return result;
}

template <class Sequence>
size_t Levenshtein(const Sequence &s1, const Sequence &s2)
{
  return Levenshtein(s1, s2, std::equal_to<typename Sequence::value_type>());
}


/**
 * M2Scorer class can compute CoNLL m2 F-score.
 */
class M2Scorer: public StatisticsBasedScorer
{
public:
  explicit M2Scorer(const std::string& config);

  virtual void setReferenceFiles(const std::vector<std::string>& referenceFiles);
  virtual void prepareStats(std::size_t sid, const std::string& text, ScoreStats& entry);

  virtual std::size_t NumberOfScores() const {
    //return 9;
    return 3;
  }

  virtual float calculateScore(const std::vector<int>& comps) const;

private:  
  boost::python::object main_namespace_;
  boost::python::object m2_;
  float beta_; 
  int max_unchanged_words_;
  bool ignore_whitespace_casing_;

  std::map<std::string, std::vector<int> > seen_;
  
  const char* code();
  
  // no copying allowed
  M2Scorer(const M2Scorer&);
  M2Scorer& operator=(const M2Scorer&);
};

float sentenceM2 (const std::vector<float>& stats);
float sentenceScaledM2(const std::vector<float>& stats);
float sentenceBackgroundM2(const std::vector<float>& stats, const std::vector<float>& bg);

}

#endif  // MERT_CDER_SCORER_H_
