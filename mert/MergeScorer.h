#ifndef MERT_MERGE_SCORER_H_
#define MERT_MERGE_SCORER_H_

#include <string>
#include <vector>

#include "StatisticsBasedScorer.h"

namespace MosesTuning
{
  

class PerScorer;
class ScoreStats;

const int kMergeScorerLength = 4;

/**
 * Merge scoring.
 */
class MergeScorer: public StatisticsBasedScorer {
public:
  explicit MergeScorer(const std::string& config = "");
  ~MergeScorer();

  virtual void setReferenceFiles(const std::vector<std::string>& referenceFiles);
  virtual void prepareStats(std::size_t sid, const std::string& text, ScoreStats& entry);
  virtual std::size_t NumberOfScores() const { return 0; }

protected:
  friend class PerScorer;
  virtual float calculateScore(const std::vector<int>& comps) const;

 private:
  // no copying allowed
  MergeScorer(const MergeScorer&);
  MergeScorer& operator=(const MergeScorer&);
};

}

#endif  // MERT_MERGE_SCORER_H_
