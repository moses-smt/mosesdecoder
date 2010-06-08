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

using namespace std;
using namespace Moses;

namespace Josiah {
  
    PhraseFeature::PhraseFeature() {
      vector<PhraseDictionary*> phraseTables = StaticData::Instance().GetPhraseDictionaries();
      if (phraseTables.size() != 1) {
        throw runtime_error("Only one phrase table is supported");
      }
      m_phraseDictionary = phraseTables[0];
      //pre-calculate the feature names
      const string& root = m_phraseDictionary->GetScoreProducerDescription();
      size_t featureCount = m_phraseDictionary->GetNumScoreComponents();
      for (size_t i = 0; i < featureCount; ++i) {
        ostringstream namestream;
        namestream << i;
        m_featureNames.push_back(FName(root,namestream.str()));
      }
      
    }
  
    /** Assign the total score of this feature on the current hypo */
    void PhraseFeature::assignScore(FVector& scores) {
      const Hypothesis* currHypo = m_sample->GetTargetTail();
      while ((currHypo = (currHypo->GetNextHypo()))) {
        assign(&(currHypo->GetTranslationOption()), scores);
      }
    }
    
    /** Score due to one segment */
    void PhraseFeature::doSingleUpdate(const TranslationOption* option, const TargetGap& gap, FVector& scores) {
      assign(option,scores);
    }
    
    
    /** Score due to two segments. The left and right refer to the target positions.**/
    void PhraseFeature::doContiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
        const TargetGap& gap, FVector& scores) {
      assign(leftOption,scores);
      assign(rightOption,scores);
      
    }
    void PhraseFeature::doDiscontiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
        const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores) {
       assign(leftOption,scores);
       assign(rightOption,scores);
      
    }
    
    /** Score due to flip. Again, left and right refer to order on the <emph>target</emph> side. */
    void PhraseFeature::doFlipUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                                     const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores) {
      assign(leftOption,scores);
      assign(rightOption,scores);
      
    }
    
    /** Add the phrase features into the feature vector */
    void PhraseFeature::assign(const TranslationOption* option, FVector& scores) {
      const vector<float>& phraseScores = option->GetScoreBreakdown().GetScoresForProducer(m_phraseDictionary);
      assert(phraseScores.size() == m_featureNames.size());
      for (size_t i = 0; i < phraseScores.size(); ++i) {
        scores[m_featureNames[i]]  = scores[m_featureNames[i]] +  phraseScores[i];
      }
    }
  
}
