#pragma once

#include <string>
#include <boost/unordered_set.hpp>
#include "StatelessFeatureFunction.h"

namespace Moses
{

class DeleteRules : public StatelessFeatureFunction
{
protected:
  std::string m_path;
  boost::unordered_set<size_t> m_ruleHashes;
public:
  DeleteRules(const std::string &line);

  void Load(AllOptions::ptr const& opts);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  void EvaluateInIsolation(const Phrase &source
                           , const TargetPhrase &targetPhrase
                           , ScoreComponentCollection &scoreBreakdown
                           , ScoreComponentCollection &estimatedScores) const;
  void EvaluateWithSourceContext(const InputType &input
                                 , const InputPath &inputPath
                                 , const TargetPhrase &targetPhrase
                                 , const StackVec *stackVec
                                 , ScoreComponentCollection &scoreBreakdown
                                 , ScoreComponentCollection *estimatedScores = NULL) const;

  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
      , const TranslationOptionList &translationOptionList) const;

  void EvaluateWhenApplied(const Hypothesis& hypo,
                           ScoreComponentCollection* accumulator) const;
  void EvaluateWhenApplied(const ChartHypothesis &hypo,
                           ScoreComponentCollection* accumulator) const;


  void SetParameter(const std::string& key, const std::string& value);

};

}

