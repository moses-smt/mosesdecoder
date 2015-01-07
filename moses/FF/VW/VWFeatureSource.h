#pragma once

#include <string>
#include "VWFeatureBase.h"

namespace Moses
{
  
class VWFeatureSource : public VWFeatureBase
{
  public:
    VWFeatureSource(const std::string &line)
      : VWFeatureBase(line, true)
    {}
    
    virtual void operator()(const InputType &input
                            , const InputPath &inputPath
                            , const WordsRange &sourceRange
                            , Discriminative::Classifier *classifier) const = 0;
    
    virtual void operator()(const InputType &input
                            , const InputPath &inputPath
                            , const TargetPhrase &targetPhrase
                            , Discriminative::Classifier *classifier) const
    {}
};

}