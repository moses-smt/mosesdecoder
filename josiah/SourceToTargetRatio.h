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


namespace Josiah {

class Sample;

class SourceToTargetRatioFeature : public Feature {
  public:
    virtual FeatureFunctionHandle getFunction(const Sample& sample) const;

};
  
class SourceToTargetRatioFeatureFunction: public SingleValuedFeatureFunction {
public:
  SourceToTargetRatioFeatureFunction(const Sample& sample) : SingleValuedFeatureFunction(sample,"SourceToTargetRatio") 
  { m_src_len = sample.GetSourceSize();}
  virtual FValue computeScore();
  /** Score due to  one segment */
  virtual FValue getSingleUpdateScore(const TranslationOption* option, const TargetGap& gap) ;
  /** Score due to two segments **/
  virtual FValue getContiguousPairedUpdateScore(const TranslationOption* leftOption, const TranslationOption* rightOption,
                                               const TargetGap& gap) ;
  
  virtual FValue getDiscontiguousPairedUpdateScore(const TranslationOption* leftOption, const TranslationOption* rightOption,
                                                  const TargetGap& leftGap, const TargetGap& rightGap);
  
  /** Score due to flip */
  virtual FValue getFlipUpdateScore(const TranslationOption* leftOption, const TranslationOption* rightOption,
                                   const TargetGap& leftGap, const TargetGap& rightGap) ;
  
  virtual ~SourceToTargetRatioFeatureFunction() {}
private:
  size_t m_src_len;
};

}


