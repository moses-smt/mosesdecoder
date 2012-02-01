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

class ScoreStats;

/**
 * Superclass of all scorers and dummy implementation.
 *
 * In order to add a new scorer it should be sufficient to override the members
 * prepareStats(), setReferenceFiles() and score() (or calculateScore()).
 */
class Scorer
{
 public:
  Scorer(const string& name, const string& config);
  virtual ~Scorer();

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
    // do nothing
  }

  /**
   * Process the given guessed text, corresponding to the given reference sindex
   * and add the appropriate statistics to the entry.
   */
  virtual void prepareStats(size_t sindex, const string& text, ScoreStats& entry) {
    // do nothing.
  }

  virtual void prepareStats(const string& sindex, const string& text, ScoreStats& entry) {
    this->prepareStats(static_cast<size_t>(atoi(sindex.c_str())), text, entry);
  }

  /**
   * Score using each of the candidate index, then go through the diffs
   * applying each in turn, and calculating a new score each time.
   */
  virtual void score(const candidates_t& candidates, const diffs_t& diffs,
                     statscores_t& scores) const {
    //dummy impl
    if (!m_score_data) {
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
    return m_name;
  }

  size_t getReferenceSize() const {
    if (m_score_data) {
      return m_score_data->size();
    }
    return 0;
  }

  /**
   * Set the score data, prior to scoring.
   */
  void setScoreData(ScoreData* data) {
    m_score_data = data;
  }

 private:
  class Encoder {
   public:
    Encoder();
    virtual ~Encoder();
    int Encode(const std::string& token);
    void Clear() { m_vocab.clear(); }

   private:
    std::map<std::string, int> m_vocab;
  };

  void InitConfig(const string& config);

  string m_name;
  Encoder* m_encoder;
  map<string, string> m_config;

 protected:
  ScoreData* m_score_data;
  bool m_enable_preserve_case;

  /**
   * Get value of config variable. If not provided, return default.
   */
  string getConfig(const string& key, const string& def="") const {
    map<string,string>::const_iterator i = m_config.find(key);
    if (i == m_config.end()) {
      return def;
    } else {
      return i->second;
    }
  }

  /**
   * Tokenise line and encode.
   * Note: We assume that all tokens are separated by single spaces.
   */
  void TokenizeAndEncode(const string& line, vector<int>& encoded);

  void ClearEncoder() { m_encoder->Clear(); }
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

  enum RegularisationType {
    NONE,
    AVERAGE,
    MINIMUM,
  };

  /**
   * Calculate the actual score.
   */
  virtual statscore_t calculateScore(const vector<int>& totals) const = 0;

  // regularisation
  RegularisationType m_regularization_type;
  size_t  m_regularization_window;
};

#endif // __SCORER_H__
