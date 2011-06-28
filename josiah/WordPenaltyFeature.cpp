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


#include "WordPenaltyFeature.h"

#include "DummyScoreProducers.h"
#include "Gibbler.h"
#include "StaticData.h"
#include "TranslationOption.h"

namespace Josiah {

FeatureFunctionHandle WordPenaltyFeature::getFunction(const Sample& sample) const {
  return FeatureFunctionHandle(new WordPenaltyFeatureFunction(sample));
}

WordPenaltyFeatureFunction::WordPenaltyFeatureFunction(const Sample& sample) :
  SingleValuedFeatureFunction(sample,
      StaticData::Instance().GetTranslationSystem(TranslationSystem::DEFAULT).
          GetWordPenaltyProducer()->GetScoreProducerDescription()) {}
  
FValue WordPenaltyFeatureFunction::computeScore() {
  return -(FValue)(getSample().GetTargetWords().size());
}

/** Score due to one segment */
FValue WordPenaltyFeatureFunction::getSingleUpdateScore(const TranslationOption* option, const TargetGap& gap) {
  return -(FValue)(option->GetTargetPhrase().GetSize());
}

/** Score due to two segments. The left and right refer to the target positions.**/
FValue WordPenaltyFeatureFunction::getContiguousPairedUpdateScore(const TranslationOption* leftOption,const TranslationOption* rightOption, 
    const TargetGap& gap) {
      return -(FValue)(leftOption->GetTargetPhrase().GetSize() + rightOption->GetTargetPhrase().GetSize());
  
}

FValue WordPenaltyFeatureFunction::getDiscontiguousPairedUpdateScore(const TranslationOption* leftOption,const TranslationOption* rightOption, 
    const TargetGap& leftGap, const TargetGap& rightGap) {
      return -(FValue)(leftOption->GetTargetPhrase().GetSize() + rightOption->GetTargetPhrase().GetSize());
}


/** Score due to flip. Again, left and right refer to order on the <emph>target</emph> side. */
FValue WordPenaltyFeatureFunction::getFlipUpdateScore(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                                  const TargetGap& leftGap, const TargetGap& rightGap)
{
  return -(FValue)(leftOption->GetTargetPhrase().GetSize() + rightOption->GetTargetPhrase().GetSize());
}

FeatureFunctionHandle UnknownWordPenaltyFeature::getFunction(const Sample& sample) const {
  return FeatureFunctionHandle(new UnknownWordPenaltyFeatureFunction(sample));
}

UnknownWordPenaltyFeatureFunction::UnknownWordPenaltyFeatureFunction(const Sample& sample) :
    SingleValuedFeatureFunction(sample,
      StaticData::Instance().GetTranslationSystem(TranslationSystem::DEFAULT).
        GetUnknownWordPenaltyProducer()->GetScoreProducerDescription()),
    m_producer(StaticData::Instance().GetTranslationSystem(TranslationSystem::DEFAULT).
        GetUnknownWordPenaltyProducer()
    ){}
  
FValue UnknownWordPenaltyFeatureFunction::computeScore() {
      FValue score = 0;
      const Hypothesis* currHypo = getSample().GetTargetTail();
      while ((currHypo = (currHypo->GetNextHypo()))) {
        score += currHypo->GetTranslationOption().GetScoreBreakdown().GetScoreForProducer(m_producer);
      }
      return score;
}

/** Score due to one segment */
FValue UnknownWordPenaltyFeatureFunction::getSingleUpdateScore(const TranslationOption* option, const TargetGap& gap) {
  return option->GetScoreBreakdown().GetScoreForProducer(m_producer);
}

/** Score due to two segments. The left and right refer to the target positions.**/
FValue UnknownWordPenaltyFeatureFunction::getContiguousPairedUpdateScore(const TranslationOption* leftOption,const TranslationOption* rightOption, 
    const TargetGap& gap) {
      return leftOption->GetScoreBreakdown().GetScoreForProducer(m_producer) +
          rightOption->GetScoreBreakdown().GetScoreForProducer(m_producer);
  
}

FValue UnknownWordPenaltyFeatureFunction::getDiscontiguousPairedUpdateScore(const TranslationOption* leftOption,const TranslationOption* rightOption, 
const TargetGap& leftGap, const TargetGap& rightGap) {
  return leftOption->GetScoreBreakdown().GetScoreForProducer(m_producer) +
      rightOption->GetScoreBreakdown().GetScoreForProducer(m_producer);
}


        /** Score due to flip. Again, left and right refer to order on the <emph>target</emph> side. */
FValue UnknownWordPenaltyFeatureFunction::getFlipUpdateScore(const TranslationOption* leftOption,const TranslationOption* rightOption, 
const TargetGap& leftGap, const TargetGap& rightGap) {
  return 0;
}


}//namespace
