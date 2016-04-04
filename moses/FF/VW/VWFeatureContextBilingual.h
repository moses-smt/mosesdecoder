#pragma once

#include <string>
#include <boost/foreach.hpp>
#include <algorithm>
#include "VWFeatureContext.h"
#include "moses/Util.h"

namespace Moses
{

class VWFeatureContextBilingual : public VWFeatureContext
{
public:
  VWFeatureContextBilingual(const std::string &line)
    : VWFeatureContext(line, DEFAULT_WINDOW_SIZE) {
    ReadParameters();

    // Call this last
    VWFeatureBase::UpdateRegister();
  }

  virtual void operator()(const InputType &input
                          , const Phrase &contextPhrase
                          , const AlignmentInfo &alignmentInfo
                          , Discriminative::Classifier &classifier
                          , Discriminative::FeatureVector &outFeatures) const {
    for (size_t i = 0; i < m_contextSize; i++) {
      std::string tgtWord = GetWord(contextPhrase, i);
      std::vector<std::string> alignedTo = GetAlignedSourceWords(contextPhrase, input, alignmentInfo, i);
      BOOST_FOREACH(const std::string &srcWord, alignedTo) {
        outFeatures.push_back(classifier.AddLabelIndependentFeature("tcblng^-" + SPrint(i + 1) + "^" + tgtWord + "^" + srcWord));
      }
    }
  }

  virtual void SetParameter(const std::string& key, const std::string& value) {
    VWFeatureContext::SetParameter(key, value);
  }

private:
  static const int DEFAULT_WINDOW_SIZE = 1;
};

}
