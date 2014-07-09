#pragma once

#include "StatelessFeatureFunction.h"

namespace Moses
{

class PhrasePenalty : public StatelessFeatureFunction
{
public:
  PhrasePenalty(const std::string &line);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  virtual void EvaluateInIsolation(const Phrase &source
                        , const TargetPhrase &targetPhrase
                        , ScoreComponentCollection &scoreBreakdown
                        , ScoreComponentCollection &estimatedFutureScore) const;

  void EvaluateWhenApplied(const Hypothesis& hypo,
                ScoreComponentCollection* accumulator) const
  {}
  void EvaluateWhenApplied(const ChartHypothesis &hypo,
                     ScoreComponentCollection* accumulator) const
  {}

  void EvaluateWithSourceContext(const InputType &input
                , const InputPath &inputPath
                , const TargetPhrase &targetPhrase
                , const StackVec *stackVec
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection *estimatedFutureScore = NULL) const
  {}

};

} //namespace

