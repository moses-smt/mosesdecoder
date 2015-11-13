#ifndef MERT_HWCM_SCORER_H_
#define MERT_HWCM_SCORER_H_

#include <string>
#include <vector>

#include "StatisticsBasedScorer.h"
#include "InternalTree.h"

namespace MosesTuning
{


class ScoreStats;
const size_t kHwcmOrder = 4;

/**
 * HWCM scoring (Liu and Gildea 2005), but F1 instead of precision.
 */
class HwcmScorer: public StatisticsBasedScorer
{
public:
  explicit HwcmScorer(const std::string& config = "");
  ~HwcmScorer();

  virtual void setReferenceFiles(const std::vector<std::string>& referenceFiles);
  virtual void prepareStats(std::size_t sid, const std::string& text, ScoreStats& entry);

  virtual std::size_t NumberOfScores() const {
    return kHwcmOrder*3;
  }

  virtual float calculateScore(const std::vector<ScoreStatsType>& comps) const;

  virtual float getReferenceLength(const std::vector<ScoreStatsType>& totals) const {
    return totals[2];
  }

  //TODO: actually, we use trees which we store in place of alignment. Maybe use something analogous to Phrase Properties to cleanly store trees?
  bool useAlignment() const {
    return true;
  }

private:

  // data extracted from reference files
  std::vector<TreePointer> m_ref_trees;
  std::vector<std::vector<std::map<std::string, int> > > m_ref_hwc;
  std::vector<std::vector<int> > m_ref_lengths;

  void extractHeadWordChain(TreePointer tree, std::vector<std::string> & history, std::vector<std::map<std::string, int> > & hwc);
  std::string getHead(TreePointer tree);

  // no copying allowed
  HwcmScorer(const HwcmScorer&);
  HwcmScorer& operator=(const HwcmScorer&);
};

}

#endif // MERT_HWCM_SCORER_H_