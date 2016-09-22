#pragma once

#include <string>
#include <algorithm>
#include "VWFeatureSource.h"
#include "moses/Util.h"

namespace Moses
{

class VWFeatureSourceWindow : public VWFeatureSource
{
public:
  VWFeatureSourceWindow(const std::string &line)
    : VWFeatureSource(line), m_size(DEFAULT_WINDOW_SIZE) {
    ReadParameters();

    // Call this last
    VWFeatureBase::UpdateRegister();
  }

  void operator()(const InputType &input
                  , const Range &sourceRange
                  , Discriminative::Classifier &classifier
                  , Discriminative::FeatureVector &outFeatures) const {
    int begin = sourceRange.GetStartPos();
    int end   = sourceRange.GetEndPos() + 1;
    int inputLen = input.GetSize();

    for (int i = std::max(0, begin - m_size); i < begin; i++) {
      outFeatures.push_back(classifier.AddLabelIndependentFeature("c^" + SPrint(i - begin) + "^" + GetWord(input, i)));
    }

    for (int i = end; i < std::min(end + m_size, inputLen); i++) {
      outFeatures.push_back(classifier.AddLabelIndependentFeature("c^" + SPrint(i - end + 1) + "^" + GetWord(input, i)));
    }
  }

  virtual void SetParameter(const std::string& key, const std::string& value) {
    if (key == "size") {
      m_size = Scan<size_t>(value);
    } else {
      VWFeatureSource::SetParameter(key, value);
    }
  }

private:
  static const int DEFAULT_WINDOW_SIZE = 3;

  int m_size;
};

}
