#pragma once

#include <string>
#include <vector>
#include <set>
#include <iostream>
#include "StatefulFeatureFunction.h"
#include "FFState.h"
#include "util/exception.hh"
#include <stdint.h>

namespace Moses
{

class TargetConstituentAdjacencyFeatureState : public FFState
{

public:

  friend class TargetConstituentAdjacencyFeature;

  TargetConstituentAdjacencyFeatureState(bool recombine)
    : m_recombine(recombine)
  {};

  size_t hash() const;

  virtual bool operator==(const FFState& other) const;

private:

  const bool m_recombine;
  std::map<const Factor*, float> m_collection;

};


class TargetConstituentAdjacencyFeature : public StatefulFeatureFunction
{

public:

  TargetConstituentAdjacencyFeature(const std::string &line);

  ~TargetConstituentAdjacencyFeature()
  {};

  bool IsUseable(const FactorMask &mask) const {
    return true;
  };

  virtual const FFState* EmptyHypothesisState(const InputType &input) const {
    return new TargetConstituentAdjacencyFeatureState(m_recombine);
  };

  void SetParameter(const std::string& key, const std::string& value);

  void Load(AllOptions::ptr const& opts)
  {};

  void EvaluateInIsolation(const Phrase &source
                           , const TargetPhrase &targetPhrase
                           , ScoreComponentCollection &scoreBreakdown
                           , ScoreComponentCollection &estimatedFutureScore) const
  {};

  void EvaluateWithSourceContext(const InputType &input
                                 , const InputPath &inputPath
                                 , const TargetPhrase &targetPhrase
                                 , const StackVec *stackVec
                                 , ScoreComponentCollection &scoreBreakdown
                                 , ScoreComponentCollection *estimatedFutureScore = NULL) const
  {};

  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
      , const TranslationOptionList &translationOptionList) const
  {};

  FFState* EvaluateWhenApplied(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const;

  FFState* EvaluateWhenApplied(
    const ChartHypothesis& cur_hypo,
    int featureID, // used to index the state in the previous hypotheses
    ScoreComponentCollection* accumulator) const {
    UTIL_THROW2(GetScoreProducerDescription() << ": feature currently not implemented for chart-based decoding.");
    return new TargetConstituentAdjacencyFeatureState(m_recombine);
  };


private:

  size_t m_featureVariant;
  bool m_recombine;

};

}

