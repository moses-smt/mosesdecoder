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
class WordsRange;

/** Calculates Distortion scores
 */
class DistortionScoreProducer : public StatefulFeatureFunction
{
public:
  DistortionScoreProducer(const std::string &line)
    : StatefulFeatureFunction("Distortion", 1, line) {
  }

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  static float CalculateDistortionScore(const Hypothesis& hypo,
                                        const WordsRange &prev, const WordsRange &curr, const int FirstGapPosition);

  virtual const FFState* EmptyHypothesisState(const InputType &input) const;

  virtual FFState* Evaluate(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const;

  virtual FFState* EvaluateChart(
    const ChartHypothesis& /* cur_hypo */,
    int /* featureID - used to index the state in the previous hypotheses */,
    ScoreComponentCollection*) const {
    throw std::logic_error("DistortionScoreProducer not supported in chart decoder, yet");
  }
};
}

