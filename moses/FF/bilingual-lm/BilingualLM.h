#pragma once

#include <string>
#include "moses/FF/StatefulFeatureFunction.h"
#include "moses/FF/FFState.h"

namespace Moses
{

class BilingualLMState : public FFState
{
  int m_targetLen;
public:
  BilingualLMState(int targetLen)
    :m_targetLen(targetLen)
  {}

  int Compare(const FFState& other) const;
};

class BilingualLM : public StatefulFeatureFunction
{
public:
  BilingualLM(const std::string &line);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }
  virtual const FFState* EmptyHypothesisState(const InputType &input) const {
    return new BilingualLMState(0);
  }

  void EvaluateInIsolation(const Phrase &source
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection &estimatedFutureScore) const;
  void EvaluateWithSourceContext(const InputType &input
                , const InputPath &inputPath
                , const TargetPhrase &targetPhrase
                , const StackVec *stackVec
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection *estimatedFutureScore = NULL) const;
  FFState* EvaluateWhenApplied(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const;
  FFState* EvaluateWhenApplied(
    const ChartHypothesis& /* cur_hypo */,
    int /* featureID - used to index the state in the previous hypotheses */,
    ScoreComponentCollection* accumulator) const;

  void SetParameter(const std::string& key, const std::string& value);

};


}

