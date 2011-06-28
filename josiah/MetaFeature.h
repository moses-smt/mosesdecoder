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

#include "FeatureFunction.h"

namespace Josiah {

/**
  * A feature which is consists of a collection of other features.
 **/
 class MetaFeature : public Feature {
   public:
     MetaFeature(const FVector& weights, const FeatureVector& features);
     virtual FeatureFunctionHandle getFunction(const Sample& sample) const;
     
     const std::string& getName() const;
     FeatureFunctionVector getFeatureFunctions(const Sample& sample) const;
     const FVector& getWeights() const;
     
   private:
     FVector m_weights;
     FeatureVector m_features;
 };
 
 class MetaFeatureFunction : public SingleValuedFeatureFunction {
    
   public:
    
     MetaFeatureFunction(const Sample& sample, const MetaFeature& parent);

     virtual FValue computeScore();
    
     /** Score due to one segment */
     virtual FValue getSingleUpdateScore(const Moses::TranslationOption* option, const TargetGap& gap);
    
    
     /** Score due to two segments. The left and right refer to the target positions.**/
     virtual FValue getContiguousPairedUpdateScore(const Moses::TranslationOption* leftOption,const Moses::TranslationOption* rightOption, 
         const TargetGap& gap);
    
     virtual FValue getDiscontiguousPairedUpdateScore(const Moses::TranslationOption* leftOption,const Moses::TranslationOption* rightOption, 
         const TargetGap& leftGap, const TargetGap& rightGap);
    
     /** Score due to flip. Again, left and right refer to order on the <emph>target</emph> side. */
     virtual FValue getFlipUpdateScore(const Moses::TranslationOption* leftOption,const Moses::TranslationOption* rightOption, 
                                       const TargetGap& leftGap, const TargetGap& rightGap);
     
   private:
     const MetaFeature& m_parent;
     FeatureFunctionVector m_featureFunctions;
     
 };
 
}

