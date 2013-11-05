#pragma once

#include <stdexcept>
#include <string>
#include "StatefulFeatureFunction.h"
#include "WordsRange.h"

/** Calculates Distortion scores
 */
class DistortionScoreProducer : public StatefulFeatureFunction
{
public:
  DistortionScoreProducer(const std::string &line);

  virtual void Evaluate(const Phrase &source
                        , const TargetPhrase &targetPhrase
                        , Scores &scores
                        , Scores &estimatedFutureScore) const {
  }

  size_t Evaluate(
    const Hypothesis& hypo,
    size_t prevState,
    Scores &scores) const;

protected:
  SCORE ComputeDistortionScore(const WordsRange &prev, const WordsRange &curr) const;

  SCORE CalculateDistortionScore_MooreAndQuick(const Hypothesis& hypo,
      const WordsRange &prevRange,
      const WordsRange &currRange,
      int firstGap);

};

