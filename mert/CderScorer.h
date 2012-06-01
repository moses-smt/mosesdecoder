#ifndef __CDERSCORER_H__
#define __CDERSCORER_H__

#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include "Types.h"
#include "Scorer.h"

using namespace std;

class CderScorer: public StatisticsBasedScorer
{
public:
  explicit CderScorer(const string& config);
  ~CderScorer();

  virtual void setReferenceFiles(const vector<string>& referenceFiles);
  virtual void prepareStats(size_t sid, const string& text, ScoreStats& entry)
  {
    vector<int> stats;
    prepareStatsVector(sid, text, stats);

    stringstream sout;
    copy(stats.begin(),stats.end(),ostream_iterator<float>(sout," "));
    string stats_str = sout.str();
    entry.set(stats_str);
  }
  virtual void prepareStatsVector(size_t sid, const string& text, vector<int>& stats);

  virtual size_t NumberOfScores() const {
    return 2;
  }

  virtual float calculateScore(const vector<int>& comps) const;

private:
  typedef vector<int> sent_t;
  vector<vector<sent_t> > ref_sentences;

  vector<int> computeCD(const sent_t& cand, const sent_t& ref) const;
  int distance(int word1, int word2) const
  {
    return word1 == word2 ? 0 : 1;
  }

  // no copying allowed
  CderScorer(const CderScorer&);
  CderScorer& operator=(const CderScorer&);
};

#endif  // __CDERSCORER_H__
