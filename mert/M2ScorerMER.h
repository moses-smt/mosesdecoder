#ifndef MERT_M2MER_SCORER_H_
#define MERT_M2MER_SCORER_H_

#include <string>
#include <vector>
#include <functional>

#include <boost/python.hpp>

#include "Types.h"
#include "Util.h"
#include "StatisticsBasedScorer.h"

namespace MosesTuning
{

typedef char Diff;
typedef std::vector<Diff> Diffs;

template <class Sequence, class Pred>
void CreateDiffRec(size_t** c,
              const Sequence &s1,
              const Sequence &s2,
              size_t start,
              size_t i,
              size_t j,
              Diffs& diffs,
              Pred pred) {
  if(i > 0 && j > 0 && pred(s1[i - 1 + start], s2[j - 1 + start])) {
    CreateDiffRec(c, s1, s2, start, i - 1, j - 1, diffs, pred);
    diffs.push_back(Diff('m'));
  }
  else if(j > 0 && (i == 0 || c[i][j-1] >= c[i-1][j])) {
    CreateDiffRec(c, s1, s2, start, i, j-1, diffs, pred);
    diffs.push_back(Diff('i'));
  }
  else if(i > 0 && (j == 0 || c[i][j-1] < c[i-1][j])) {
    CreateDiffRec(c, s1, s2, start, i-1, j, diffs, pred);
    diffs.push_back(Diff('d'));
  }
}

template <class Sequence, class Pred>
Diffs CreateDiff(const Sequence& s1,
           const Sequence& s2,
           Pred pred) {
  
  Diffs diffs;
  
  size_t n = s2.size();
  
  int start = 0;
  int m_end = s1.size() - 1;
  int n_end = s2.size() - 1;
    
  while(start <= m_end && start <= n_end && pred(s1[start], s2[start])) {
    diffs.push_back(Diff('m'));
    start++;
  }
  while(start <= m_end && start <= n_end && pred(s1[m_end], s2[n_end])) {
    m_end--;
    n_end--;
  }
  
  size_t m_new = m_end - start + 1;
  size_t n_new = n_end - start + 1;
  
  size_t** c = new size_t*[m_new + 1];
  for(size_t i = 0; i <= m_new; ++i) {
    c[i] = new size_t[n_new + 1];
    c[i][0] = 0;
  }
  for(size_t j = 0; j <= n_new; ++j)
    c[0][j] = 0;  
  for(size_t i = 1; i <= m_new; ++i)
    for(size_t j = 1; j <= n_new; ++j)
      if(pred(s1[i - 1 + start], s2[j - 1 + start]))
        c[i][j] = c[i-1][j-1] + 1;
      else
        c[i][j] = c[i][j-1] > c[i-1][j] ? c[i][j-1] : c[i-1][j];
  
  CreateDiffRec(c, s1, s2, start, m_new, n_new, diffs, pred);
  
  for(size_t i = 0; i <= m_new; ++i)
    delete[] c[i];
  delete[] c;
    
  for (size_t i = n_end + 1; i < n; ++i)
    diffs.push_back(Diff('m'));
  
  return diffs;
}

template <class Sequence>
Diffs CreateDiff(const Sequence& s1, const Sequence& s2) {
  return CreateDiff(s1, s2, std::equal_to<typename Sequence::value_type>());
}

/**
 * M2Scorer class can compute CoNLL m2 F-score.
 */
class M2ScorerMER: public StatisticsBasedScorer
{
public:
  explicit M2ScorerMER(const std::string& config);

  virtual void setReferenceFiles(const std::vector<std::string>& referenceFiles);
  virtual void prepareStats(std::size_t sid, const std::string& text, ScoreStats& entry);

  virtual std::size_t NumberOfScores() const {
    //return 9;
    return 7;
  }

  virtual float calculateScore(const std::vector<int>& comps) const;

private:  
  boost::python::object main_namespace_;
  boost::python::object m2_;
  float alpha_;
  float beta_; 
  int max_unchanged_words_;
  bool ignore_whitespace_casing_;

  std::map<std::string, std::vector<int> > seen_;
  std::vector<std::vector<std::string> > references_;
  
  const char* code();
  
  // no copying allowed
  M2ScorerMER(const M2ScorerMER&);
  M2ScorerMER& operator=(const M2ScorerMER&);
  
  void addMERStats(std::string, size_t, std::vector<int>&);
};

float sentenceM2MER (const std::vector<float>& stats);
float sentenceScaledM2MER(const std::vector<float>& stats);
float sentenceBackgroundM2MER(const std::vector<float>& stats, const std::vector<float>& bg);

}

#endif  // MERT_CDER_SCORER_H_
