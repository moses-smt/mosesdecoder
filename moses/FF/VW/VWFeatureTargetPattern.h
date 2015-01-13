#pragma once

#include <string>
#include "VWFeatureTarget.h"
#include "moses/FF/CorrectionPattern.h"

namespace Moses
{
  
class VWFeatureTargetPattern : public VWFeatureTarget
{
  public:
    VWFeatureTargetPattern(const std::string &line)
      : VWFeatureTarget(line)
    {
      ReadParameters();
      
      VWFeatureBase::UpdateRegister();
    }

    void operator()(const InputType &input
                            , const InputPath &inputPath
                            , const TargetPhrase &targetPhrase
                            , Discriminative::Classifier &classifier) const
    {
      const Phrase &source = inputPath.GetPhrase();
      std::vector<std::string> sourceTokens;
      for(size_t i = 0; i < source.GetSize(); ++i)
        sourceTokens.push_back(source.GetWord(i).GetString(m_sourceFactors, false));
  
      std::vector<std::string> targetTokens;
      for(size_t i = 0; i < targetPhrase.GetSize(); ++i)
        targetTokens.push_back(targetPhrase.GetWord(i).GetString(m_targetFactors, false));
  
      std::vector<std::string> patternList
        = CorrectionPattern::CreatePattern(sourceTokens, targetTokens, input, inputPath, m_targetFactors);
      for(size_t i = 0; i < patternList.size(); i++)
        classifier.AddLabelDependentFeature("tpatt^" + patternList[i]);
        
      if(patternList.empty())
        classifier.AddLabelDependentFeature("tpatt^none");
    }

    virtual void SetParameter(const std::string& key, const std::string& value) {
      VWFeatureTarget::SetParameter(key, value);
    }
};

}
