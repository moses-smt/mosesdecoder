#ifndef MERT_TER_SCORER_H_
#define MERT_TER_SCORER_H_

#include <set>
#include <string>
#include <vector>

#include "Types.h"
#include "StatisticsBasedScorer.h"

namespace MosesTuning
{


class ScoreStats;

/**
 * TER scoring
 */
class TerScorer: public StatisticsBasedScorer
{
public:
  explicit TerScorer(const std::string& config = "");
  ~TerScorer();

  virtual void setReferenceFiles(const std::vector<std::string>& referenceFiles);
  virtual void prepareStats(std::size_t sid, const std::string& text, ScoreStats& entry);

  virtual std::size_t NumberOfScores() const {
    // cerr << "TerScorer: " << (LENGTH + 1) << endl;
    return kLENGTH + 1;
  }

  virtual float calculateScore(const std::vector<ScoreStatsType>& comps) const;

private:
  const int kLENGTH;

  std::string m_java_env;
  std::string m_ter_com_env;

  // data extracted from reference files
  std::vector<std::size_t> m_ref_lengths;
  std::vector<std::multiset<int> > m_ref_tokens;
  std::vector<std::vector<int> > m_references;
  std::vector<std::vector<std::vector<int> > > m_multi_references;
  std::string m_pid;

  // no copying allowed
  TerScorer(const TerScorer&);
  TerScorer& operator=(const TerScorer&);
};

}

#endif // MERT_TER_SCORER_H_
