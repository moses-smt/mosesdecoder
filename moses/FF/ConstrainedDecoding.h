#pragma once

#include <string>
#include <map>
#include "StatefulFeatureFunction.h"
#include "FFState.h"
#include "moses/Phrase.h"

namespace Moses
{
class ConstrainedDecodingState : public FFState
{
public:
  ConstrainedDecodingState() {
  }

  ConstrainedDecodingState(const Hypothesis &hypo);
  ConstrainedDecodingState(const ChartHypothesis &hypo);

  int Compare(const FFState& other) const;

  const Phrase &GetPhrase() const {
    return m_outputPhrase;
  }

protected:
  Phrase m_outputPhrase;
};

//////////////////////////////////////////////////////////////////

// only allow hypotheses which match reference
class ConstrainedDecoding : public StatefulFeatureFunction
{
public:
  ConstrainedDecoding(const std::string &line);

  void Load();

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  void EvaluateInIsolation(const Phrase &source
                           , const TargetPhrase &targetPhrase
                           , ScoreComponentCollection &scoreBreakdown
                           , ScoreComponentCollection &estimatedFutureScore) const {
  }

  void EvaluateWithSourceContext(const InputType &input
                                 , const InputPath &inputPath
                                 , const TargetPhrase &targetPhrase
                                 , const StackVec *stackVec
                                 , ScoreComponentCollection &scoreBreakdown
                                 , ScoreComponentCollection *estimatedFutureScore = NULL) const {
  }

  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
      , const TranslationOptionList &translationOptionList) const {
  }

  FFState* EvaluateWhenApplied(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const;

  FFState* EvaluateWhenApplied(
    const ChartHypothesis& /* cur_hypo */,
    int /* featureID - used to index the state in the previous hypotheses */,
    ScoreComponentCollection* accumulator) const;

  virtual const FFState* EmptyHypothesisState(const InputType &input) const {
    return new ConstrainedDecodingState();
  }

  std::vector<float> DefaultWeights() const;

  void SetParameter(const std::string& key, const std::string& value);

protected:
  std::vector<std::string> m_paths;
  std::map<long, std::vector<Phrase> > m_constraints;
  int m_maxUnknowns;
  bool m_negate; // only keep translations which DON'T match the reference
  bool m_soft;

};


}

