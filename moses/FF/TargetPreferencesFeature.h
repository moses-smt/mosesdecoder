#pragma once

#include <string>
#include <map>
#include <iostream>
#include <boost/unordered_map.hpp>
#include "StatefulFeatureFunction.h"
#include "FFState.h"
#include "util/exception.hh"
#include <stdint.h>

namespace Moses
{

class TargetPreferencesFeatureState : public FFState
{

public:

  TargetPreferencesFeatureState(bool distinguishStates)
    : m_distinguishStates(distinguishStates)
  {}

  void AddProbabilityForLHSLabel(size_t label, double cost);

  void NormalizeProbabilitiesForLHSLabels(double denominator);

  const std::map<size_t,double> &GetProbabilitiesForLHSLabels() const {
    return m_probabilitiesForLHSLabels;
  }

  double GetProbabilityForLHSLabel(size_t label, bool &isMatch) const;

  size_t hash() const;

  virtual bool operator==(const FFState& other) const;


private:

  const bool m_distinguishStates;
  std::map<size_t,double> m_probabilitiesForLHSLabels;

};


class TargetPreferencesFeature : public StatefulFeatureFunction
{

public:

  TargetPreferencesFeature(const std::string &line);

  ~TargetPreferencesFeature();

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  virtual const FFState* EmptyHypothesisState(const InputType &input) const {
    return new TargetPreferencesFeatureState(m_distinguishStates);
  }

  void SetParameter(const std::string& key, const std::string& value);

  void Load(AllOptions::ptr const& opts);

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
  {}

  FFState* EvaluateWhenApplied(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const {
    UTIL_THROW2(GetScoreProducerDescription() << ": feature currently not implemented for phrase-based decoding.");
    return new TargetPreferencesFeatureState(m_distinguishStates);
  };

  FFState* EvaluateWhenApplied(
    const ChartHypothesis& cur_hypo,
    int featureID, // used to index the state in the previous hypotheses
    ScoreComponentCollection* accumulator) const;


private:

  std::string m_labelSetFile;
  std::string m_unknownLeftHandSideFile;
  size_t m_featureVariant;
  bool m_distinguishStates;
  bool m_noMismatches;

  mutable boost::unordered_map<std::string,size_t> m_labels;
  mutable std::vector<std::string> m_labelsByIndex;
  mutable size_t m_XRHSLabel;
  mutable size_t m_XLHSLabel;
  mutable size_t m_GlueTopLabel;
  std::map<size_t,double> m_unknownLHSProbabilities;

  void LoadLabelSet();
  void LoadUnknownLeftHandSideFile();

};

}

