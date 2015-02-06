#pragma once

#include "StatelessFeatureFunction.h"

namespace Moses
{

class CountNonTerms : public StatelessFeatureFunction
{
public:
  CountNonTerms(const std::string &line);
  bool IsUseable(const FactorMask &mask) const {
    return true;
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
                                 , ScoreComponentCollection *estimatedFutureScore = NULL) const {
  }

  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
      , const TranslationOptionList &translationOptionList) const {
  }

  void EvaluateWhenApplied(const Hypothesis& hypo,
                           ScoreComponentCollection* accumulator) const {
  }

  void EvaluateWhenApplied(
    const ChartHypothesis& hypo,
    ScoreComponentCollection* accumulator) const {
  }

  void SetParameter(const std::string& key, const std::string& value);

protected:
  bool m_all, m_sourceSyntax, m_targetSyntax;
};

}

