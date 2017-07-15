#pragma once

#include "StatelessFeatureFunction.h"

namespace Moses
{

class PhraseDistanceFeature : public StatelessFeatureFunction
{
  enum Measure {
    EuclideanDistance,
    TotalVariationDistance,
  };

public:
  PhraseDistanceFeature(const std::string &line);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  virtual void EvaluateInIsolation(const Phrase &source
                                   , const TargetPhrase &targetPhrase
                                   , ScoreComponentCollection &scoreBreakdown
                                   , ScoreComponentCollection &estimatedScores) const {
  }

  void EvaluateWhenApplied(const Hypothesis& hypo,
                           ScoreComponentCollection* accumulator) const {
  }
  void EvaluateWhenApplied(const ChartHypothesis &hypo,
                           ScoreComponentCollection* accumulator) const {
  }
  void EvaluateWhenApplied(const Syntax::SHyperedge &hyperedge,
                           ScoreComponentCollection* accumulator) const {
  }

  void EvaluateWithSourceContext(const InputType &input
                                 , const InputPath &inputPath
                                 , const TargetPhrase &targetPhrase
                                 , const StackVec *stackVec
                                 , ScoreComponentCollection &scoreBreakdown
                                 , ScoreComponentCollection *estimatedScores = NULL) const;

  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
      , const TranslationOptionList &translationOptionList) const {
  }
  void SetParameter(const std::string& key, const std::string& value);

protected:
  Measure m_measure;
  std::string m_space;
  size_t m_spaceID;
};

} //namespace
