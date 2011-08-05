/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2011 University of Edinburgh

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

#include <string>
#include <boost/shared_ptr.hpp>
#include "../moses/src/FFState.h"
#include "../moses/src/FeatureFunction.h"
#include "FeatureFunction.h"

namespace Josiah {

class Sample;
typedef boost::shared_ptr<const FFState> StateHandle;

/**
  * Wraps a moses feature, whose state depends only on the previous phrase.
**/
class SingleStateFeature : public Feature {
  public:
    SingleStateFeature(const Moses::OptionStatefulFeatureFunction* mosesFeature) :
      m_mosesFeature(mosesFeature) {}
    virtual FeatureFunctionHandle getFunction(const Sample& sample) const;
   
   
  private:
    const Moses::OptionStatefulFeatureFunction* m_mosesFeature; 


};

class SingleStateFeatureFunction : public FeatureFunction {
  public:
    SingleStateFeatureFunction(
      const Sample& sample,
      const Moses::OptionStatefulFeatureFunction* mosesFeature):
        FeatureFunction(sample),
        m_mosesFeature(mosesFeature) {}

    /** Assign the total score of this feature on the current hypo */
    virtual void assignScore(FVector& scores);

    virtual void updateTarget();

    /** Score due to one segment */
    virtual void doSingleUpdate(const Moses::TranslationOption* option,
           const TargetGap& gap, FVector& scores);

    /** Score due to two segments.
       The left and right refer to the target positions.**/
    virtual void doContiguousPairedUpdate(
      const Moses::TranslationOption* leftOption,
      const Moses::TranslationOption* rightOption, 
      const TargetGap& gap, FVector& scores);

    virtual void doDiscontiguousPairedUpdate(
      const Moses::TranslationOption* leftOption,
      const Moses::TranslationOption* rightOption, 
      const TargetGap& leftGap,
      const TargetGap& rightGap,
      FVector& scores);
    
    /** Score due to flip.
      Again, left and right refer to order on the <emph>target</emph> side. */
    virtual void doFlipUpdate(
      const Moses::TranslationOption* leftOption,
      const Moses::TranslationOption* rightOption, 
      const TargetGap& leftGap,
      const TargetGap& rightGap,
      FVector& scores);

  private:
    const OptionStatefulFeatureFunction* m_mosesFeature; 
    Moses::ScoreComponentCollection m_accumulator;
    std::vector<StateHandle> m_prevStates;
 
};

}



