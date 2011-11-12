#ifndef __MERGESCORER_H__
#define __MERGESCORER_H__

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
#include "TerScorer.h"
#include "BleuScorer.h"
#include "PerScorer.h"
#include "CderScorer.h"
//#include "TERsrc/tercalc.h"
//#include "TERsrc/terAlignment.h"

using namespace std;
using namespace TERCpp;

// enum MergeReferenceLengthStrategy { MERGE_AVERAGE, MERGE_SHORTEST, MERGE_CLOSEST };


/**
 * Merge scoring.
 */
class MergeScorer: public StatisticsBasedScorer {
public:
  explicit MergeScorer(const string& config = "");
  ~MergeScorer();

  virtual void setReferenceFiles(const vector<string>& referenceFiles);
  virtual void prepareStats(size_t sid, const string& text, ScoreStats& entry);
  virtual void whoami() const
  {
    cerr << "I AM MergeScorer" << std::endl;
  }

protected:
  friend class PerScorer;
  float calculateScore(const vector<int>& comps);

 private:
  const int kLENGTH;

  string javaEnv;
  string tercomEnv;

  // data extracted from reference files
  vector<size_t> _reflengths;
  vector<multiset<int> > _reftokens;
  vector<vector<int> > m_references;
  string m_pid;

  // no copying allowed
  MergeScorer(const MergeScorer&);
  MergeScorer& operator=(const MergeScorer&);
};

#endif  //__TERSCORER_H
