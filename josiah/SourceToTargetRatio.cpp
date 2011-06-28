/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2009 University of Edinburgh
 
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
#include "SourceToTargetRatio.h"



using namespace std;
using namespace Moses;

namespace Josiah {
 
  FeatureFunctionHandle SourceToTargetRatioFeature::getFunction(const Sample& sample) const {
    return FeatureFunctionHandle(new SourceToTargetRatioFeatureFunction(sample));
  }
  
  FValue Josiah::SourceToTargetRatioFeatureFunction::computeScore() {
    return 1.0 - ((float) m_src_len /(float) getSample().GetTargetWords().size());
  }
  /** Score due to  one segment */
  FValue Josiah::SourceToTargetRatioFeatureFunction::getSingleUpdateScore(const TranslationOption* option, const TargetGap& gap) {
    return 1.0 - ((float) m_src_len / (float) (getSample().GetTargetWords().size() + option->GetTargetPhrase().GetSize() - gap.segment.GetNumWordsCovered()));
  }
  /** Score due to two segments **/
  FValue Josiah::SourceToTargetRatioFeatureFunction::getContiguousPairedUpdateScore(const TranslationOption* leftOption, const TranslationOption* rightOption,const TargetGap& gap) 
  {
      return 1.0 - ((float) m_src_len /(float)  (getSample().GetTargetWords().size() 
          + leftOption->GetTargetPhrase().GetSize() 
          + rightOption->GetTargetPhrase().GetSize() - gap.segment.GetNumWordsCovered()));
  }
  
  FValue Josiah::SourceToTargetRatioFeatureFunction::getDiscontiguousPairedUpdateScore(const TranslationOption* leftOption, const TranslationOption* rightOption,
                                                  const TargetGap& leftGap, const TargetGap& rightGap) 
  {
      return 1.0 - ((float) m_src_len /(float)  (getSample().GetTargetWords().size() + leftOption->GetTargetPhrase().GetSize() 
          + rightOption->GetTargetPhrase().GetSize() -
                        (leftGap.segment.GetNumWordsCovered() + rightGap.segment.GetNumWordsCovered() )) ) ;
  }
  
  /** Score due to flip */
  FValue Josiah::SourceToTargetRatioFeatureFunction::getFlipUpdateScore(const TranslationOption* leftOption, const TranslationOption* rightOption,
                                   const TargetGap& leftGap, const TargetGap& rightGap) {
    return computeScore();
  }  
  
}
