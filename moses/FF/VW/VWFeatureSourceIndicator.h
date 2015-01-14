#pragma once

#include <string>
#include <algorithm>
#include "VWFeatureSource.h"
#include "moses/Util.h"

namespace Moses
{

class VWFeatureSourceIndicator : public VWFeatureSource
{
public:
  VWFeatureSourceIndicator(const std::string &line)
    : VWFeatureSource(line) {
    ReadParameters();

    // Call this last
    VWFeatureBase::UpdateRegister();
  }

  void operator()(const InputType &input
                  , const InputPath &inputPath
                  , const WordsRange &sourceRange
                  , Discriminative::Classifier &classifier) const {
    size_t begin = sourceRange.GetStartPos();
    size_t end   = sourceRange.GetEndPos() + 1;

    std::vector<std::string> words(end - begin);

    for (size_t i = 0; i < end - begin; i++)
      words[i] = GetWord(input, begin + i);

    classifier.AddLabelIndependentFeature("sind^" + Join(" ", words));
  }

  virtual void SetParameter(const std::string& key, const std::string& value) {
    VWFeatureSource::SetParameter(key, value);
  }
};

}
