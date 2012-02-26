#ifndef __INTERPOLATED_SCORER_H__
#define __INTERPOLATED_SCORER_H__

#include <algorithm>
#include <cmath>
#include <iostream>
#include <iterator>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "Types.h"
#include "ScoreData.h"
#include "Scorer.h"

/**
  * Class that includes other scorers eg.
  * Interpolated HAMMING and BLEU scorer **/
class InterpolatedScorer : public Scorer
{

public:
  // name would be: "HAMMING,BLEU" or similar
  InterpolatedScorer(const string& name, const string& config);
  virtual ~InterpolatedScorer() {};
  virtual void score(const candidates_t& candidates, const diffs_t& diffs,
             statscores_t& scores) const;

  void setReferenceFiles(const vector<string>& referenceFiles);
  void prepareStats(size_t sid, const string& text, ScoreStats& entry);
  virtual size_t NumberOfScores() const {
    size_t sz=0;
    for (vector<Scorer*>::const_iterator itsc =  _scorers.begin(); itsc < _scorers.end(); itsc++) {
      sz += (*itsc)->NumberOfScores();
    }
    return sz;
  };

  virtual void setScoreData(ScoreData* data);

protected:
  vector<Scorer*> _scorers;
  vector<float> _scorerWeights;
};

#endif //__INTERPOLATED_SCORER_H
