#pragma once

#include <string>
#include "VWFeatureBase.h"

namespace Moses
{
  
class VWFeatureTarget : public VWFeatureBase
{
  public:
    VWFeatureTarget(const std::string &line)
      : VWFeatureBase(line, false)
    {}
    
    virtual void operator()(const InputType &input
                            , const InputPath &inputPath
                            , const TargetPhrase &targetPhrase
                            , Discriminative::Classifier *classifier) const = 0;
    
    virtual void operator()(const InputType &input
                            , const InputPath &inputPath
                            , const Phrase &sourcePhrase
                            , Discriminative::Classifier *classifier) const
    {}
    
    virtual void SetParameter(const std::string& key, const std::string& value) {
      VMFeatureBase::SetParameter(key, value);
    }
};

}