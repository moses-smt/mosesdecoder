#pragma once

#include <string>
#include "StatefulFeatureFunction.h"
#include "FFState.h"

namespace Moses
{

class SkeletonState : public FFState
{
  int m_targetLen;
public:
  SkeletonState(int targetLen)
    :m_targetLen(targetLen)
  {}

  int Compare(const FFState& other) const;
};

class SkeletonStatefulFF : public StatefulFeatureFunction
{
public:
  SkeletonStatefulFF(const std::string &line)
    :StatefulFeatureFunction(3, line)
  {}

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }
  virtual const FFState* EmptyHypothesisState(const InputType &input) const {
    return new SkeletonState(0);
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
  FFState* Evaluate(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const;
  FFState* EvaluateChart(
    const ChartHypothesis& /* cur_hypo */,
    int /* featureID - used to index the state in the previous hypotheses */,
    ScoreComponentCollection* accumulator) const;

};


}

