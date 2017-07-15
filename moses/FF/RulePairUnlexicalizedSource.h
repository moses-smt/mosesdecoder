#pragma once

#include <string>
#include <limits>
#include <boost/unordered_map.hpp>
#include "StatelessFeatureFunction.h"
#include "moses/Factor.h"

namespace Moses
{

class RulePairUnlexicalizedSource : public StatelessFeatureFunction
{
public:

  RulePairUnlexicalizedSource(const std::string &line);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  void SetParameter(const std::string& key, const std::string& value);

  void EvaluateInIsolation(const Phrase &source
                           , const TargetPhrase &targetPhrase
                           , ScoreComponentCollection &scoreBreakdown
                           , ScoreComponentCollection &estimatedScores) const;

  void EvaluateWithSourceContext(const InputType &input
                                 , const InputPath &inputPath
                                 , const TargetPhrase &targetPhrase
                                 , const StackVec *stackVec
                                 , ScoreComponentCollection &scoreBreakdown
                                 , ScoreComponentCollection *estimatedScores = NULL) const
  {}

  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
      , const TranslationOptionList &translationOptionList) const
  {}

  void EvaluateWhenApplied(
    const Hypothesis& cur_hypo,
    ScoreComponentCollection* accumulator) const
  {}

  void EvaluateWhenApplied(
    const ChartHypothesis& cur_hypo,
    ScoreComponentCollection* accumulator) const
  {}

protected:

  bool m_glueRules;
  bool m_nonGlueRules;
  std::string m_glueTargetLHSStr;
  const Factor* m_glueTargetLHS;
};


}

