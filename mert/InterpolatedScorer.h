#ifndef MERT_INTERPOLATED_SCORER_H_
#define MERT_INTERPOLATED_SCORER_H_

#include <string>
#include <vector>
#include "Types.h"
#include "ScoreData.h"
#include "Scorer.h"
#include "ScopedVector.h"

namespace MosesTuning
{


/**
  * Class that includes other scorers eg.
  * Interpolated HAMMING and BLEU scorer **/
class InterpolatedScorer : public Scorer
{
public:
  // name would be: "HAMMING,BLEU" or similar
  InterpolatedScorer(const std::string& name, const std::string& config);
  virtual ~InterpolatedScorer() {}

  virtual void score(const candidates_t& candidates, const diffs_t& diffs,
                     statscores_t& scores) const;

  virtual void setReferenceFiles(const std::vector<std::string>& referenceFiles);
  virtual void prepareStats(std::size_t sid, const std::string& text, ScoreStats& entry);

  virtual std::size_t NumberOfScores() const {
    std::size_t sz = 0;
    for (ScopedVector<Scorer>::const_iterator itsc = m_scorers.begin();
         itsc != m_scorers.end(); ++itsc) {
      sz += (*itsc)->NumberOfScores();
    }
    return sz;
  }

  virtual void setScoreData(ScoreData* data);

  virtual float calculateScore(const std::vector<ScoreStatsType>& totals) const;

  virtual float getReferenceLength(const std::vector<ScoreStatsType>& totals) const;

  /**
   * Set the factors, which should be used for this metric
   */
  virtual void setFactors(const std::string& factors);

  virtual void setFilter(const std::string& filterCommand);

  bool useAlignment() const;

protected:
  ScopedVector<Scorer> m_scorers;

  // Take the ownership of the heap-allocated the objects
  // by Scorer objects.
  ScopedVector<ScoreData> m_scorers_score_data;

  std::vector<float> m_scorer_weights;
};

}

#endif  // MERT_INTERPOLATED_SCORER_H_
