#pragma once

#include <string>
#include "VWFeatureTarget.h"

namespace Moses
{

class VWFeatureTargetBigrams : public VWFeatureTarget
{
public:
  VWFeatureTargetBigrams(const std::string &line)
    : VWFeatureTarget(line) {
    ReadParameters();

    VWFeatureBase::UpdateRegister();
  }

  void operator()(const InputType &input
                  , const TargetPhrase &targetPhrase
                  , Discriminative::Classifier &classifier
                  , Discriminative::FeatureVector &outFeatures) const {
    for (size_t i = 1; i < targetPhrase.GetSize(); i++) {
      outFeatures.push_back(classifier.AddLabelDependentFeature("tbigram^" + GetWord(targetPhrase, i - 1) + "^" + GetWord(targetPhrase, i)));
    }
  }

  virtual void SetParameter(const std::string& key, const std::string& value) {
    VWFeatureTarget::SetParameter(key, value);
  }
};

}
