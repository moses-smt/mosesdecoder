#ifndef MERT_SCORER_H_
#define MERT_SCORER_H_

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "Types.h"
#include "ScoreData.h"

class PreProcessFilter;
class ScoreStats;

namespace mert {

class Vocabulary;

} // namespace mert

/**
 * Superclass of all scorers and dummy implementation.
 *
 * In order to add a new scorer it should be sufficient to override the members
 * prepareStats(), setReferenceFiles() and score() (or calculateScore()).
 */
class Scorer
{
 public:
  Scorer(const std::string& name, const std::string& config);
  virtual ~Scorer();

  /**
   * Return the number of statistics needed for the computation of the score.
   */
  virtual std::size_t NumberOfScores() const = 0;

  /**
   * Set the reference files. This must be called before prepareStats().
   */
  virtual void setReferenceFiles(const std::vector<std::string>& referenceFiles) {
    // do nothing
  }

  /**
   * Process the given guessed text, corresponding to the given reference sindex
   * and add the appropriate statistics to the entry.
   */
  virtual void prepareStats(std::size_t sindex, const std::string& text, ScoreStats& entry) {
    // do nothing.
  }

  virtual void prepareStats(const std::string& sindex, const std::string& text, ScoreStats& entry) {
    this->prepareStats(static_cast<std::size_t>(atoi(sindex.c_str())), text, entry);
  }

  /**
   * Score using each of the candidate index, then go through the diffs
   * applying each in turn, and calculating a new score each time.
   */
  virtual void score(const candidates_t& candidates, const diffs_t& diffs,
                     statscores_t& scores) const = 0;
  /*
  {
    //dummy impl
    if (!m_score_data) {
      throw runtime_error("score data not loaded");
    }
    scores.push_back(0);
    for (std::size_t i = 0; i < diffs.size(); ++i) {
      scores.push_back(0);
    }
  }
  */

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

  const std::string& getName() const {
    return m_name;
  }

  std::size_t getReferenceSize() const {
    if (m_score_data) {
      return m_score_data->size();
    }
    return 0;
  }

  /**
   * Set the score data, prior to scoring.
   */
  virtual void setScoreData(ScoreData* data) {
    m_score_data = data;
  }

  /**
   * Set the factors, which should be used for this metric
   */
  virtual void setFactors(const std::string& factors);

  mert::Vocabulary* GetVocab() const { return m_vocab; }

  /**
   * Set unix filter, which will be used to preprocess the sentences
   */
  virtual void setFilter(const std::string& filterCommand);

 private:
  void InitConfig(const std::string& config);

  /**
   * Take the factored sentence and return the desired factors
   */
  std::string applyFactors(const std::string& sentece) const;

  /**
   * Preprocess the sentence with the filter (if given)
   */
  std::string applyFilter(const std::string& sentence) const;

  std::string m_name;
  mert::Vocabulary* m_vocab;
  std::map<std::string, std::string> m_config;
  std::vector<int> m_factors;
  PreProcessFilter* m_filter;

 protected:
  ScoreData* m_score_data;
  bool m_enable_preserve_case;

  /**
   * Get value of config variable. If not provided, return default.
   */
  std::string getConfig(const std::string& key, const std::string& def="") const {
    std::map<std::string,std::string>::const_iterator i = m_config.find(key);
    if (i == m_config.end()) {
      return def;
    } else {
      return i->second;
    }
  }

  /**
   * Tokenise line and encode.
   * Note: We assume that all tokens are separated by whitespaces.
   */
  void TokenizeAndEncode(const std::string& line, std::vector<int>& encoded);

  /**
   * Every inherited scorer should call this function for each sentence
   */
  std::string preprocessSentence(const std::string& sentence) const
  {
    return applyFactors(applyFilter(sentence));
  }

};

/**
 * Abstract base class for Scorers that work by adding statistics across all
 * outout sentences, then apply some formula, e.g., BLEU, PER.
 */
class StatisticsBasedScorer : public Scorer
{
 public:
  StatisticsBasedScorer(const std::string& name, const std::string& config);
  virtual ~StatisticsBasedScorer() {}
  virtual void score(const candidates_t& candidates, const diffs_t& diffs,
                     statscores_t& scores) const;

 protected:

  enum RegularisationType {
    NONE,
    AVERAGE,
    MINIMUM
  };

  /**
   * Calculate the actual score.
   */
  virtual statscore_t calculateScore(const std::vector<int>& totals) const = 0;

  // regularisation
  RegularisationType m_regularization_type;
  std::size_t  m_regularization_window;
};

#endif // MERT_SCORER_H_
