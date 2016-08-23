#pragma once

#include "StatelessFeatureFunction.h"
#include "moses/parameters/AllOptions.h"

namespace Moses
{

// assumes that source-side syntax labels are stored in the target non-terminal field of the rules
class SourceGHKMTreeInputMatchFeature : public StatelessFeatureFunction
{
public:
  SourceGHKMTreeInputMatchFeature(const std::string &line);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  void SetParameter(const std::string& key, const std::string& value);

  void EvaluateInIsolation(const Phrase &source
                           , const TargetPhrase &targetPhrase
                           , ScoreComponentCollection &scoreBreakdown
                           , ScoreComponentCollection &estimatedScores) const {};

  void EvaluateWithSourceContext(const InputType &input
                                 , const InputPath &inputPath
                                 , const TargetPhrase &targetPhrase
                                 , const StackVec *stackVec
                                 , ScoreComponentCollection &scoreBreakdown
                                 , ScoreComponentCollection *estimatedScores = NULL) const;

  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
      , const TranslationOptionList &translationOptionList) const {
  }


  void EvaluateWhenApplied(const Hypothesis& hypo,
                           ScoreComponentCollection* accumulator) const {};

  void EvaluateWhenApplied(const ChartHypothesis &hypo,
                           ScoreComponentCollection* accumulator) const {};

  void Load(AllOptions::ptr const& opts);
};


}

