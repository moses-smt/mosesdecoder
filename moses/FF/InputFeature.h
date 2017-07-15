#pragma once

#include "InputFeature.h"
#include "StatelessFeatureFunction.h"

namespace Moses
{


class InputFeature : public StatelessFeatureFunction
{
protected:
  static InputFeature *s_instance;

  size_t m_numInputScores;
  size_t m_numRealWordCount;
  bool m_legacy;

public:
  static const InputFeature *InstancePtr() {
    return s_instance;
  }

  InputFeature(const std::string &line);

  void Load(AllOptions::ptr const& opts);

  void SetParameter(const std::string& key, const std::string& value);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  size_t GetNumInputScores() const {
    return m_numInputScores;
  }
  size_t GetNumRealWordsInInput() const {
    return m_numRealWordCount;
  }

  void EvaluateInIsolation(const Phrase &source
                           , const TargetPhrase &targetPhrase
                           , ScoreComponentCollection &scoreBreakdown
                           , ScoreComponentCollection &estimatedScores) const {
  }

  void EvaluateWithSourceContext(const InputType &input
                                 , const InputPath &inputPath
                                 , const TargetPhrase &targetPhrase
                                 , const StackVec *stackVec
                                 , ScoreComponentCollection &scoreBreakdown
                                 , ScoreComponentCollection *estimatedScores = NULL) const;

  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
      , const TranslationOptionList &translationOptionList) const {
  }

  void EvaluateWhenApplied(const Hypothesis& hypo,
                           ScoreComponentCollection* accumulator) const {
  }
  void EvaluateWhenApplied(const ChartHypothesis &hypo,
                           ScoreComponentCollection* accumulator) const {
  }


};


}

