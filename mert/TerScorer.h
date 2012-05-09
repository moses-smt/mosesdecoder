#ifndef MERT_TER_SCORER_H_
#define MERT_TER_SCORER_H_

#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "Types.h"
#include "Scorer.h"

using namespace std;

class ScoreStats;

/**
 * TER scoring
 */
class TerScorer: public StatisticsBasedScorer
{
public:
  explicit TerScorer(const string& config = "");
  ~TerScorer();

  virtual void setReferenceFiles(const vector<string>& referenceFiles);
  virtual void prepareStats(size_t sid, const string& text, ScoreStats& entry);

  virtual size_t NumberOfScores() const {
    // cerr << "TerScorer: " << (LENGTH + 1) << endl;
    return kLENGTH + 1;
  }

  virtual float calculateScore(const vector<int>& comps) const;

  void whoami() const {
    cerr << "I AM TerScorer" << std::endl;
  }

private:
  const int kLENGTH;

  string m_java_env;
  string m_ter_com_env;

  // data extracted from reference files
  vector<size_t> m_ref_lengths;
  vector<multiset<int> > m_ref_tokens;
  vector<vector<int> > m_references;
  vector<vector<vector<int> > > m_multi_references;
  string m_pid;

  // no copying allowed
  TerScorer(const TerScorer&);
  TerScorer& operator=(const TerScorer&);
};

#endif // MERT_TER_SCORER_H_
