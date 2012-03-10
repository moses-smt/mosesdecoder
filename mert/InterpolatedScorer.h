#ifndef MERT_INTERPOLATED_SCORER_H_
#define MERT_INTERPOLATED_SCORER_H_

#include <string>
#include <vector>
#include "Types.h"
#include "ScoreData.h"
#include "Scorer.h"
#include "ScopedVector.h"

/**
  * Class that includes other scorers eg.
  * Interpolated HAMMING and BLEU scorer **/
class InterpolatedScorer : public Scorer
{
public:
  // name would be: "HAMMING,BLEU" or similar
  InterpolatedScorer(const string& name, const string& config);
  virtual ~InterpolatedScorer() {}

  virtual void score(const candidates_t& candidates, const diffs_t& diffs,
                     statscores_t& scores) const;

  virtual void setReferenceFiles(const vector<string>& referenceFiles);
  virtual void prepareStats(size_t sid, const string& text, ScoreStats& entry);

  virtual size_t NumberOfScores() const {
    size_t sz = 0;
    for (ScopedVector<Scorer>::const_iterator itsc = m_scorers.begin();
         itsc != m_scorers.end(); ++itsc) {
      sz += (*itsc)->NumberOfScores();
    }
    return sz;
  }

  virtual void setScoreData(ScoreData* data);

  /**
   * Set the factors, which should be used for this metric
   */
  virtual void setFactors(const string& factors);

protected:
  ScopedVector<Scorer> m_scorers;

  // Take the ownership of the heap-allocated the objects
  // by Scorer objects.
  ScopedVector<ScoreData> m_scorers_score_data;

  vector<float> m_scorer_weights;
};

#endif  // MERT_INTERPOLATED_SCORER_H_
