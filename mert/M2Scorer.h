#ifndef MERT_M2_SCORER_H_
#define MERT_M2_SCORER_H_

#include <string>
#include <vector>
#include <functional>

#include "Types.h"
#include "Util.h"
#include "StatisticsBasedScorer.h"
#include "M2.h"

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
    return 4;
  }

  virtual float calculateScore(const std::vector<ScoreStatsType>& comps) const;
  virtual float getReferenceLength(const std::vector<ScoreStatsType>& comps) const;

private:
  float beta_;
  int max_unchanged_words_;
  bool truecase_;
  bool verbose_;
  M2::M2 m2_;

  std::map<std::pair<size_t, std::string>, std::vector<ScoreStatsType> > seen_;

  // no copying allowed
  M2Scorer(const M2Scorer&);
  M2Scorer& operator=(const M2Scorer&);
};

float sentenceM2 (const std::vector<ScoreStatsType>& stats);

}

#endif  // MERT_M2_SCORER_H_
