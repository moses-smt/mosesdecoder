#pragma once

#include <string>
#include "VWFeatureBase.h"

namespace Moses
{
  
class VWFeatureBagOfWords : public VWFeatureBase
{
  public:
    VWFeatureBagOfWords(const std::string &line)
      : VWFeatureBase(line)
    {}
  
    void operator()(const InputType &input
                  , const InputPath &inputPath
                  , const TargetPhrase &targetPhrase
                  , Discriminative::Classifier *classifier) const
    {
      std::cerr << GetScoreProducerDescription() << " got TargetPhrase: " << targetPhrase << std::endl;
    }
};

}