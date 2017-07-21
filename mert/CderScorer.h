#ifndef MERT_CDER_SCORER_H_
#define MERT_CDER_SCORER_H_

#include <string>
#include <vector>
#include "Types.h"
#include "StatisticsBasedScorer.h"

namespace MosesTuning
{


/**
 * CderScorer class can compute both CDER and WER metric.
 */
class CderScorer: public StatisticsBasedScorer
{
public:
  explicit CderScorer(const std::string& config, bool allowed_long_jumps = true);
  ~CderScorer();

  virtual void setReferenceFiles(const std::vector<std::string>& referenceFiles);

  virtual void prepareStats(std::size_t sid, const std::string& text, ScoreStats& entry);

  virtual void prepareStatsVector(std::size_t sid, const std::string& text, std::vector<ScoreStatsType>& stats);

  virtual std::size_t NumberOfScores() const {
    return 2;
  }

  virtual float calculateScore(const std::vector<ScoreStatsType>& comps) const;

  virtual float getReferenceLength(const std::vector<ScoreStatsType>& totals) const {
    return totals[1];
  }

private:
  bool m_allowed_long_jumps;

  typedef std::vector<int> sent_t;
  std::vector<std::vector<sent_t> > m_ref_sentences;

  void computeCD(const sent_t& cand, const sent_t& ref,
                 std::vector<ScoreStatsType>& stats) const;

  // no copying allowed
  CderScorer(const CderScorer&);
  CderScorer& operator=(const CderScorer&);
};

}

#endif  // MERT_CDER_SCORER_H_
