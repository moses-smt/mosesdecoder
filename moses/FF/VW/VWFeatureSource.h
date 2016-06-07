#pragma once

#include <string>
#include "VWFeatureBase.h"
#include "moses/InputType.h"

namespace Moses
{

// Inherit from this for source-dependent classifier features. They will
// automatically register with the classifier class named VW0 or one or more
// names specified by the used-by=name1,name2,... parameter.
//
// The classifier gets a full list by calling
// VWFeatureBase::GetSourceFeatures(GetScoreProducerDescription())


class VWFeatureSource : public VWFeatureBase
{
public:
  VWFeatureSource(const std::string &line)
    : VWFeatureBase(line, vwft_source) {
  }

  // Gets its pure virtual functions from VWFeatureBase

  virtual void operator()(const InputType &input
                          , const TargetPhrase &targetPhrase
                          , Discriminative::Classifier &classifier
                          , Discriminative::FeatureVector &outFeatures) const {
  }

  virtual void operator()(const InputType &input
                          , const Phrase &contextPhrase
                          , const AlignmentInfo &alignmentInfo
                          , Discriminative::Classifier &classifier
                          , Discriminative::FeatureVector &outFeatures) const {
  }

  virtual void SetParameter(const std::string& key, const std::string& value) {
    VWFeatureBase::SetParameter(key, value);
  }

protected:
  inline std::string GetWord(const InputType &input, size_t pos) const {
    return input.GetWord(pos).GetString(m_sourceFactors, false);
  }
};

}
