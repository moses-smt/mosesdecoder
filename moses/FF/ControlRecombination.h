#pragma once

#include <string>
#include <map>
#include "StatefulFeatureFunction.h"
#include "FFState.h"
#include "moses/Phrase.h"

namespace Moses
{
enum ControlRecombinationType {
  // when to recombine
  SameOutput = 1,
  Never = 2
};

class ControlRecombination;

class ControlRecombinationState : public FFState
{
public:
  ControlRecombinationState(const ControlRecombination &ff)
    :m_ff(ff) {
  }

  ControlRecombinationState(const Hypothesis &hypo, const ControlRecombination &ff);
  ControlRecombinationState(const ChartHypothesis &hypo, const ControlRecombination &ff);

  int Compare(const FFState& other) const;

  const Phrase &GetPhrase() const {
    return m_outputPhrase;
  }

protected:
  Phrase m_outputPhrase;
  const ControlRecombination &m_ff;
  const void *m_hypo;
};

//////////////////////////////////////////////////////////////////

// only allow recombination for the same output
class ControlRecombination : public StatefulFeatureFunction
{
public:
  ControlRecombination(const std::string &line)
    :StatefulFeatureFunction(0, line)
    ,m_type(SameOutput)

  {
    m_tuneable = false;
    ReadParameters();
  }

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  void EvaluateInIsolation(const Phrase &source
                           , const TargetPhrase &targetPhrase
                           , ScoreComponentCollection &scoreBreakdown
                           , ScoreComponentCollection &estimatedFutureScore) const {
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

  FFState* EvaluateWhenApplied(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const;

  FFState* EvaluateWhenApplied(
    const ChartHypothesis& /* cur_hypo */,
    int /* featureID - used to index the state in the previous hypotheses */,
    ScoreComponentCollection* accumulator) const;

  virtual const FFState* EmptyHypothesisState(const InputType &input) const {
    return new ControlRecombinationState(*this);
  }

  std::vector<float> DefaultWeights() const;

  void SetParameter(const std::string& key, const std::string& value);

  ControlRecombinationType GetType() const {
    return m_type;
  }
protected:
  ControlRecombinationType m_type;
};


}

