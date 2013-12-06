#pragma once

#include <string>
#include "StatelessFeatureFunction.h"

namespace Moses
{
class TargetPhrase;
class ScoreComponentCollection;

class WordPenaltyProducer : public StatelessFeatureFunction
{
protected:
  static WordPenaltyProducer *s_instance;

public:
  static const WordPenaltyProducer& Instance() {
    return *s_instance;
  }
  static WordPenaltyProducer& InstanceNonConst() {
    return *s_instance;
  }

  WordPenaltyProducer(const std::string &line);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  virtual void Evaluate(const Phrase &source
                        , const TargetPhrase &targetPhrase
                        , ScoreComponentCollection &scoreBreakdown
                        , ScoreComponentCollection &estimatedFutureScore) const;
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

};

}

