#pragma once

#include "StatelessFeatureFunction.h"

namespace Moses
{
class DecodeStep;

/**
  * Baseclass for phrase-table or generation table feature function
 **/
class HyperParameterAsWeight : public StatelessFeatureFunction
{
public:
  HyperParameterAsWeight(const std::string &line);

  virtual bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  virtual void EvaluateInIsolation(const Phrase &source
                                   , const TargetPhrase &targetPhrase
                                   , ScoreComponentCollection &scoreBreakdown
                                   , ScoreComponentCollection &estimatedFutureScore) const {
  }

  virtual void EvaluateWithSourceContext(const InputType &input
                                         , const InputPath &inputPath
                                         , const TargetPhrase &targetPhrase
                                         , const StackVec *stackVec
                                         , ScoreComponentCollection &scoreBreakdown
                                         , ScoreComponentCollection *estimatedFutureScore = NULL) const {
  }

  virtual void EvaluateTranslationOptionListWithSourceContext(const InputType &input
      , const TranslationOptionList &translationOptionList) const {
  }

  virtual void EvaluateWhenApplied(const Hypothesis& hypo,
                                   ScoreComponentCollection* accumulator) const {
  }

  /**
    * Same for chart-based features.
    **/
  virtual void EvaluateWhenApplied(const ChartHypothesis &hypo,
                                   ScoreComponentCollection* accumulator) const {
  }

};

} // namespace



