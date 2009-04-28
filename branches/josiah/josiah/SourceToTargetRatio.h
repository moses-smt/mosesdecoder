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

#pragma once

#include <algorithm>
#include <iostream>

#include "TypeDef.h"

#include "FeatureFunction.h"
#include "Gibbler.h"

namespace Moses {
  class Sample;
};

namespace Josiah {
  
class SourceToTargetRatio: public FeatureFunction {
public:
  SourceToTargetRatio() : FeatureFunction("SourceToTargetRatio") {}
  /** Initialise with new sample */
  virtual void init(const Moses::Sample& sample);      
  virtual float computeScore();
  /** Score due to  one segment */
  virtual float getSingleUpdateScore(const TranslationOption* option, const TargetGap& gap) ;
  /** Score due to two segments **/
  virtual float getContiguousPairedUpdateScore(const TranslationOption* leftOption, const TranslationOption* rightOption,
                                               const TargetGap& gap) ;
  
  virtual float getDiscontiguousPairedUpdateScore(const TranslationOption* leftOption, const TranslationOption* rightOption,
                                                  const TargetGap& leftGap, const TargetGap& rightGap);
  
  /** Score due to flip */
  virtual float getFlipUpdateScore(const TranslationOption* leftOption, const TranslationOption* rightOption,
                                   const TargetGap& leftGap, const TargetGap& rightGap) ;
  
  virtual ~SourceToTargetRatio() {}
private:
  const Moses::Sample* m_sample;
  size_t m_src_len;
};

}


