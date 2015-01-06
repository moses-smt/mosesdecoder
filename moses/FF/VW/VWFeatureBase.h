#pragma once

#include <string>
#include <Classifier.h>
#include "moses/FF/StatelessFeatureFunction.h"

namespace Moses
{
  
class VWFeatureBase : public StatelessFeatureFunction
{
  public:
    VWFeatureBase(const std::string &line)
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
    {}
    
    virtual void operator()(const InputType &input
                  , const InputPath &inputPath
                  , const TargetPhrase &targetPhrase
                  , Discriminative::Classifier *classifier) const = 0;
    
    static const std::vector<VWFeatureBase*>& GetFeatures() const {
      return s_features;
    }
  
  private:
    static std::vector<VWFeatureBase*> s_features;
};

}

