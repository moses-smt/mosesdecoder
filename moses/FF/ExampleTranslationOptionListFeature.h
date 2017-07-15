#pragma once

#include <string>
#include "StatelessFeatureFunction.h"

namespace Moses
{

class ExampleTranslationOptionListFeature : public StatelessFeatureFunction
{
public:
  ExampleTranslationOptionListFeature(const std::string &line)
    :StatelessFeatureFunction(1, line) {
    ReadParameters();
  }

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
    std::vector<float> newScores(m_numScoreComponents);
    newScores[0] = translationOptionList.size();

    TranslationOptionList::const_iterator iterTransOpt;
    for(iterTransOpt = translationOptionList.begin() ;
        iterTransOpt != translationOptionList.end() ; ++iterTransOpt) {
      TranslationOption &transOpt = **iterTransOpt;

      ScoreComponentCollection &scoreBreakDown = transOpt.GetScoreBreakdown();
      scoreBreakDown.PlusEquals(this, newScores);

      transOpt.UpdateScore();
    }
  }

  void EvaluateWhenApplied(const Hypothesis& hypo,
                           ScoreComponentCollection* accumulator) const {
  }

  void EvaluateWhenApplied(const ChartHypothesis &hypo,
                           ScoreComponentCollection* accumulator) const {
  }


  void SetParameter(const std::string& key, const std::string& value) {
  }

};

}

