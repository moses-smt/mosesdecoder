#pragma once

#include <string>
#include "VWFeatureSource.h"

namespace Moses
{
  
class VWFeatureBagOfWords : public VWFeatureSource
{
  public:
    VWFeatureBagOfWords(const std::string &line)
      : VWFeatureSource(line)
    {
      ReadParameters();
      
      // Call this last
      VWFeatureBase::UpdateRegister();
    }

    void operator()(const InputType &input
                  , const InputPath &inputPath
                  , const WordsRange &sourceRange
                  , Discriminative::Classifier *classifier) const
    {
      for (size_t i = 0; i < input.GetSize(); i++) {
        classifier->AddLabelIndependentFeature("bow^" + input.GetWord(i).GetString(m_sourceFactors, false));
      }
    }
    
    virtual void SetParameter(const std::string& key, const std::string& value) {
      VWFeatureSource::SetParameter(key, value);
    }
};

}
