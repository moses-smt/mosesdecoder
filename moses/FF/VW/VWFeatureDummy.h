#pragma once

#include <string>
#include "moses/FF/StatelessFeatureFunction.h"

#include "VWFeatureBase.h"

namespace Moses
{

class VWFeatureDummy : public StatelessFeatureFunction
{
public:
  VWFeatureDummy(const std::string &line)
    :StatelessFeatureFunction(1, line)
  {
    ReadParameters();
  }

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  void EvaluateInIsolation(const Phrase &source
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection &estimatedFutureScore) const
  {}
  
  void EvaluateWithSourceContext(const InputType &input
                , const InputPath &inputPath
                , const TargetPhrase &targetPhrase
                , const StackVec *stackVec
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection *estimatedFutureScore = NULL) const
  {}

  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
                , const TranslationOptionList &translationOptionList) const
  {
    std::vector<VWFeatureBase*>& features = VWFeatureBase::GetFeatures();
    
    TranslationOptionList::const_iterator iterTransOpt;
    for(iterTransOpt = translationOptionList.begin() ;
        iterTransOpt != translationOptionList.end() ; ++iterTransOpt) {
     
      TranslationOption &transOpt = **iterTransOpt;
      for(size_t i = 0; i < features.size(); ++i)
      
        (*features[i])(input, transOpt.GetInputPath(), transOpt.GetTargetPhrase());
    }
    
    for(iterTransOpt = translationOptionList.begin() ;
        iterTransOpt != translationOptionList.end() ; ++iterTransOpt) {
      TranslationOption &transOpt = **iterTransOpt;
      
      vector<float> newScores(m_numScoreComponents);
      newScores[0] = 1; // Future result of VW
    
      ScoreComponentCollection &scoreBreakDown = transOpt.GetScoreBreakdown();
      scoreBreakDown.PlusEquals(this, newScores);
      
      transOpt.UpdateScore();
    }
  }

  void EvaluateWhenApplied(const Hypothesis& hypo,
                ScoreComponentCollection* accumulator) const
  {}
  
  void EvaluateWhenApplied(const ChartHypothesis &hypo,
                     ScoreComponentCollection* accumulator) const
  {}


  void SetParameter(const std::string& key, const std::string& value)
  {
  }

};

}

