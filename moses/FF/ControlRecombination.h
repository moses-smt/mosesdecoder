#pragma once

#include <string>
#include "StatefulFeatureFunction.h"
#include "moses/FF/FFState.h"

namespace Moses
{

class ControlRecombinationState;

// force hypotheses NOT to recombine. For forced decoding
class ControlRecombination : public StatefulFeatureFunction
{
public:
  enum Type {
    None,
    Output,
    Segmentation
  };

  ControlRecombination(const std::string &line);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  virtual FFState* Evaluate(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const;

  virtual FFState* EvaluateChart(
    const ChartHypothesis& /* cur_hypo */,
    int /* featureID - used to index the state in the previous hypotheses */,
    ScoreComponentCollection* accumulator) const;

  void Evaluate(const InputType &input
                        , const InputPath &inputPath
                        , ScoreComponentCollection &scoreBreakdown) const
  {}
  void Evaluate(const Phrase &source
                        , const TargetPhrase &targetPhrase
                        , ScoreComponentCollection &scoreBreakdown
                        , ScoreComponentCollection &estimatedFutureScore) const
  {}

  //! return the state associated with the empty hypothesis for a given sentence
  virtual const FFState* EmptyHypothesisState(const InputType &input) const;

  void SetParameter(const std::string& key, const std::string& value);
protected:
  Type m_type;
};

class ControlRecombinationState : public FFState
{
protected:
  const Hypothesis *m_hypo;

public:
  ControlRecombinationState();
  ControlRecombinationState(const Hypothesis *hypo);
  int Compare(const FFState& other) const;

};

} // namespace
