#pragma once

// $Id$

#include "StatelessFeatureFunction.h"

namespace Moses
{

class WordsRange;


/** unknown word penalty */
class UnknownWordPenaltyProducer : public StatelessFeatureFunction
{
public:
  UnknownWordPenaltyProducer(const std::string &line);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }
  std::vector<float> DefaultWeights() const;

  void Evaluate(const Hypothesis& hypo,
                ScoreComponentCollection* accumulator) const
  {}
  void EvaluateChart(const ChartHypothesis &hypo,
                     ScoreComponentCollection* accumulator) const
  {}
  void Evaluate(const InputType &input
                , const InputPath &inputPath
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection *estimatedFutureScore = NULL) const
  {}
  void Evaluate(const Phrase &source
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection &estimatedFutureScore) const
  {}

};

}

