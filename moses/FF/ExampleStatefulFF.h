#pragma once

#include <string>
#include "StatefulFeatureFunction.h"
#include "FFState.h"

namespace Moses
{

class ExampleState : public FFState
{
  int m_targetLen;
public:
  ExampleState(int targetLen)
    :m_targetLen(targetLen) {
  }

  virtual size_t hash() const {
    return (size_t) m_targetLen;
  }
  virtual bool operator==(const FFState& o) const {
    const ExampleState& other = static_cast<const ExampleState&>(o);
    return m_targetLen == other.m_targetLen;
  }

};

class ExampleStatefulFF : public StatefulFeatureFunction
{
public:
  ExampleStatefulFF(const std::string &line);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }
  virtual const FFState* EmptyHypothesisState(const InputType &input) const {
    return new ExampleState(0);
  }

  // An empty implementation of this function is provided by StatefulFeatureFunction.
  // Unless you are actually implementing this, please remove this declaration here
  // and the empty skeleton implementation from the corresponding .cpp
  // file to reduce code clutter.
  void
  EvaluateInIsolation(const Phrase &source
                      , const TargetPhrase &targetPhrase
                      , ScoreComponentCollection &scoreBreakdown
                      , ScoreComponentCollection &estimatedScores) const;

  // An empty implementation of this function is provided by StatefulFeatureFunction.
  // Unless you are actually implementing this, please remove this declaration here
  // and the empty skeleton implementation from the corresponding .cpp
  // file to reduce code clutter.
  void
  EvaluateWithSourceContext(const InputType &input
                            , const InputPath &inputPath
                            , const TargetPhrase &targetPhrase
                            , const StackVec *stackVec
                            , ScoreComponentCollection &scoreBreakdown
                            , ScoreComponentCollection *estimatedScores = NULL) const;

  // An empty implementation of this function is provided by StatefulFeatureFunction.
  // Unless you are actually implementing this, please remove this declaration here
  // and the empty skeleton implementation from the corresponding .cpp
  // file to reduce code clutter.
  void
  EvaluateTranslationOptionListWithSourceContext
  ( const InputType &input , const TranslationOptionList &translationOptionList) const;

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

