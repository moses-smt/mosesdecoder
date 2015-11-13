#pragma once
#include <string>
#include "StatelessFeatureFunction.h"

namespace Moses
{

// Rule Scope - not quite completely implemented yet
class SpanLength : public StatelessFeatureFunction
{
public:
  SpanLength(const std::string &line);

  virtual bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  virtual void EvaluateInIsolation(const Phrase &source
                                   , const TargetPhrase &targetPhrase
                                   , ScoreComponentCollection &scoreBreakdown
                                   , ScoreComponentCollection &estimatedScores) const;

  virtual void EvaluateWithSourceContext(const InputType &input
                                         , const InputPath &inputPath
                                         , const TargetPhrase &targetPhrase
                                         , const StackVec *stackVec
                                         , ScoreComponentCollection &scoreBreakdown
                                         , ScoreComponentCollection *estimatedScores = NULL) const;

  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
      , const TranslationOptionList &translationOptionList) const {
  }


  virtual void EvaluateWhenApplied(const Hypothesis& hypo,
                                   ScoreComponentCollection* accumulator) const {
  }

  virtual void EvaluateWhenApplied(const ChartHypothesis &hypo,
                                   ScoreComponentCollection* accumulator) const {
  }

  void SetParameter(const std::string& key, const std::string& value);

protected:
  enum SmoothingMethod {
    None,
    PlusConst,
  };
  SmoothingMethod m_smoothingMethod;

  float m_const;
};

}

