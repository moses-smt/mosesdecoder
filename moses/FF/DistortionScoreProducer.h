#pragma once

#include <stdexcept>
#include <string>
#include "StatefulFeatureFunction.h"

namespace Moses
{
class FFState;
class ScoreComponentCollection;
class Hypothesis;
class ChartHypothesis;
class Range;

/** Calculates Distortion scores
 */
class DistortionScoreProducer : public StatefulFeatureFunction
{
protected:
  static std::vector<const DistortionScoreProducer*> s_staticColl;

public:
  static const std::vector<const DistortionScoreProducer*>& GetDistortionFeatureFunctions() {
    return s_staticColl;
  }

  DistortionScoreProducer(const std::string &line);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  static float CalculateDistortionScore(const Hypothesis& hypo,
                                        const Range &prev, const Range &curr, const int FirstGapPosition);

  virtual const FFState* EmptyHypothesisState(const InputType &input) const;

  virtual FFState* EvaluateWhenApplied(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const;

  virtual FFState* EvaluateWhenApplied(
    const ChartHypothesis& /* cur_hypo */,
    int /* featureID - used to index the state in the previous hypotheses */,
    ScoreComponentCollection*) const {
    throw std::logic_error("DistortionScoreProducer not supported in chart decoder, yet");
  }

};
}

