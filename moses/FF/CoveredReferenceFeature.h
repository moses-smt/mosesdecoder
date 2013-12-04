#pragma once

#include <vector>
#include <string>
#include <set>
#include "StatefulFeatureFunction.h"
#include "FFState.h"
#include "moses/Phrase.h"
#include "moses/Factor.h"
#include "boost/unordered_map.hpp"

namespace Moses
{

// Given a file with reference translation, reward phrases for matching words from the reference.

class CoveredReferenceState : public FFState
{
public:
  std::multiset<std::string> m_coveredRef;

  int Compare(const FFState& other) const;
};

class CoveredReferenceFeature : public StatefulFeatureFunction
{
  static std::multiset<std::string> GetWordsInPhrase(const Phrase &phr) {
    std::multiset<std::string> out;
    for (size_t i = 0; i < phr.GetSize(); i++) {
      out.insert(phr.GetFactor(i, 0)->GetString().as_string());
    }
    return out;
  }

  std::string m_path;
  boost::unordered_map<long, std::multiset<std::string> > m_refs;

public:
  CoveredReferenceFeature(const std::string &line)
    :StatefulFeatureFunction(1, line)
  {
    m_tuneable = true;
    ReadParameters();
  }

  void Load();

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }
  virtual const FFState* EmptyHypothesisState(const InputType &input) const {
    return new CoveredReferenceState();
  }

  void Evaluate(const Phrase &source
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection &estimatedFutureScore) const;
  void Evaluate(const InputType &input
                , const InputPath &inputPath
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection *estimatedFutureScore = NULL) const;
  FFState* Evaluate(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const;
  FFState* EvaluateChart(
    const ChartHypothesis& /* cur_hypo */,
    int /* featureID - used to index the state in the previous hypotheses */,
    ScoreComponentCollection* accumulator) const;

  void SetParameter(const std::string& key, const std::string& value);
};

}
