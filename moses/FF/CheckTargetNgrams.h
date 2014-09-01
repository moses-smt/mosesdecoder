#pragma once

#include <string>
#include "StatefulFeatureFunction.h"
#include "FFState.h"
#include "BloomFilter.h"

class bloom_filter;

namespace Moses
{

class CheckTargetNgramsState : public FFState
{
  unsigned int m_history;

public:
  CheckTargetNgramsState(unsigned int history)
    :m_history(history)
  {}

  int Compare(const FFState& other) const;
};

class CheckTargetNgrams : public StatefulFeatureFunction
{
  std::string m_filePath;
  int m_maxorder;
  int m_minorder;
  std::vector<FactorType> m_FactorsVec;

  bloom_filter m_bloomfilter;

public:
  CheckTargetNgrams(const std::string &line);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }
  virtual const FFState* EmptyHypothesisState(const InputType &input) const {
    return new CheckTargetNgramsState(0);
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
  FFState* EvaluateWhenApplied(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const;
  FFState* EvaluateWhenApplied(
    const ChartHypothesis& /* cur_hypo */,
    int /* featureID - used to index the state in the previous hypotheses */,
    ScoreComponentCollection* accumulator) const;

  void SetParameter(const std::string& key, const std::string& value);

  void Load();

};


}

