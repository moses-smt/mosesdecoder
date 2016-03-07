#pragma once

#include <string>
#include "VWFeatureBase.h"
#include "moses/InputType.h"
#include "moses/TypeDef.h"

namespace Moses
{

// Inherit from this for source-dependent classifier features. They will
// automatically register with the classifier class named VW0 or one or more
// names specified by the used-by=name1,name2,... parameter.
//
// The classifier gets a full list by calling
// VWFeatureBase::GetTargetContextFeatures(GetScoreProducerDescription())


class VWFeatureContext : public VWFeatureBase
{
public:
  VWFeatureContext(const std::string &line, size_t contextSize)
    : VWFeatureBase(line, vwft_targetContext), m_contextSize(contextSize) {
  }

  // Gets its pure virtual functions from VWFeatureBase

  virtual void operator()(const InputType &input
                          , const TargetPhrase &targetPhrase
                          , Discriminative::Classifier &classifier) const {
  }

  virtual void operator()(const InputType &input
                          , const Range &sourceRange
                          , Discriminative::Classifier &classifier) const {
  }

  virtual void SetParameter(const std::string& key, const std::string& value) {
    VWFeatureBase::SetParameter(key, value);
  }

  size_t GetContextSize() {
    return m_contextSize;
  }

protected:
  // Get word with the correct subset of factors as string. Because we're target
  // context features, we look at a limited number of words to the left of the
  // current translation. posFromEnd is interpreted like this:
  // 0 = last word of the hypothesis
  // 1 = next to last word
  // ...etc.
  inline std::string GetWord(const Phrase &phrase, size_t posFromEnd) const {
    return phrase.GetWord(phrase.GetSize() - posFromEnd - 1).GetString(m_targetFactors, false);
  }

  // required context size
  size_t m_contextSize;
};

}
