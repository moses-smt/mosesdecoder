#pragma once

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <limits>
#include "Types.h"
#include "ScoreData.h"

namespace mert
{

class Vocabulary;

} // namespace mert

namespace MosesTuning
{

class PreProcessFilter;
class ScoreStats;

enum ScorerRegularisationStrategy {REG_NONE, REG_AVERAGE, REG_MINIMUM};

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
   * Calculate score based on a vector of sufficient statistics.
   */
  virtual float calculateScore(const std::vector<ScoreStatsType>& totals) const = 0;

  float calculateSentenceLevelBackgroundScore(const std::vector<ScoreStatsType>& totals, const std::vector<ScoreStatsType>& bg) {
	if (m_name.compare("CHRF")!=0){
		std::vector<ScoreStatsType> stats(totals.size());
   for(size_t i=0; i<stats.size(); i++)
       stats[i] = totals[i]+bg[i];
   // Get score and scale by reference length (as per Chiang et al 08)
   return calculateScore(stats) * getReferenceLength(stats);
	}
	else{
		return calculateScore(totals);
	}
  }

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
  float score(const candidates_t& candidates) const;

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
   * Based on vector of sufficient statistics, return length of reference.
   */
  virtual float getReferenceLength(const std::vector<ScoreStatsType>& totals) const = 0;

  /**
   * Set the score data, prior to scoring.
   */
  virtual void setScoreData(ScoreData* data) {
    m_score_data = data;
  }

  /**
   * The scorer returns if it uses the reference alignment data
   * for permutation distance scores
   **/
  virtual bool useAlignment() const {
    //cout << "Scorer::useAlignment returning false " << endl;
    return false;
  };

  /**
   * Set the factors, which should be used for this metric
   */
  virtual void setFactors(const std::string& factors);

  mert::Vocabulary* GetVocab() const {
    return m_vocab;
  }

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

#if defined(__GLIBCXX__) || defined(__GLIBCPP__)
  PreProcessFilter* m_filter;
#endif

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
  void TokenizeAndEncode(const std::string& line, std::vector<int>& encoded) const;

  /*
   * Tokenize functions for testing only.
   */
  void TokenizeAndEncodeTesting(const std::string& line, std::vector<int>& encoded) const;

  /**
   * Every inherited scorer should call this function for each sentence
   */
  std::string preprocessSentence(const std::string& sentence) const {
    return applyFactors(applyFilter(sentence));
  }

};

namespace
{

//regularisation strategies
inline float score_min(const statscores_t& scores, size_t start, size_t end)
{
  float min = std::numeric_limits<float>::max();
  for (size_t i = start; i < end; ++i) {
    if (scores[i] < min) {
      min = scores[i];
    }
  }
  return min;
}

inline float score_average(const statscores_t& scores, size_t start, size_t end)
{
  if ((end - start) < 1) {
    // this shouldn't happen
    return 0;
  }
  float total = 0;
  for (size_t j = start; j < end; ++j) {
    total += scores[j];
  }

  return total / (end - start);
}

} // namespace

}

