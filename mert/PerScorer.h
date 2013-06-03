#ifndef MERT_PER_SCORER_H_
#define MERT_PER_SCORER_H_

#include <set>
#include <string>
#include <vector>
#include "Types.h"
#include "StatisticsBasedScorer.h"

namespace MosesTuning
{


class ScoreStats;

/**
 * An implementation of position-independent word error rate.
 * This is defined as
 *   1 - (correct - max(0,output_length - ref_length)) / ref_length
 * In fact, we ignore the " 1 - " so that it can be maximised.
 */
class PerScorer: public StatisticsBasedScorer
{
public:
  explicit PerScorer(const std::string& config = "");
  ~PerScorer();

  virtual void setReferenceFiles(const std::vector<std::string>& referenceFiles);
  virtual void prepareStats(std::size_t sid, const std::string& text, ScoreStats& entry);
  virtual std::size_t NumberOfScores() const {
    return 3;
  }
  virtual float calculateScore(const std::vector<int>& comps) const;

private:
  // no copying allowed
  PerScorer(const PerScorer&);
  PerScorer& operator=(const PerScorer&);

  // data extracted from reference files
  std::vector<std::size_t> m_ref_lengths;
  std::vector<std::multiset<int> > m_ref_tokens;
};

}

#endif  // MERT_PER_SCORER_H_
