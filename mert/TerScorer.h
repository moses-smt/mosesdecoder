#ifndef __TERSCORER_H__
#define __TERSCORER_H__

#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "Types.h"
#include "Scorer.h"

using namespace std;

// enum TerReferenceLengthStrategy { TER_AVERAGE, TER_SHORTEST, TER_CLOSEST };

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

  virtual void whoami() const {
    cerr << "I AM TerScorer" << std::endl;
  }

  virtual size_t NumberOfScores() const {
    // cerr << "TerScorer: " << (LENGTH + 1) << endl;
    return kLENGTH + 1;
  }

  virtual float calculateScore(const vector<int>& comps) const;

private:
  const int kLENGTH;

  string javaEnv;
  string tercomEnv;

  // data extracted from reference files
  vector<size_t> _reflengths;
  vector<multiset<int> > _reftokens;
  vector<vector<int> > m_references;
  vector<vector<vector<int> > > m_multi_references;
  string m_pid;

  // no copying allowed
  TerScorer(const TerScorer&);
  TerScorer& operator=(const TerScorer&);
};

#endif // __TERSCORER_H__
