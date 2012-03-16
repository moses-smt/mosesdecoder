#ifndef MERT_PER_SCORER_H_
#define MERT_PER_SCORER_H_

#include <set>
#include <string>
#include <vector>
#include "Types.h"
#include "Scorer.h"

using namespace std;

class ScoreStats;

/**
 * An implementation of position-independent word error rate.
 * This is defined as
 *   1 - (correct - max(0,output_length - ref_length)) / ref_length
 * In fact, we ignore the " 1 - " so that it can be maximised.
 */
class PerScorer: public StatisticsBasedScorer
{
public:
  explicit PerScorer(const string& config = "");
  ~PerScorer();

  virtual void setReferenceFiles(const vector<string>& referenceFiles);
  virtual void prepareStats(size_t sid, const string& text, ScoreStats& entry);
  virtual size_t NumberOfScores() const { return 3; }
  virtual float calculateScore(const vector<int>& comps) const;

private:
  // no copying allowed
  PerScorer(const PerScorer&);
  PerScorer& operator=(const PerScorer&);

  // data extracted from reference files
  vector<size_t> m_ref_lengths;
  vector<multiset<int> > m_ref_tokens;
};

#endif  // MERT_PER_SCORER_H_
