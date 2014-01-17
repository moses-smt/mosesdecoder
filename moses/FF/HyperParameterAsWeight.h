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

  virtual void Evaluate(const Hypothesis& hypo,
                        ScoreComponentCollection* accumulator) const
  {}

  /**
    * Same for chart-based features.
    **/
  virtual void EvaluateChart(const ChartHypothesis &hypo,
                             ScoreComponentCollection* accumulator) const
  {}

};

} // namespace



