#pragma once

#include <string>
#include "VWFeatureSource.h"

namespace Moses
{

class VWFeatureSourceBagOfWords : public VWFeatureSource
{
public:
  VWFeatureSourceBagOfWords(const std::string &line)
    : VWFeatureSource(line) {
    ReadParameters();

    // Call this last
    VWFeatureBase::UpdateRegister();
  }

  void operator()(const InputType &input
                  , const InputPath &inputPath
                  , const WordsRange &sourceRange
                  , Discriminative::Classifier &classifier) const {
    for (size_t i = 0; i < input.GetSize(); i++) {
      classifier.AddLabelIndependentFeature("bow^" + GetWord(input, i));
    }
  }

  virtual void SetParameter(const std::string& key, const std::string& value) {
    VWFeatureSource::SetParameter(key, value);
  }
};

}
