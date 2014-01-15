#pragma once

#include <string>
#include "StatelessFeatureFunction.h"

namespace Moses
{

class SkeletonStatelessFF : public StatelessFeatureFunction
{
public:
  SkeletonStatelessFF(const std::string &line)
    :StatelessFeatureFunction(2, line)
  {}

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  void Evaluate(const Phrase &source
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection &estimatedFutureScore) const;
  void Evaluate(const InputType &input
                , const InputPath &inputPath
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection *estimatedFutureScore = NULL) const;
  void Evaluate(const Hypothesis& hypo,
                ScoreComponentCollection* accumulator) const;
  void EvaluateChart(const ChartHypothesis &hypo,
                     ScoreComponentCollection* accumulator) const;

};

}

