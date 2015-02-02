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
                                        const WordsRange &prev, const WordsRange &curr, const int FirstGapPosition);

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

  void EvaluateWithSourceContext(const InputType &input
                                 , const InputPath &inputPath
                                 , const TargetPhrase &targetPhrase
                                 , const StackVec *stackVec
                                 , ScoreComponentCollection &scoreBreakdown
                                 , ScoreComponentCollection *estimatedFutureScore = NULL) const {
  }

  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
      , const TranslationOptionList &translationOptionList) const {
  }

  void EvaluateInIsolation(const Phrase &source
                           , const TargetPhrase &targetPhrase
                           , ScoreComponentCollection &scoreBreakdown
                           , ScoreComponentCollection &estimatedFutureScore) const {
  }
};
}

