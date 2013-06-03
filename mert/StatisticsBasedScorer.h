//
//  StatisticsBasedScorer.h
//  mert_lib
//
//  Created by Hieu Hoang on 23/06/2012.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//

#ifndef mert_lib_StatisticsBasedScorer_h
#define mert_lib_StatisticsBasedScorer_h

#include "Scorer.h"

namespace MosesTuning
{


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

} // namespace

#endif
