#ifndef __TERSCORER_H__
#define __TERSCORER_H__

#include <algorithm>
#include <cmath>
#include <iostream>
#include <iterator>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <limits.h>
#include "Types.h"
#include "ScoreData.h"
#include "Scorer.h"
#include "TERsrc/tercalc.h"
#include "TERsrc/terAlignment.h"

using namespace std;
using namespace TERCpp;

// enum TerReferenceLengthStrategy { TER_AVERAGE, TER_SHORTEST, TER_CLOSEST };


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

  virtual void whoami() {
    cerr << "I AM TerScorer" << std::endl;
  }
  size_t NumberOfScores() {
    // cerr << "TerScorer: " << (LENGTH + 1) << endl;
    return (kLENGTH + 1);
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
