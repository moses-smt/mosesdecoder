#ifndef __SCORER_H__
#define __SCORER_H__

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "Types.h"
#include "ScoreData.h"

using namespace std;

enum ScorerRegularisationStrategy {REG_NONE, REG_AVERAGE, REG_MINIMUM};

class ScoreStats;

/**
 * Superclass of all scorers and dummy implementation.
 *
 * In order to add a new scorer it should be sufficient to override the members
 * prepareStats(), setReferenceFiles() and score() (or calculateScore()).
 */
class Scorer
{
private:
  string _name;

public:
  Scorer(const string& name, const string& config);
  virtual ~Scorer() {}

  /**
   * Return the number of statistics needed for the computation of the score.
   */
  virtual size_t NumberOfScores() const {
    cerr << "Scorer: 0" << endl;
    return 0;
  }

  /**
   * Set the reference files. This must be called before prepareStats().
   */
  virtual void setReferenceFiles(const vector<string>& referenceFiles) {
    //do nothing
  }

  /**
   * Process the given guessed text, corresponding to the given reference sindex
   * and add the appropriate statistics to the entry.
   */
  virtual void prepareStats(size_t sindex, const string& text, ScoreStats& entry)
  {}

  virtual void prepareStats(const string& sindex, const string& text, ScoreStats& entry) {

//            cerr << sindex << endl;
    this->prepareStats((size_t) atoi(sindex.c_str()), text, entry);
    //cerr << text << std::endl;
  }

  /**
   * Score using each of the candidate index, then go through the diffs
   * applying each in turn, and calculating a new score each time.
   */
  virtual void score(const candidates_t& candidates, const diffs_t& diffs,
                     statscores_t& scores) const {
    //dummy impl
    if (!_scoreData) {
      throw runtime_error("score data not loaded");
    }
    scores.push_back(0);
    for (size_t i = 0; i < diffs.size(); ++i) {
      scores.push_back(0);
    }
  }

  /**
   * Calculate the score of the sentences corresponding to the list of candidate
   * indices. Each index indicates the 1-best choice from the n-best list.
   */
  float score(const candidates_t& candidates) const {
    diffs_t diffs;
    statscores_t scores;
    score(candidates, diffs, scores);
    return scores[0];
  }

  const string& getName() const {
    return _name;
  }

  size_t getReferenceSize() const {
    if (_scoreData) {
      return _scoreData->size();
    }
    return 0;
  }

  /**
   * Set the score data, prior to scoring.
   */
  void setScoreData(ScoreData* data) {
    _scoreData = data;
  }

protected:
  typedef map<string,int> encodings_t;
  typedef map<string,int>::iterator encodings_it;

  ScoreData* _scoreData;
  encodings_t _encodings;

  bool _preserveCase;

  /**
   * Get value of config variable. If not provided, return default.
   */
  string getConfig(const string& key, const string& def="") const {
    map<string,string>::const_iterator i = _config.find(key);
    if (i == _config.end()) {
      return def;
    } else {
      return i->second;
    }
  }


  /**
   * Tokenise line and encode.
   * Note: We assume that all tokens are separated by single spaces.
   */
  void encode(const string& line, vector<int>& encoded) {
    //cerr << line << endl;
    istringstream in (line);
    string token;
    while (in >> token) {
      if (!_preserveCase) {
        for (string::iterator i = token.begin(); i != token.end(); ++i) {
          *i = tolower(*i);
        }
      }
      encodings_it encoding = _encodings.find(token);
      int encoded_token;
      if (encoding == _encodings.end()) {
        encoded_token = (int)_encodings.size();
        _encodings[token] = encoded_token;
        //cerr << encoded_token << "(n) ";
      } else {
        encoded_token = encoding->second;
        //cerr << encoded_token << " ";
      }
      encoded.push_back(encoded_token);
    }
    //cerr << endl;
  }

private:
  map<string,string> _config;
};


/**
 * Abstract base class for Scorers that work by adding statistics across all
 * outout sentences, then apply some formula, e.g., BLEU, PER.
 */
class StatisticsBasedScorer : public Scorer
{
public:
  StatisticsBasedScorer(const string& name, const string& config);
  virtual ~StatisticsBasedScorer() {}
  virtual void score(const candidates_t& candidates, const diffs_t& diffs,
                     statscores_t& scores) const;

protected:
  /**
   * Calculate the actual score.
   */
  virtual statscore_t calculateScore(const vector<int>& totals) const = 0;

  // regularisation
  ScorerRegularisationStrategy _regularisationStrategy;
  size_t  _regularisationWindow;
};

#endif // __SCORER_H__
