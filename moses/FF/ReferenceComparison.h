#pragma once
#include <string>
#include "StatelessFeatureFunction.h"

namespace Moses
{

// Count how many hypotheses are in each stack, compare score with reference hypo
// NOT threadsafe.
class ReferenceComparison : public StatelessFeatureFunction
{
public:
  ReferenceComparison(const std::string &line);

  virtual bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  virtual void Evaluate(const Phrase &source
                        , const TargetPhrase &targetPhrase
                        , ScoreComponentCollection &scoreBreakdown
                        , ScoreComponentCollection &estimatedFutureScore) const
  {}

  <<<<<<< HEAD
  virtual void Evaluate(const InputType &input
                        , const InputPath &inputPath
                        , const TargetPhrase &targetPhrase
                        , ScoreComponentCollection &scoreBreakdown
                        , ScoreComponentCollection *estimatedFutureScore = NULL) const
  {}
  =======
    virtual void Evaluate(const InputType &input
                          , const InputPath &inputPath
                          , const TargetPhrase &targetPhrase
                          , const StackVec *stackVec
                          , ScoreComponentCollection &scoreBreakdown
                          , ScoreComponentCollection *estimatedFutureScore = NULL) const
  {}
  >>>>>>> master

  virtual void Evaluate(const Hypothesis& hypo,
                        ScoreComponentCollection* accumulator) const
  {}

  virtual void EvaluateChart(const ChartHypothesis &hypo,
                             ScoreComponentCollection* accumulator) const
  {}

  std::vector<float> DefaultWeights() const {
    return std::vector<float>();
  }

protected:

};

}

