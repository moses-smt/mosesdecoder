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
  * Abstract base class for scorers that include other scorers eg.
  * Interpolated HAMMING and BLEU scorer **/
class InterpolatedScorer : public Scorer
{

public:
  // name would be: "HAMMING,BLEU" or similar
  InterpolatedScorer(const string& name, const string& config);
  ~InterpolatedScorer() {};
  void score(const candidates_t& candidates, const diffs_t& diffs,
             statscores_t& scores);

  void setReferenceFiles(const vector<string>& referenceFiles);
  void prepareStats(size_t sid, const string& text, ScoreStats& entry);
  size_t NumberOfScores() const {
    size_t sz=0;
    for (vector<Scorer*>::const_iterator itsc =  _scorers.begin(); itsc < _scorers.end(); itsc++) {
      sz += (*itsc)->NumberOfScores();
    }
    return sz;
  };

  bool useAlignment() const {
    //cout << "InterpolatedScorer::useAlignment" << endl;
    for (vector<Scorer*>::const_iterator itsc =  _scorers.begin(); itsc < _scorers.end(); itsc++) {
      if ((*itsc)->useAlignment()) {
        //cout <<"InterpolatedScorer::useAlignment Returning true"<<endl;
        return true;
      }
    }
    return false;
  };

  //calculate the actual score - this gets done in the individual scorers
  //statscore_t calculateScore(const vector<statscore_t>& totals);
  void setScoreData(ScoreData* data);

protected:

  //regularisation
  ScorerRegularisationStrategy _regularisationStrategy;
  size_t  _regularisationWindow;

  vector<Scorer*> _scorers;
  vector<float> _scorerWeights;

};

#endif //__INTERPOLATED_SCORER_H
