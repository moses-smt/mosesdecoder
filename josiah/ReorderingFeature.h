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

#include <boost/unordered_set.hpp>

#include "DiscriminativeReorderingFeature.h"

#include "FeatureFunction.h"

namespace Josiah {

typedef boost::shared_ptr<const Moses::DiscriminativeReorderingState> DRStateHandle;


/**
 * Wraps the Moses DiscriminativeReorderingFeature
**/
class ReorderingFeature : public Feature {
  public:

  ReorderingFeature(const std::vector<std::string>& featureConfig,
                    const std::vector<std::string>& vocabConfig) :
    m_mosesFeature(featureConfig,vocabConfig) {}
  
  virtual FeatureFunctionHandle getFunction(const Sample& sample) const;
  
  
  private:
    Moses::DiscriminativeReorderingFeature m_mosesFeature;

};


class ReorderingFeatureFunction : public FeatureFunction {
    
  public:
    
    ReorderingFeatureFunction(const Sample& sample, 
      const Moses::DiscriminativeReorderingFeature& mosesFeature);
    
   
    /** Assign the total score of this feature on the current hypo */
    virtual void assignScore(FVector& scores);

    virtual void updateTarget();
    
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
    const Moses::DiscriminativeReorderingFeature& m_mosesFeature;
    Moses::ScoreComponentCollection m_accumulator;
    std::vector<DRStateHandle> m_prevStates;
};

}

