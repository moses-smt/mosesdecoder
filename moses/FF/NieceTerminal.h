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
  NieceTerminal(const std::string &line)
    :StatelessFeatureFunction(line)
  {}

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  void Evaluate(const Phrase &source
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection &estimatedFutureScore) const;
  void Evaluate(const InputType &input
                , const InputPath &inputPath
                , const TargetPhrase &targetPhrase
                , const StackVec *stackVec
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection *estimatedFutureScore = NULL) const;
  void Evaluate(const Hypothesis& hypo,
                ScoreComponentCollection* accumulator) const;
  void EvaluateChart(const ChartHypothesis &hypo,
                     ScoreComponentCollection* accumulator) const;

protected:
  bool ContainTerm(const InputType &input,
		  	  	  const WordsRange &ntRange,
		  	  	  const std::set<Word> &terms) const;
};

}

