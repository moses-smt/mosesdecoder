#pragma once

#include <string>
#include "moses/HypothesisStackNormal.h"
#include "moses/TranslationOptionCollection.h"
#include "StatefulFeatureFunction.h"
#include "FFState.h"
#include <boost/shared_ptr.hpp>

class NMT_Wrapper;

namespace Moses
{


class NeuralScoreFeature : public StatefulFeatureFunction
{
public:
  NeuralScoreFeature(const std::string &line);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  /*
  void InitializeForInput(ttasksptr const& ttask);
  void CleanUpAfterSentenceProcessing(ttasksptr const& ttask);
  */

  void ProcessStack(const HypothesisStackNormal& hstack,
                    const TranslationOptionCollection& to,
                    size_t index);
  
  virtual const FFState* EmptyHypothesisState(const InputType &input) const;
  
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
  size_t m_stateLength;
  size_t m_factor;
  boost::shared_ptr<NMT_Wrapper> m_wrapper;
};

}

