//
//  SentenceLevelScorer.h
//  mert_lib
//
//  Created by Hieu Hoang on 22/06/2012.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//

#ifndef mert_lib_SentenceLevelScorer_h
#define mert_lib_SentenceLevelScorer_h

#include "Scorer.h"
#include <string>
#include <vector>

namespace MosesTuning
{

/**
 * Abstract base class for scorers that work by using sentence level
 * statistics (e.g., permutation distance metrics). **/
class SentenceLevelScorer : public Scorer
{
public:
  SentenceLevelScorer(const std::string& name, const std::string& config);
  ~SentenceLevelScorer();

  /** The sentence level scores have already been calculated, just need to average them
      and include the differences. Allows scores which are floats. **/
  virtual void score(const candidates_t& candidates, const diffs_t& diffs,
                     statscores_t& scores);

  // calculate the actual score *
  virtual statscore_t calculateScore(const std::vector<statscore_t>& totals) const {
    return 0;
  }

protected:
  // Set up regularisation parameters.
  void Init();

  //regularisation
  ScorerRegularisationStrategy m_regularisationStrategy;
  size_t m_regularisationWindow;
};

}
#endif
