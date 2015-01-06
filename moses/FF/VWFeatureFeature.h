#pragma once

#include <string>
#include "StatelessFeatureFunction.h"

namespace Moses
{
  
  // BE IGNORED BY STATIC DATA.

class VWFeatureFeature : public StatelessFeatureFunction
{
  public:
    VWFeatureFeature(const std::string &line)
      :StatelessFeatureFunction(0, line)
    {
      ReadParameters();
      s_features.push_back(this);
    }
  
    bool IsUseable(const FactorMask &mask) const {
      return true;
    }
  
    // Official hooks do nothing
    void EvaluateInIsolation(const Phrase &source
                  , const TargetPhrase &targetPhrase
                  , ScoreComponentCollection &scoreBreakdown
                  , ScoreComponentCollection &estimatedFutureScore) const {}
    void EvaluateWithSourceContext(const InputType &input
                  , const InputPath &inputPath
                  , const TargetPhrase &targetPhrase
                  , const StackVec *stackVec
                  , ScoreComponentCollection &scoreBreakdown
                  , ScoreComponentCollection *estimatedFutureScore = NULL) const {}
  
    void EvaluateTranslationOptionListWithSourceContext(const InputType &input
                  , const TranslationOptionList &translationOptionList) const {}
  
    void EvaluateWhenApplied(const Hypothesis& hypo,
                  ScoreComponentCollection* accumulator) const {}
    void EvaluateWhenApplied(const ChartHypothesis &hypo,
                       ScoreComponentCollection* accumulator) const {}
    
    void SetParameter(const std::string& key, const std::string& value)
    {
    }
    
    void DoSomething(const InputType &input
                  , const InputPath &inputPath
                  , const TargetPhrase &targetPhrase) {

      std::cerr << GetScoreProducerDescription() << " got TargetPhrase: " << targetPhrase << std::endl;

    }
    
    static std::vector<VWFeatureFeature*>& GetFeatures() {
      return s_features;  
    }
  
  private:
    static std::vector<VWFeatureFeature*> s_features;
  };
}

