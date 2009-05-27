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
 
  void Josiah::SourceToTargetRatio::init(const Sample& sample) {
    m_sample = &sample;
    m_src_len = m_sample->GetSourceSize();
  }
  float Josiah::SourceToTargetRatio::computeScore() {
    return 1.0 - ((float) m_src_len /(float) m_sample->GetTargetWords().size());
  }
  /** Score due to  one segment */
  float Josiah::SourceToTargetRatio::getSingleUpdateScore(const TranslationOption* option, const TargetGap& gap) {
    return 1.0 - ((float) m_src_len / (float) (m_sample->GetTargetWords().size() + option->GetTargetPhrase().GetSize() - gap.segment.GetNumWordsCovered()));
  }
  /** Score due to two segments **/
  float Josiah::SourceToTargetRatio::getContiguousPairedUpdateScore(const TranslationOption* leftOption, const TranslationOption* rightOption,
                                               const TargetGap& gap) {
    return 1.0 - ((float) m_src_len /(float)  (m_sample->GetTargetWords().size() + leftOption->GetTargetPhrase().GetSize() + rightOption->GetTargetPhrase().GetSize() - gap.segment.GetNumWordsCovered()));
  }
  
  float Josiah::SourceToTargetRatio::getDiscontiguousPairedUpdateScore(const TranslationOption* leftOption, const TranslationOption* rightOption,
                                                  const TargetGap& leftGap, const TargetGap& rightGap) {
    return 1.0 - ((float) m_src_len /(float)  (m_sample->GetTargetWords().size() + leftOption->GetTargetPhrase().GetSize() + rightOption->GetTargetPhrase().GetSize() -
                        (leftGap.segment.GetNumWordsCovered() + rightGap.segment.GetNumWordsCovered() )) ) ;
  }
  
  /** Score due to flip */
  float Josiah::SourceToTargetRatio::getFlipUpdateScore(const TranslationOption* leftOption, const TranslationOption* rightOption,
                                   const TargetGap& leftGap, const TargetGap& rightGap) {
    return computeScore();
  }  
  
}
