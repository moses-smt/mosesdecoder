#ifndef __CDERSCORER_H__
#define __CDERSCORER_H__

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include "Types.h"
#include "ScoreData.h"
#include "Scorer.h"


using namespace std;


class CderScorer: public StatisticsBasedScorer
{
public:
  CderScorer(const string& config);
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

  size_t NumberOfScores() {
    return 2;
  };

  float calculateScore(const vector<int>& comps);

private:
  typedef vector<int> sent_t;
  vector<vector<sent_t> > ref_sentences;

  vector<int> computeCD(const sent_t& cand, const sent_t& ref);
  int distance(int word1, int word2)
  {
	  if (word1 == word2)
		  return 0;
	  else
		  return 1;
  }

  //no copy
  CderScorer(const CderScorer&);
  ~CderScorer() {};
  CderScorer& operator=(const CderScorer&);
};


#endif  // __CDERSCORER_H__
