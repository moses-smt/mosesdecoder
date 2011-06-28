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
 
#include "MetaFeature.h"

using namespace std;

namespace Josiah {


MetaFeature::MetaFeature(const FVector& weights, const FeatureVector& features) :
    m_weights(weights),
    m_features(features){}

  

FeatureFunctionHandle MetaFeature::getFunction(const Sample& sample) const {
      return FeatureFunctionHandle(new MetaFeatureFunction(sample,*this));
}


FeatureFunctionVector MetaFeature::getFeatureFunctions(const Sample& sample) const {
  FeatureFunctionVector ffv;
  for (FeatureVector::const_iterator i = m_features.begin(); i != m_features.end(); ++i) {
    ffv.push_back((*i)->getFunction(sample));
  }
  return ffv;
}

const FVector& MetaFeature::getWeights() const {
  return m_weights;
}

MetaFeatureFunction::MetaFeatureFunction(const Sample& sample, const MetaFeature& parent) 
  : SingleValuedFeatureFunction(sample,"core"),
  m_parent(parent),
  m_featureFunctions(parent.getFeatureFunctions(sample))
{}



FValue MetaFeatureFunction::computeScore() {
  FVector scores;
  for (FeatureFunctionVector::const_iterator i = m_featureFunctions.begin(); i != m_featureFunctions.end(); ++i) {
    (*i)->assignScore(scores);
  }
  return scores.inner_product(m_parent.getWeights());
}
    
/** Score due to one segment */
FValue MetaFeatureFunction::getSingleUpdateScore(const Moses::TranslationOption* option, const TargetGap& gap) {
  FVector scores;
  for (FeatureFunctionVector::const_iterator i = m_featureFunctions.begin(); i != m_featureFunctions.end(); ++i) {
    (*i)->doSingleUpdate(option,gap,scores);
  }
  return scores.inner_product(m_parent.getWeights());
}


/** Score due to two segments. The left and right refer to the target positions.**/
FValue MetaFeatureFunction::getContiguousPairedUpdateScore(const Moses::TranslationOption* leftOption,const Moses::TranslationOption* rightOption, 
    const TargetGap& gap) 
{
  FVector scores;
  for (FeatureFunctionVector::const_iterator i = m_featureFunctions.begin(); i != m_featureFunctions.end(); ++i) {
    (*i)->doContiguousPairedUpdate(leftOption,rightOption,gap,scores);
  }
  return scores.inner_product(m_parent.getWeights());
}

FValue MetaFeatureFunction::getDiscontiguousPairedUpdateScore(const Moses::TranslationOption* leftOption,const Moses::TranslationOption* rightOption, 
    const TargetGap& leftGap, const TargetGap& rightGap) 
{
  FVector scores;
  for (FeatureFunctionVector::const_iterator i = m_featureFunctions.begin(); i != m_featureFunctions.end(); ++i) {
    (*i)->doDiscontiguousPairedUpdate(leftOption,rightOption,leftGap,rightGap,scores);
  }
  return scores.inner_product(m_parent.getWeights());
}

/** Score due to flip. Again, left and right refer to order on the <emph>target</emph> side. */
FValue MetaFeatureFunction::getFlipUpdateScore(const Moses::TranslationOption* leftOption,const Moses::TranslationOption* rightOption, 
                                       const TargetGap& leftGap, const TargetGap& rightGap) 
{
  FVector scores;
  for (FeatureFunctionVector::const_iterator i = m_featureFunctions.begin(); i != m_featureFunctions.end(); ++i) {
    (*i)->doFlipUpdate(leftOption,rightOption,leftGap,rightGap,scores);
  }
  return scores.inner_product(m_parent.getWeights());
}
                                  
}

