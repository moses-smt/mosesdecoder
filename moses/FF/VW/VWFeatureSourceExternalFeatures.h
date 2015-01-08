#pragma once

#include <string>
#include <cstdlib>

#include "ThreadLocalFeatureStorage.h"
#include "VWFeatureSource.h"
#include "TabbedSentence.h"

namespace Moses
{
  
class VWFeatureSourceExternalFeatures : public VWFeatureSource,
                                        public ThreadLocalFeatureStorage
{
  public:
    VWFeatureSourceExternalFeatures(const std::string &line)
      : VWFeatureSource(line), ThreadLocalFeatureStorage(this), m_column(0)
    {
      ReadParameters();
      
      // Call this last
      VWFeatureBase::UpdateRegister();
    }

    void operator()(const InputType &input
                  , const InputPath &inputPath
                  , const WordsRange &sourceRange
                  , Discriminative::Classifier *classifier) const
    {
      const Features& features = GetStoredFeatures();
      for (size_t i = 0; i < features.size(); i++) {
        classifier->AddLabelIndependentFeature(features[i]);
      }
    }
    
    virtual void SetParameter(const std::string& key, const std::string& value) {
      if(key == "column")
        m_column = Scan<size_t>(value);
      else
        VWFeatureSource::SetParameter(key, value);
    }
    
    virtual void InitializeForInput(InputType const& source) {
      ThreadLocalFeatureStorage::InitializeForInput(source);
      
      UTIL_THROW_IF2(source.GetType() != TabbedSentenceInput,
                     "This feature function requires the TabbedSentence input type");
      
      
      const TabbedSentence& tabbedSentence = static_cast<const TabbedSentence&>(source);
      const std::string &column = tabbedSentence.GetColumn(m_column);
      
      Features& features = GetStoredFeatures();
      Tokenize(features, column, " ");
    }
    
  private:
    size_t m_column;
};

}
