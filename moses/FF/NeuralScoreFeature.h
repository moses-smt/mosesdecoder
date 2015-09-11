#pragma once

#include <string>
#include "StatefulFeatureFunction.h"
#include "FFState.h"
#include <boost/shared_ptr.hpp>

class NMT_Wrapper;

namespace Moses
{

class NeuralScoreState : public FFState
{
  int m_targetLen;
public:
  NeuralScoreState(int targetLen)
    :m_targetLen(targetLen) {
  }

  int Compare(const FFState& other) const;
};


class NeuralScoreFeature : public StatefulFeatureFunction
{
public:
  NeuralScoreFeature(const std::string &line);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  virtual const FFState* EmptyHypothesisState(const InputType &input) const {
    return new NeuralScoreState(0);
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
      , const TranslationOptionList &translationOptionList) const;

  FFState* EvaluateWhenApplied(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const;
  FFState* EvaluateWhenApplied(
    const ChartHypothesis& /* cur_hypo */,
    int /* featureID - used to index the state in the previous hypotheses */,
    ScoreComponentCollection* accumulator) const;

  void SetParameter(const std::string& key, const std::string& value);

private:
  std::string m_statePath;
  std::string m_modelPath;
  std::string m_wrapperPath;

  boost::shared_ptr<NMT_Wrapper> m_wrapper;
};

}

