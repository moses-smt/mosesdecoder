#pragma once

#include <string>
#include "StatefulFeatureFunction.h"
#include "FFState.h"
#include "lbl/nlm.h"

namespace Moses
{

class LBLLMState : public FFState
{
  int m_targetLen;
public:
  LBLLMState(int targetLen)
  {}

  int Compare(const FFState& other) const;
};

class LBLLM : public StatefulFeatureFunction
{
public:
  LBLLM(const std::string &line);
  void Load();

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }
  virtual const FFState* EmptyHypothesisState(const InputType &input) const {
    return new LBLLMState(0);
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

protected:
  std::string m_lmPath, m_refPath;

  oxlm::Dict dict;
  oxlm::FactoredOutputNLM lm;

};


}

