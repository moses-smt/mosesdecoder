#pragma once

#include <string>
#include "VWFeatureTarget.h"

namespace Moses
{

class VWFeatureTargetPhraseInternal : public VWFeatureTarget
{
public:
  VWFeatureTargetPhraseInternal(const std::string &line)
    : VWFeatureTarget(line) {
    ReadParameters();

    VWFeatureBase::UpdateRegister();
  }

  void operator()(const InputType &input
                  , const InputPath &inputPath
                  , const TargetPhrase &targetPhrase
                  , Discriminative::Classifier &classifier) const {
    for (size_t i = 0; i < targetPhrase.GetSize(); i++) {
      classifier.AddLabelDependentFeature("tin^" + GetWord(targetPhrase, i));
    }
  }

  virtual void SetParameter(const std::string& key, const std::string& value) {
    VWFeatureTarget::SetParameter(key, value);
  }
};

}
