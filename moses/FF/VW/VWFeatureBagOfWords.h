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
    }
    
    void operator()(const InputType &input
                  , const InputPath &inputPath
                  , const WordsRange &sourceRange
                  , Discriminative::Classifier *classifier) const
    {
      std::cerr << GetScoreProducerDescription() << " got Phrase: " << sourceRange << std::endl;
    }
    
    virtual void SetParameter(const std::string& key, const std::string& value) {
      VWFeatureSource::SetParameter(key, value);
    }

};

}