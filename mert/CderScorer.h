#ifndef MERT_CDER_SCORER_H_
#define MERT_CDER_SCORER_H_

#include <string>
#include <vector>
#include "Types.h"
#include "Scorer.h"

using namespace std;

/**
 * CderScorer class can compute both CDER and WER metric.
 */
class CderScorer: public StatisticsBasedScorer {
 public:
  explicit CderScorer(const string& config, bool allowed_long_jumps = true);
  ~CderScorer();

  virtual void setReferenceFiles(const vector<string>& referenceFiles);

  virtual void prepareStats(size_t sid, const string& text, ScoreStats& entry);

  virtual void prepareStatsVector(size_t sid, const string& text, vector<int>& stats);

  virtual size_t NumberOfScores() const { return 2; }

  virtual float calculateScore(const vector<int>& comps) const;

 private:
  bool m_allowed_long_jumps;

  typedef vector<int> sent_t;
  vector<vector<sent_t> > m_ref_sentences;

  void computeCD(const sent_t& cand, const sent_t& ref,
                 vector<int>& stats) const;

  // no copying allowed
  CderScorer(const CderScorer&);
  CderScorer& operator=(const CderScorer&);
};

#endif  // MERT_CDER_SCORER_H_
