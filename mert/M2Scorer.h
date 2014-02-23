#ifndef MERT_M2_SCORER_H_
#define MERT_M2_SCORER_H_

#include <string>
#include <vector>
#include <boost/python.hpp>

#include "Types.h"
#include "StatisticsBasedScorer.h"

namespace MosesTuning
{

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
    return 3;
  }

  virtual float calculateScore(const std::vector<int>& comps) const;

private:  
  boost::python::object main_namespace_;
  std::string beta_; 
  std::string max_unchanged_words_;
  std::string ignore_whitespace_casing_;

  std::map<std::string, std::vector<int> > seen_;
  
  const char* code();
  
  // no copying allowed
  M2Scorer(const M2Scorer&);
  M2Scorer& operator=(const M2Scorer&);
};

/** Computes sentence-level BLEU+1 score.
 * This function is used in PRO.
 */
float sentenceM2 (const std::vector<float>& stats);
float sentenceSmoothingM2(const std::vector<float>& stats, float smoothing);
float sentenceBackgroundM2(const std::vector<float>& stats, const std::vector<float>& bg);

}

#endif  // MERT_CDER_SCORER_H_
