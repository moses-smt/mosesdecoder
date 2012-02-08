#ifndef __MERGESCORER_H__
#define __MERGESCORER_H__

#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "Scorer.h"

using namespace std;

class PerScorer;
class ScoreStats;

/**
 * Merge scoring.
 */
class MergeScorer: public StatisticsBasedScorer {
public:
  explicit MergeScorer(const string& config = "");
  ~MergeScorer();

  virtual void setReferenceFiles(const vector<string>& referenceFiles);
  virtual void prepareStats(size_t sid, const string& text, ScoreStats& entry);

  void whoami() const {
    cerr << "I AM MergeScorer" << endl;
  }

protected:
  friend class PerScorer;
  virtual float calculateScore(const vector<int>& comps) const;

 private:
  const int kLENGTH;

  // no copying allowed
  MergeScorer(const MergeScorer&);
  MergeScorer& operator=(const MergeScorer&);
};

#endif  //__TERSCORER_H
