#pragma once

#include <boost/unordered_set.hpp>
#include <string>
#include "StatelessFeatureFunction.h"

namespace Moses
{
class Range;
class Word;

// 1 of the non-term covers the same word as 1 of the terminals
class NieceTerminal : public StatelessFeatureFunction
{
public:
  NieceTerminal(const std::string &line);

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
      , const TranslationOptionList &translationOptionList) const {
  }

  void EvaluateWhenApplied(const Hypothesis& hypo,
                           ScoreComponentCollection* accumulator) const;
  void EvaluateWhenApplied(const ChartHypothesis &hypo,
                           ScoreComponentCollection* accumulator) const;

  void SetParameter(const std::string& key, const std::string& value);
  std::vector<float> DefaultWeights() const;

protected:
  bool m_hardConstraint;
  bool ContainTerm(const InputType &input,
                   const Range &ntRange,
                   const boost::unordered_set<Word> &terms) const;
};

}


