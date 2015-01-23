#pragma once

#include <string>
#include <set>
#include <boost/lexical_cast.hpp>

#include "VWFeatureTarget.h"

namespace Moses
{

class VWFeatureTargetPhraseScores : public VWFeatureTarget
{
public:
  VWFeatureTargetPhraseScores(const std::string &line)
    : VWFeatureTarget(line) {
    ReadParameters();

    VWFeatureBase::UpdateRegister();
  }

  void operator()(const InputType &input
                  , const InputPath &inputPath
                  , const TargetPhrase &targetPhrase
                  , Discriminative::Classifier &classifier) const {
    std::vector<FeatureFunction*> features = FeatureFunction::GetFeatureFunctions();
    for (size_t i = 0; i < features.size(); i++) {
      std::string fname = features[i]->GetScoreProducerDescription();
      if(!m_fnames.empty() && m_fnames.count(fname) == 0)
        continue;

      std::vector<float> scores = targetPhrase.GetScoreBreakdown().GetScoresForProducer(features[i]);
      for(size_t j = 0; j < scores.size(); ++j)
        classifier.AddLabelDependentFeature(fname + "^" + boost::lexical_cast<std::string>(j), scores[j]);
    }
  }

  virtual void SetParameter(const std::string& key, const std::string& value) {
    if(key == "use") {
      std::vector<std::string> names;
      Tokenize(names, value, ",");
      m_fnames.insert(names.begin(), names.end());
    } else
      VWFeatureTarget::SetParameter(key, value);
  }

private:
  std::set<std::string> m_fnames;

};

}
