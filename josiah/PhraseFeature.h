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

#pragma once

#include <vector>

#include "FeatureFunction.h"
#include "PhraseDictionary.h"

namespace Josiah {
  
  

/** The Moses phrase features. */
class PhraseFeature : public Feature {
  public:
    PhraseFeature(PhraseDictionaryFeature* dictionary, size_t id);
    virtual FeatureFunctionHandle getFunction(const Sample& sample ) const;
    
    /** Inform all phrase features that the weights have been updated so 
    that the new weights can be passed to moses */
    static void updateWeights(const FVector& weights);
    
    
  private:
    static std::set<PhraseFeature*> s_phraseFeatures; 
    Moses::PhraseDictionaryFeature* m_phraseDictionary;
    std::vector<FName> m_featureNames;
  
};

class PhraseFeatureFunction : public FeatureFunction {
  
public:
  
  PhraseFeatureFunction(const Sample& sample, Moses::PhraseDictionaryFeature* phraseDictionary,  std::vector<FName> featureNames);
  
  /** Assign the total score of this feature on the current hypo */
  virtual void assignScore(FVector& scores);
  
  /** Score due to one segment */
  virtual void doSingleUpdate(const TranslationOption* option, const TargetGap& gap, FVector& scores);
  /** Score due to two segments. The left and right refer to the target positions.**/
  virtual void doContiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
      const TargetGap& gap, FVector& scores);
  virtual void doDiscontiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
      const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores);
  
  /** Score due to flip. Again, left and right refer to order on the <emph>target</emph> side. */
  virtual void doFlipUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                                    const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores);
  private:
    void assign(const TranslationOption* option, FVector& scores);
    std::vector<FName> m_featureNames;
    Moses::PhraseDictionaryFeature* m_phraseDictionary;
};
  
}

