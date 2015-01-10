#pragma once

#include <set>
#include <string>
#include "StatelessFeatureFunction.h"

namespace Moses
{
class WordsRange;
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
                , ScoreComponentCollection &estimatedFutureScore) const;
  void EvaluateWithSourceContext(const InputType &input
                , const InputPath &inputPath
                , const TargetPhrase &targetPhrase
                , const StackVec *stackVec
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection *estimatedFutureScore = NULL) const;
  
  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
              , const TranslationOptionList &translationOptionList) const
  {}
  
  void EvaluateWhenApplied(const Hypothesis& hypo,
                ScoreComponentCollection* accumulator) const;
  void EvaluateWhenApplied(const ChartHypothesis &hypo,
                     ScoreComponentCollection* accumulator) const;

  void SetParameter(const std::string& key, const std::string& value);
  std::vector<float> DefaultWeights() const;

protected:
  bool m_hardConstraint;
  bool ContainTerm(const InputType &input,
                   const WordsRange &ntRange,
                   const std::set<Word> &terms) const;
};

}


