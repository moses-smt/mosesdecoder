#ifndef MERT_MERGE_SCORER_H_
#define MERT_MERGE_SCORER_H_

#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "Scorer.h"

using namespace std;

class PerScorer;
class ScoreStats;

const int kMergeScorerLength = 4;

/**
 * Merge scoring.
 */
class MergeScorer: public StatisticsBasedScorer {
public:
  explicit MergeScorer(const string& config = "");
  ~MergeScorer();

  virtual void setReferenceFiles(const vector<string>& referenceFiles);
  virtual void prepareStats(size_t sid, const string& text, ScoreStats& entry);
  virtual size_t NumberOfScores() const { return 0; }

protected:
  friend class PerScorer;
  virtual float calculateScore(const vector<int>& comps) const;

 private:
  // no copying allowed
  MergeScorer(const MergeScorer&);
  MergeScorer& operator=(const MergeScorer&);
};

#endif  // MERT_MERGE_SCORER_H_
