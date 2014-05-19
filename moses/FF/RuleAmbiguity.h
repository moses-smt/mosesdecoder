#pragma once
#include <string>
#include "StatelessFeatureFunction.h"

namespace Moses
{

// similar to Scope, however, adjacent non-term count as 1 ammbiguity, rather than 2
class RuleAmbiguity : public StatelessFeatureFunction
{
public:
  RuleAmbiguity(const std::string &line);

  virtual bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  virtual void Evaluate(const Phrase &source
                        , const TargetPhrase &targetPhrase
                        , ScoreComponentCollection &scoreBreakdown
                        , ScoreComponentCollection &estimatedFutureScore) const;

  virtual void Evaluate(const InputType &input
                        , const InputPath &inputPath
                        , const TargetPhrase &targetPhrase
                        , ScoreComponentCollection &scoreBreakdown
                        , ScoreComponentCollection *estimatedFutureScore = NULL) const
  {}

  virtual void Evaluate(const Hypothesis& hypo,
                        ScoreComponentCollection* accumulator) const
  {}

  virtual void EvaluateChart(const ChartHypothesis &hypo,
                             ScoreComponentCollection* accumulator) const
  {}

  void SetParameter(const std::string& key, const std::string& value);

protected:
  bool m_sourceSyntax;
};

}

