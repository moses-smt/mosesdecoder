#pragma once

#include <string>
#include <cstdlib>

#include "VWFeatureSource.h"
#include "TabbedSentence.h"

namespace Moses
{
  
class VWFeatureSourceExternalFeatures : public VWFeatureSource
{
  public:
    VWFeatureSourceExternalFeatures(const std::string &line)
      : VWFeatureSource(line), m_column(0)
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
      Features& features = (*m_nameMap)[GetScoreProducerDescription()];
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
      UTIL_THROW_IF2(source.GetType() != TabbedSentenceInput, "This feature function requires the TabbedSentence input type");
      
      const TabbedSentence& tabbedSentence = static_cast<const TabbedSentence&>(source);
      UTIL_THROW_IF2(tabbedSentence.GetColumns().size() <= m_column, "There is no column with index: " << m_column);
      
      if(!m_nameMap.get())
        m_nameMap.reset(new NameFeatureMap());
      
      (*m_nameMap)[GetScoreProducerDescription()].clear();
      
      Features& features = (*m_nameMap)[GetScoreProducerDescription()];
      const std::string &column = tabbedSentence.GetColumns()[m_column];
      
      Tokenize(features, column, " ");
    }
    
  private:
    size_t m_column;
    
    static TSNameFeatureMap m_nameMap;
};

}
