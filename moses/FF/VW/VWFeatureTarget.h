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
    
    // Gets its pure virtual functions from VWFeatureBase
    
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