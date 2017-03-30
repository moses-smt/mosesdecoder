#ifndef MERT_QE_SCORER_H_
#define MERT_QE_SCORER_H_

#include <string>
#include <vector>
#include <functional>

#include "Types.h"
#include "Util.h"
#include "StatisticsBasedScorer.h"
#include "M2.h"

namespace MosesTuning
{

class QEScorer: public StatisticsBasedScorer
{
public:
  explicit QEScorer(const std::string& config);

  virtual void setReferenceFiles(const std::vector<std::string>& referenceFiles);
  virtual void prepareStats(std::size_t sid, const std::string& text, ScoreStats& entry);

  virtual std::size_t NumberOfScores() const {
    return 4;
  }

  virtual float calculateScore(const std::vector<ScoreStatsType>& comps) const;
  virtual float getReferenceLength(const std::vector<ScoreStatsType>& comps) const { return 0; };

private:
  // no copying allowed
  QEScorer(const QEScorer&);
  QEScorer& operator=(const QEScorer&);
  bool bad_;
};

}
#endif  // MERT_QE_SCORER_H_
