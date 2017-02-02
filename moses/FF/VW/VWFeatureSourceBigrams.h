#pragma once

#include <string>
#include "VWFeatureSource.h"

namespace Moses
{

class VWFeatureSourceBigrams : public VWFeatureSource
{
public:
  VWFeatureSourceBigrams(const std::string &line)
    : VWFeatureSource(line) {
    ReadParameters();

    // Call this last
    VWFeatureBase::UpdateRegister();
  }

  void operator()(const InputType &input
                  , const Range &sourceRange
                  , Discriminative::Classifier &classifier
                  , Discriminative::FeatureVector &outFeatures) const {
    for (size_t i = 1; i < input.GetSize(); i++) {
      outFeatures.push_back(classifier.AddLabelIndependentFeature("bigram^" + GetWord(input, i - 1) + "^" + GetWord(input, i)));
    }
  }

  virtual void SetParameter(const std::string& key, const std::string& value) {
    VWFeatureSource::SetParameter(key, value);
  }
};

}
