/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2010 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include "PhraseFeature.h"

#include <sstream>

#include "Gibbler.h"
#include "StaticData.h"
#include "WeightManager.h"

using namespace std;
using namespace Moses;

namespace Josiah {

  set<PhraseFeature*> PhraseFeature::s_phraseFeatures;
  
  PhraseFeature::PhraseFeature
    (PhraseDictionaryFeature* dictionary, size_t index) : m_phraseDictionary(dictionary) {
      //pre-calculate the feature names
      const string& root = m_phraseDictionary->GetScoreProducerDescription();
      size_t featureCount = m_phraseDictionary->GetNumScoreComponents();
      for (size_t i = 1; i <= featureCount; ++i) {
        ostringstream namestream;
        if (index > 0) {
          namestream << index << "-";
        }
        namestream << i;
        m_featureNames.push_back(FName(root,namestream.str()));
      }
      
      s_phraseFeatures.insert(this);
    }

    void PhraseFeature::updateWeights(const FVector& weights) {
      for (set<PhraseFeature*>::iterator i = s_phraseFeatures.begin();
          i != s_phraseFeatures.end(); ++i) {
        PhraseFeature* pf = *i;
        vector<float> newWeights(pf->m_featureNames.size());
        for (size_t j = 0; j < pf->m_featureNames.size(); ++j) {
          FValue weight = weights[pf->m_featureNames[j]];
          newWeights[j] = weight;
        }
        ScoreComponentCollection mosesWeights = StaticData::Instance().GetAllWeights();
        mosesWeights.Assign(pf->m_phraseDictionary,newWeights);
        (const_cast<StaticData&>(StaticData::Instance()))
          .SetAllWeights(mosesWeights);
        //pf->m_phraseDictionary->GetFeature()->SetWeightTransModel(newWeights);
      }
    }
    
    FeatureFunctionHandle PhraseFeature::getFunction(const Sample& sample ) const {
      return FeatureFunctionHandle
        (new PhraseFeatureFunction(sample,m_phraseDictionary,m_featureNames));
    }
    
    PhraseFeatureFunction::PhraseFeatureFunction(const Sample& sample, Moses::PhraseDictionaryFeature* phraseDictionary, std::vector<FName> featureNames) :
        FeatureFunction(sample), 
         m_featureNames(featureNames),
         m_phraseDictionary(phraseDictionary) {}
  
    /** Assign the total score of this feature on the current hypo */
    void PhraseFeatureFunction::assignScore(FVector& scores) {
      for (size_t i = 0; i < m_featureNames.size(); ++i) {
        scores[m_featureNames[i]] = 0;
      }
      const Hypothesis* currHypo = getSample().GetTargetTail();
      while ((currHypo = (currHypo->GetNextHypo()))) {
        assign(&(currHypo->GetTranslationOption()), scores);
      }
    }
    
    /** Score due to one segment */
    void PhraseFeatureFunction::doSingleUpdate(const TranslationOption* option, const TargetGap& gap, FVector& scores) {
      assign(option,scores);
    }
    
    
    /** Score due to two segments. The left and right refer to the target positions.**/
    void PhraseFeatureFunction::doContiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
        const TargetGap& gap, FVector& scores) {
      assign(leftOption,scores);
      assign(rightOption,scores);
      
    }
    void PhraseFeatureFunction::doDiscontiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
        const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores) {
       assign(leftOption,scores);
       assign(rightOption,scores);
      
    }
    
    /** Score due to flip. Again, left and right refer to order on the <emph>target</emph> side. */
    void PhraseFeatureFunction::doFlipUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                                     const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores) {
      assign(leftOption,scores);
      assign(rightOption,scores);
      
    }
    
    /** Add the phrase features into the feature vector */
    void PhraseFeatureFunction::assign(const TranslationOption* option, FVector& scores) {
      const ScoreComponentCollection& breakdown = option->GetScoreBreakdown();
      vector<float> mosesScores= breakdown.GetScoresForProducer(m_phraseDictionary);
      for (size_t i = 0; i < m_featureNames.size(); ++i) {
        scores[m_featureNames[i]] += mosesScores[i];
      }
    }
  
}
