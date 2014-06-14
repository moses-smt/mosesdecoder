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
