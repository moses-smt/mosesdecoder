#pragma once

#include <string>
#include <cstdlib>

#include "ThreadLocalByFeatureStorage.h"
#include "VWFeatureSource.h"
#include "TabbedSentence.h"

namespace Moses
{

// Assuming a given column of TabbedSentence contains space separated source features
class VWFeatureSourceExternalFeatures : public VWFeatureSource
{
  public:
    VWFeatureSourceExternalFeatures(const std::string &line)
      : VWFeatureSource(line), m_tls(this), m_column(0)
    {
      ReadParameters();
      
      // Call this last
      VWFeatureBase::UpdateRegister();
    }

    void operator()(const InputType &input
                  , const InputPath &inputPath
                  , const WordsRange &sourceRange
                  , Discriminative::Classifier &classifier) const
    {
      const Features& features = *m_tls.GetStored();
      for (size_t i = 0; i < features.size(); i++) {
        classifier.AddLabelIndependentFeature("srcext^" + features[i]);
      }
    }
    
    virtual void SetParameter(const std::string& key, const std::string& value) {
      if(key == "column")
        m_column = Scan<size_t>(value);
      else
        VWFeatureSource::SetParameter(key, value);
    }
    
    virtual void InitializeForInput(InputType const& source) {      
      UTIL_THROW_IF2(source.GetType() != TabbedSentenceInput,
                     "This feature function requires the TabbedSentence input type");
      
      const TabbedSentence& tabbedSentence = static_cast<const TabbedSentence&>(source);
      const std::string &column = tabbedSentence.GetColumn(m_column);
      
      Features& features = *m_tls.GetStored();
      features.clear();
      
      Tokenize(features, column, " ");
    }
    
  private:
    typedef std::vector<std::string> Features;
    typedef ThreadLocalByFeatureStorage<Features> TLSFeatures;

    TLSFeatures m_tls;    
    size_t m_column;
};

}
